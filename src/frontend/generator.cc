#include <string>
#include "generator.h"

namespace cilly {

Generator::Generator() 
    : current_fn_(nullptr),
      next_local_index_(0) {}

// 主调用函数
Function Generator::Generate(const std::vector<StmtPtr>& program) { 
  Function script("script", 0);
  script.SetLocalCount(0);
  current_fn_ = &script;
  for (const auto& i : program) {
    EmitStmt(i);
  }
  current_fn_->SetLocalCount(next_local_index_);
  EmitConst(Value::Null());
  EmitOp(OpCode::OP_RETURN);
  current_fn_ = nullptr;
  return script;
}

void Generator::EmitStmt(const StmtPtr& stmt) {      // 分类处理不同类型语句
  switch (stmt->kind) {
    case Stmt::Kind::kPrint: {
      auto p = static_cast<PrintStmt*>(stmt.get());  // get()拿出unique_ptr中的原始指针stmt*
      EmitPrintStmt(p);                              // get只是借出对象，所有权依旧只有stmt，stmt离开作用于后p依旧会失效
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
    default:
      assert(false && "当前无法处理此类语句");
  }
}


void Generator::PatchJump(int jump_pos) {
  current_fn_->PatchI32(jump_pos, current_fn_->CodeSize());
  return;
}

void Generator::PatchJumpTo(int jump_pos, int32_t target){
  current_fn_->PatchI32(jump_pos, target);
  return;
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
  const std::string& name = stmt->name.lexeme;    // 加move不好排查
  if (local_.find(name) != local_.end()) {
    assert(false && "该变量已存在！");
  }
  int index = next_local_index_;
  local_[name] = index;   
  next_local_index_++;
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
  loop_stack_.emplace_back(); // 直接构造了，不用{}
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
  if(stmt->init)EmitStmt(stmt->init);
  int loop_start = current_fn_->CodeSize();
  EmitExpr(stmt->cond);
  EmitOp(OpCode::OP_JUMP_IF_FALSE);
  int end_lable = current_fn_->CodeSize();
  EmitI32(0);
  EmitStmt(stmt->body);
  for (auto i : loop_stack_.back().continue_jumps) {
    PatchJump(i);
  }
  if(stmt->step)EmitStmt(stmt->step);
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
  EmitOp(OpCode::OP_JUMP);  
  int break_pos = current_fn_->CodeSize();
  EmitI32(0);
  loop_stack_.back().break_jumps.emplace_back(break_pos);
  return;
}

void Generator::EmitContinueStmt(const ContinueStmt* stmt) {
  assert(!loop_stack_.empty() && "continue outside loop");
  EmitOp(OpCode::OP_JUMP);
  int continue_pos = current_fn_->CodeSize();
  EmitI32(0);
  loop_stack_.back().continue_jumps.emplace_back(continue_pos);
  return;
}

void Generator::EmitBlockStmt(const BlockStmt* stmt) {
  for (auto& i : stmt->statements) {
    EmitStmt(i);
  }
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

void Generator::EmitExpr(const ExprPtr& expr) {   // 分类处理不同类型表达式
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
  default:
    assert(false && "未找到此类表达式！");
    break;
  }
}

void Generator::EmitLiteralExpr(const LiteralExpr* expr) {\
  switch (expr->literal_kind) {
    case LiteralExpr::LiteralKind::kNumber: {
      double num = std::stod(expr->lexeme);    // stod将字符串转化为double
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
  auto it = local_.find(name);
  assert(it != local_.end() && "未定义该变量！");
  index = it->second;
  EmitOp(OpCode::OP_LOAD_VAR);
  EmitI32(index);
  return;
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
    EmitOp(OpCode::OP_DUP); // 防止set时没有保留dict副本导致无法连续set
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

}  // namespace cilly
