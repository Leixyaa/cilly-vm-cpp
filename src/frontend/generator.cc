#include <string>
#include "generator.h"

namespace cilly {

Generator::Generator() : current_fn_(nullptr) {}

// 主调用函数
Function Generator::Generate(const std::vector<StmtPtr>& program) { 
  Function script;
  script.SetLocalCount(0);
  current_fn_ = &script;
  for (const auto& i : program) {
    EmitStmt(i);
  }
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
  }
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
    assert(false && "还未设置变量部分！");
    auto p = static_cast<VariableExpr*>(expr.get());
    EmitVariableExpr(p);
    break;
  }
  }
}

void Generator::EmitLiteralExpr(const LiteralExpr* expr) {
  double num = std::stod(expr->lexeme);    // stod将字符串转化为double
  Value v = Value::Num(num);
  EmitConst(v);
  return;
}

void Generator::EmitVariableExpr(const VariableExpr* expr) {
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
  }
  return;
}

void Generator::EmitOp(OpCode op) {
  current_fn_->Emit(op, 1);
  return;
}

void Generator::EmitConst(const Value& v) {
  int index = current_fn_->AddConst(v);
  current_fn_->Emit(OpCode::OP_CONSTANT, 1);
  current_fn_->EmitI32(index, 1);
  return;
}

}  // namespace cilly
