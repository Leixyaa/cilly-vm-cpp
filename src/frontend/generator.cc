#include "generator.h"

#include <algorithm>
#include <iostream>
#include <string>

namespace cilly {

Generator::Generator() :
    current_fn_(nullptr),
    next_local_index_(0),
    max_local_index_(0),
    scope_stack_() {
  scope_stack_.emplace_back();  // 只会在第一次创建实例的时候执行
  scope_stack_.back().start_local = 0;
  scope_stack_.back().shadowns = local_;
  InitBuiltins();
}

// 主调用函数
Function Generator::Generate(const std::vector<StmtPtr>& program) {
  // 每次编译一个脚本，都要从干净状态开始
  // （注意：functions_ / func_name_to_index_ 要在开头清，不能在结尾清）

  PredeclareFunctions(program);
  for (const auto& s : program) {
    if (s->kind == Stmt::Kind::kFun) {
      CompileFunctionBody(static_cast<FunctionStmt*>(s.get()));
    }
  }

  local_.clear();
  loop_stack_.clear();
  scope_stack_.clear();
  scope_stack_.emplace_back();
  scope_stack_.back().start_local = 0;
  scope_stack_.back().shadowns = local_;

  next_local_index_ = 0;
  max_local_index_ = 0;

  Function script("script", 0);
  current_fn_ = &script;

  for (const auto& s : program) {
    if (s->kind == Stmt::Kind::kFun)
      continue;  // 不在顶层执行fun相关，前面已经编译过一次了
    EmitStmt(s);
  }

  current_fn_->SetLocalCount(max_local_index_);

  // 隐式 return null
  EmitConst(Value::Null());
  EmitOp(OpCode::OP_RETURN);

  current_fn_ = nullptr;
  return script;
}

int Generator::FindFunctionIndex(const std::string& name) const {
  auto it = func_name_to_index_.find(name);
  return it == func_name_to_index_.end() ? -1 : it->second;
}

void Generator::EmitStmt(const StmtPtr& stmt) {  // 分类处理不同类型语句
  switch (stmt->kind) {
    case Stmt::Kind::kPrint: {
      auto p = static_cast<PrintStmt*>(
          stmt.get());  // get()拿出unique_ptr中的原始指针stmt*
      EmitPrintStmt(
          p);  // get只是借出对象，所有权依旧只有stmt，stmt离开作用于后p依旧会失效
      break;
    }
    case Stmt::Kind::kExpr: {
      auto p = static_cast<ExprStmt*>(stmt.get());
      EmitExprStmt(p);
      break;
    }
    case Stmt::Kind::kVar: {
      auto p = static_cast<VarStmt*>(stmt.get());
      EmitVarStmt(p);
      break;
    }
    case Stmt::Kind::kAssign: {
      auto p = static_cast<AssignStmt*>(stmt.get());
      EmitAssignStmt(p);
      break;
    }
    case Stmt::Kind::kWhile: {
      auto p = static_cast<WhileStmt*>(stmt.get());
      EmitWhileStmt(p);
      break;
    }
    case Stmt::Kind::kFor: {
      auto p = static_cast<ForStmt*>(stmt.get());
      EmitForStmt(p);
      break;
    }
    case Stmt::Kind::kBreak: {
      auto p = static_cast<BreakStmt*>(stmt.get());
      EmitBreakStmt(p);
      break;
    }
    case Stmt::Kind::kContinue: {
      auto p = static_cast<ContinueStmt*>(stmt.get());
      EmitContinueStmt(p);
      break;
    }
    case Stmt::Kind::kBlock: {
      auto p = static_cast<BlockStmt*>(stmt.get());
      EmitBlockStmt(p);
      break;
    }
    case Stmt::Kind::kIndexAssign: {
      auto p = static_cast<IndexAssignStmt*>(stmt.get());
      EmitIndexAssignStmt(p);
      break;
    }
    case Stmt::Kind::kIf: {
      auto p = static_cast<IfStmt*>(stmt.get());
      EmitIfStmt(p);
      break;
    }
    case Stmt::Kind::kReturn: {
      auto p = static_cast<ReturnStmt*>(stmt.get());
      EmitReturnStmt(p);
      break;
    }
    case Stmt::Kind::kFun: {
      return;
    }
    default:
      assert(false && "当前无法处理此类语句");
  }
}

void Generator::PatchJump(int jump_pos) {
  current_fn_->PatchI32(jump_pos, current_fn_->CodeSize());
  return;
}

void Generator::PatchJumpTo(int jump_pos, int32_t target) {
  current_fn_->PatchI32(jump_pos, target);
  return;
}

void Generator::PredeclareFunctions(const std::vector<StmtPtr>& program) {
  functions_.clear();
  func_name_to_index_.clear();

  for (const auto& s : program) {
    if (s->kind != Stmt::Kind::kFun)
      continue;
    auto* fn = static_cast<FunctionStmt*>(s.get());
    const std::string& name = fn->name.lexeme;

    assert(func_name_to_index_.count(name) == 0 && "duplicate function name");

    int idx = static_cast<int>(functions_.size());
    functions_.push_back(
        std::make_unique<Function>(name, (int)fn->params.size()));
    func_name_to_index_[name] = idx;
  }
}

void Generator::CompileFunctionBody(const FunctionStmt* stmt) {
  int idx = FindFunctionIndex(stmt->name.lexeme);
  assert(idx >= 0);

  // 保存当前 script 现场
  Function* saved_fn = current_fn_;
  auto saved_local = local_;
  int saved_next = next_local_index_;
  int saved_max = max_local_index_;
  auto saved_loop = loop_stack_;
  auto saved_scope = scope_stack_;

  // 切到该函数（第一遍已经创建好空 Function）
  current_fn_ = functions_[idx].get();

  // 清空编译状态（函数内部独立）
  local_.clear();
  loop_stack_.clear();
  scope_stack_.clear();
  scope_stack_.emplace_back();
  scope_stack_.back().start_local = 0;
  scope_stack_.back().shadowns = local_;

  next_local_index_ = 0;
  max_local_index_ = 0;

  // 参数绑定：params -> locals[0..arity-1]
  std::unordered_map<std::string, bool> seen;
  for (int i = 0; i < (int)stmt->params.size(); i++) {
    const std::string& pname = stmt->params[i].lexeme;
    assert(!seen.count(pname) && "duplicate parameter name");
    seen[pname] = true;

    local_[pname] = i;
    next_local_index_++;
    if (next_local_index_ > max_local_index_)
      max_local_index_ = next_local_index_;
  }

  // 编 body
  EmitBlockStmt(stmt->body.get());

  // 隐式 return null
  EmitConst(Value::Null());
  EmitOp(OpCode::OP_RETURN);

  current_fn_->SetLocalCount(max_local_index_);

  // 恢复 script 现场
  current_fn_ = saved_fn;
  local_ = std::move(saved_local);
  next_local_index_ = saved_next;
  max_local_index_ = saved_max;
  loop_stack_ = std::move(saved_loop);
  scope_stack_ = std::move(saved_scope);
}

void Generator::EmitPrintStmt(const PrintStmt* stmt) {
  EmitExpr(stmt->expr);
  EmitOp(OpCode::OP_PRINT);
  return;
}

void Generator::EmitExprStmt(const ExprStmt* stmt) {
  EmitExpr(stmt->expr);
  EmitOp(OpCode::OP_POP);
  return;
}

void Generator::EmitVarStmt(const VarStmt* stmt) {
  const std::string& name = stmt->name.lexeme;  // 加move不好排查
  auto& names = scope_stack_.back().names;
  if (std::find(names.begin(), names.end(), name) != names.end()) {
    assert(false && "该变量已存在！");
  }
  names.emplace_back(name);
  int index = next_local_index_;
  local_[name] = index;
  next_local_index_++;
  max_local_index_ = max_local_index_ < next_local_index_ ? next_local_index_
                                                          : max_local_index_;
  if (stmt->initializer) {
    EmitExpr(stmt->initializer);
  } else {
    EmitConst(Value::Null());
  }
  EmitOp(OpCode::OP_STORE_VAR);
  EmitI32(index);
  return;
}

void Generator::EmitAssignStmt(const AssignStmt* stmt) {
  const std::string& name = stmt->name.lexeme;
  EmitExpr(stmt->expr);
  auto it = local_.find(name);
  if (it == local_.end()) {
    assert(false && "该变量未定义！");
  }
  EmitOp(OpCode::OP_STORE_VAR);
  EmitI32(it->second);
  return;
}

void Generator::EmitWhileStmt(const WhileStmt* stmt) {
  loop_stack_.emplace_back();  // 直接构造了，不用{}
  loop_stack_.back().scope_depth = static_cast<int>(scope_stack_.size());
  int loop_start = current_fn_->CodeSize();
  EmitExpr(stmt->cond);
  EmitOp(OpCode::OP_JUMP_IF_FALSE);
  int end_lable = current_fn_->CodeSize();
  EmitI32(0);
  EmitStmt(stmt->body);
  for (auto i : loop_stack_.back().continue_jumps) {
    PatchJumpTo(i, loop_start);
  }
  EmitOp(OpCode::OP_JUMP);
  EmitI32(loop_start);
  PatchJump(end_lable);
  for (auto i : loop_stack_.back().break_jumps) {
    PatchJump(i);
  }
  loop_stack_.pop_back();
  return;
}

void Generator::EmitForStmt(const ForStmt* stmt) {
  loop_stack_.emplace_back();
  loop_stack_.back().scope_depth = static_cast<int>(scope_stack_.size());
  if (stmt->init)
    EmitStmt(stmt->init);
  int loop_start = current_fn_->CodeSize();
  EmitExpr(stmt->cond);
  EmitOp(OpCode::OP_JUMP_IF_FALSE);
  int end_lable = current_fn_->CodeSize();
  EmitI32(0);
  EmitStmt(stmt->body);
  for (auto i : loop_stack_.back().continue_jumps) {
    PatchJump(i);
  }
  if (stmt->step)
    EmitStmt(stmt->step);
  EmitOp(OpCode::OP_JUMP);
  EmitI32(loop_start);
  PatchJump(end_lable);
  for (auto i : loop_stack_.back().break_jumps) {
    PatchJump(i);
  }
  loop_stack_.pop_back();
  return;
}

void Generator::EmitBreakStmt(const BreakStmt* stmt) {
  assert(!loop_stack_.empty() && "break outside loop");
  EmitUnwindToDepth(loop_stack_.back().scope_depth);
  EmitOp(OpCode::OP_JUMP);
  int break_pos = current_fn_->CodeSize();
  EmitI32(0);
  loop_stack_.back().break_jumps.emplace_back(break_pos);
  return;
}

void Generator::EmitContinueStmt(const ContinueStmt* stmt) {
  assert(!loop_stack_.empty() && "continue outside loop");
  EmitUnwindToDepth(loop_stack_.back().scope_depth);
  EmitOp(OpCode::OP_JUMP);
  int continue_pos = current_fn_->CodeSize();
  EmitI32(0);
  loop_stack_.back().continue_jumps.emplace_back(continue_pos);
  return;
}

void Generator::EmitBlockStmt(const BlockStmt* stmt) {
  scope_stack_.emplace_back();
  scope_stack_.back().start_local = next_local_index_;
  scope_stack_.back().shadowns = local_;
  for (auto& i : stmt->statements) {
    EmitStmt(i);
  }
  int start = scope_stack_.back().start_local;
  int count = next_local_index_ - scope_stack_.back().start_local;
  EmitOp(OpCode::OP_POPN);
  EmitI32(start);
  EmitI32(count);
  next_local_index_ = scope_stack_.back().start_local;
  local_ = scope_stack_.back().shadowns;
  scope_stack_.pop_back();
  return;
}

void Generator::EmitIndexAssignStmt(const IndexAssignStmt* stmt) {
  EmitExpr(stmt->object);
  EmitExpr(stmt->index);
  EmitExpr(stmt->expr);
  EmitOp(OpCode::OP_INDEX_SET);
  return;
}

void Generator::EmitIfStmt(const IfStmt* stmt) {
  EmitExpr(stmt->cond);
  EmitOp(OpCode::OP_JUMP_IF_FALSE);
  int pos_else = current_fn_->CodeSize();
  EmitI32(0);

  EmitStmt(stmt->then_branch);
  EmitOp(OpCode::OP_JUMP);
  int pos_then = current_fn_->CodeSize();
  EmitI32(0);
  PatchJump(pos_else);
  if (stmt->else_branch) {
    EmitStmt(stmt->else_branch);
  }
  PatchJump(pos_then);
  return;
}

void Generator::EmitReturnStmt(const ReturnStmt* stmt) {
  EmitExpr(stmt->expr);
  EmitUnwindToDepth(1);  // 清理运行是local表，清理知道最外层根作用域;
  EmitOp(OpCode::OP_RETURN);
  return;
}

void Generator::EmitFunctionStmt(const FunctionStmt* stmt) {
  // 重名检查
  assert(FindFunctionIndex(stmt->name.lexeme) == -1 && "该函数名已定义！");

  // 保存当前script现场
  Function* saved_fn = current_fn_;
  auto saved_local = local_;
  int saved_next = next_local_index_;
  int saved_max = max_local_index_;
  auto saved_loop = loop_stack_;
  auto saved_scope = scope_stack_;

  // 编译函数
  Function fn(stmt->name.lexeme, static_cast<int>(stmt->params.size()));

  // 清空编译状态
  local_.clear();
  loop_stack_.clear();
  scope_stack_.clear();
  scope_stack_.emplace_back();
  scope_stack_.back().start_local = 0;
  scope_stack_.back().shadowns = local_;

  next_local_index_ = 0;
  max_local_index_ = 0;

  // 将参数放入当前local变量表中
  {
    std::unordered_map<std::string, int> seen;
    for (int i = 0; i < static_cast<int>(stmt->params.size()); i++) {
      const std::string& pname = stmt->params[i].lexeme;
      assert(seen.find(pname) == seen.end() && "函数参数变量名已定义！");
      seen[pname]++;
      local_[pname] = i;
      next_local_index_++;
      if (next_local_index_ > max_local_index_) {
        max_local_index_ = next_local_index_;
      }
    }
  }
  int index = static_cast<int>(functions_.size());
  functions_.push_back(std::make_unique<Function>(std::move(fn)));
  func_name_to_index_[stmt->name.lexeme] = index;
  current_fn_ = functions_[index].get();

  EmitBlockStmt(stmt->body.get());
  EmitConst(Value::Null());
  EmitOp(OpCode::OP_RETURN);  // 隐式return
  current_fn_->SetLocalCount(max_local_index_);

  // 回复上一层的script相关信息
  current_fn_ = saved_fn;
  local_ = std::move(saved_local);
  next_local_index_ = saved_next;
  max_local_index_ = saved_max;
  loop_stack_ = std::move(saved_loop);
  scope_stack_ = std::move(saved_scope);
}

void Generator::EmitExpr(const ExprPtr& expr) {  // 分类处理不同类型表达式
  switch (expr->kind) {
    case Expr::Kind::kLiteral: {
      auto p = static_cast<LiteralExpr*>(expr.get());
      EmitLiteralExpr(p);
      break;
    }
    case Expr::Kind::kBinary: {
      auto p = static_cast<BinaryExpr*>(expr.get());
      EmitBinaryExpr(p);
      break;
    }
    case Expr::Kind::kUnaryExpr: {
      auto p = static_cast<UnaryExpr*>(expr.get());
      EmitUnaryExpr(p);
      break;
    }
    case Expr::Kind::kVariable: {
      auto p = static_cast<VariableExpr*>(expr.get());
      EmitVariableExpr(p);
      break;
    }
    case Expr::Kind::kList: {
      auto p = static_cast<ListExpr*>(expr.get());
      EmitListExpr(p);
      break;
    }
    case Expr::Kind::kDict: {
      auto p = static_cast<DictExpr*>(expr.get());
      EmitDictExpr(p);
      break;
    }
    case Expr::Kind::kIndex: {
      auto p = static_cast<IndexExpr*>(expr.get());
      EmitIndexExpr(p);
      break;
    }
    case Expr::Kind::kCall: {
      auto p = static_cast<CallExpr*>(expr.get());
      EmitCallExpr(p);
      break;
    }
    default:
      assert(false && "未找到此类表达式！");
      break;
  }
}

void Generator::EmitLiteralExpr(const LiteralExpr* expr) {
  switch (expr->literal_kind) {
    case LiteralExpr::LiteralKind::kNumber: {
      double num = std::stod(expr->lexeme);  // stod将字符串转化为double
      Value v = Value::Num(num);
      EmitConst(v);
      return;
    }
    case LiteralExpr::LiteralKind::kString: {
      std::string s = expr->lexeme;
      Value v = Value::Str(s);
      EmitConst(v);
      return;
    }
    case LiteralExpr::LiteralKind::kBool: {
      Value v = Value::Bool(expr->lexeme == "true");
      EmitConst(v);
      return;
    }
    case LiteralExpr::LiteralKind::kNull: {
      Value v = Value::Null();
      EmitConst(v);
      return;
    }
    default:
      assert(false && "未定义此种字面量!");
  }
}

void Generator::EmitVariableExpr(const VariableExpr* expr) {
  const std::string& name = expr->name.lexeme;
  int index;
  // 变量
  auto it = local_.find(name);
  if (it != local_.end()) {
    EmitOp(OpCode::OP_LOAD_VAR);
    EmitI32(it->second);
    return;
  }

  // native function
  if (IsBuiltin(name)) {
    int index = BuiltinIndex(name);
    EmitConst(Value::Callable(index));
    return;
  }

  // 用户函数
  int user_index = FindFunctionIndex(name);
  if (user_index >= 0) {
    EmitConst(Value::Callable(user_index + kBuiltinCount));
    return;
  }
  assert(false && "Undefined variable/function name.");
}

void Generator::EmitBinaryExpr(const BinaryExpr* expr) {
  EmitExpr(expr->left);
  EmitExpr(expr->right);
  switch (expr->op.kind) {
    case TokenKind::kPlus:
      EmitOp(OpCode::OP_ADD);
      break;
    case TokenKind::kMinus:
      EmitOp(OpCode::OP_SUB);
      break;
    case TokenKind::kStar:
      EmitOp(OpCode::OP_MUL);
      break;
    case TokenKind::kSlash:
      EmitOp(OpCode::OP_DIV);
      break;
    case TokenKind::kLess:
      EmitOp(OpCode::OP_LESS);
      break;
    case TokenKind::kEqualEqual:
      EmitOp(OpCode::OP_EQ);
      break;
    case TokenKind::kNotEqual:
      EmitOp(OpCode::OP_NOT_EQUAL);
      break;
    case TokenKind::kGreater:
      EmitOp(OpCode::OP_GREATER);
      break;
    default:
      assert(false && "未找到此类二元运算表达式！");
      break;
  }
  return;
}

void Generator::EmitUnaryExpr(const UnaryExpr* expr) {
  switch (expr->op.kind) {
    case TokenKind::kNot: {
      EmitExpr(expr->expr);
      EmitOp(OpCode::OP_NOT);
      break;
    }

    case TokenKind::kMinus: {
      EmitConst(Value::Num(0));
      EmitExpr(expr->expr);
      EmitOp(OpCode::OP_SUB);
      break;
    }

    default:
      assert(false && "未定义此种一元表达式！");
      break;
  }

  return;
}

void Generator::EmitListExpr(const ListExpr* expr) {
  EmitOp(OpCode::OP_LIST_NEW);
  for (auto& i : expr->elements) {
    EmitExpr(i);
    EmitOp(OpCode::OP_LIST_PUSH);
  }
  return;
}

void Generator::EmitDictExpr(const DictExpr* expr) {
  EmitOp(OpCode::OP_DICT_NEW);
  for (auto& i : expr->entries) {
    EmitOp(OpCode::OP_DUP);  // 防止set时没有保留dict副本导致无法连续set
    EmitConst(Value::Str(i.first));
    EmitExpr(i.second);
    EmitOp(OpCode::OP_INDEX_SET);
  }
  return;
}

void Generator::EmitIndexExpr(const IndexExpr* expr) {
  EmitExpr(expr->object);
  EmitExpr(expr->expr);
  EmitOp(OpCode::OP_INDEX_GET);
}

void Generator::EmitCallExpr(const CallExpr* expr) {
  EmitExpr(expr->callee);
  int argc = (int)expr->arg.size();
  assert(argc <= 255 && "Too many arguments");
  for (int i = 0; i < argc; i++) {
    EmitExpr(expr->arg[i]);
  }

  EmitOp(OpCode::OP_CALLV);
  EmitI32(argc);
  return;
}

void Generator::EmitUnwindToDepth(int target_depth) {
  // target_depth 表示：希望回到 scope_stack_ 的这个大小（不包含更深层 block）
  int cur_depth = static_cast<int>(scope_stack_.size());
  assert(target_depth >= 0 && target_depth <= cur_depth);

  // 从最内层 scope 开始往外清理
  int end = next_local_index_;
  for (int i = cur_depth - 1; i >= target_depth; --i) {
    int start = scope_stack_[i].start_local;
    int count = end - start;
    // next_local_index_ 是“当前编译点已分配的最大 local 下标”
    // start_local 是进入该 block 时的下标
    // 它们的差就是：这个 block 内声明了多少 locals 需要清理

    if (count > 0) {
      EmitOp(OpCode::OP_POPN);
      EmitI32(start);
      EmitI32(count);
    }
    end = start;
  }
}

void Generator::EmitOp(OpCode op) {
  current_fn_->Emit(op, 1);
  return;
}

void Generator::EmitI32(int32_t v) {
  current_fn_->EmitI32(v, 1);
  return;
}

void Generator::EmitConst(const Value& v) {
  int index = current_fn_->AddConst(v);
  current_fn_->Emit(OpCode::OP_CONSTANT, 1);
  current_fn_->EmitI32(index, 1);
  return;
}

void Generator::InitBuiltins() {
  builtin_name_to_arity_.clear();
  builtin_name_to_index_.clear();
  // 固定顺序 = 固定 index（必须和 VM 注册顺序一致）
  builtin_name_to_index_["len"] = 0;
  builtin_name_to_arity_["len"] = 1;
  builtin_name_to_index_["str"] = 1;
  builtin_name_to_arity_["str"] = 1;
  builtin_name_to_index_["type"] = 2;
  builtin_name_to_arity_["type"] = 1;
  builtin_name_to_index_["abs"] = 3;
  builtin_name_to_arity_["abs"] = 1;
  builtin_name_to_index_["clock"] = 4;
  builtin_name_to_arity_["clock"] = 0;
  builtin_name_to_index_["__test_emit"] = 5;
  builtin_name_to_arity_["__test_emit"] = 1;
}

bool Generator::IsBuiltin(const std::string& name) const {
  return builtin_name_to_index_.count(name) != 0;
}

int Generator::BuiltinIndex(const std::string& name) const {
  auto it = builtin_name_to_index_.find(name);
  return it == builtin_name_to_index_.end() ? -1 : it->second;
}

int Generator::BuiltinArity(const std::string& name) const {
  auto it = builtin_name_to_arity_.find(name);
  return it == builtin_name_to_arity_.end() ? -1 : it->second;
}

}  // namespace cilly
