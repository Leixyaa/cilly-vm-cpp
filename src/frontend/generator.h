#ifndef CILLY_VM_CPP_GENERATOR_H_
#define CILLY_VM_CPP_GENERATOR_H_

#include <memory>
#include <vector>
#include <unordered_map>
#include "ast.h"
#include "../function.h"
#include "../opcodes.h"


namespace cilly {

// 负责把 AST (Stmt/Expr) 生成字节码（一个 Function）
class Generator {
 public:
  // 生成器不一定需要 VM，先不依赖 VM，只生成 Function。
  Generator();

  // 输入：一整棵 AST
  // 输出：一个可在 VM 中执行的 Function
  Function Generate(const std::vector<StmtPtr>& program);

 private:
  // 当前正在生成字节码的函数
  Function* current_fn_;
  std::unordered_map<std::string, int> local_;
  int next_local_index_;

  // 工具：生成一条语句
  void EmitStmt(const StmtPtr& stmt);
  void PatchJump(int jump_pos);


  // 未来会扩展：var/if/while/for 等
  void EmitPrintStmt(const PrintStmt* stmt);
  void EmitExprStmt(const ExprStmt* stmt);
  void EmitVarStmt(const VarStmt* stmt);
  void EmitAssignStmt(const AssignStmt* stmt);
  void EmitWhileStmt(const WhileStmt* stmt);
  void EmitBlockStmt(const BlockStmt* stmt);
  void EmitIndexAssignStmt(const IndexAssignStmt* stmt);

  // 工具：生成一个表达式的字节码，把结果留在栈顶
  void EmitExpr(const ExprPtr& expr);
  void EmitLiteralExpr(const LiteralExpr* expr);
  void EmitVariableExpr(const VariableExpr* expr);
  void EmitBinaryExpr(const BinaryExpr* expr);
  void EmitListExpr(const ListExpr* expr);
  void EmitDictExpr(const DictExpr* expr);
  void EmitIndexExpr(const IndexExpr* expr);
  // 以后可能会有 UnaryExpr / AssignExpr 等

  // 往 current_fn_ 里 emit 指令
  void EmitOp(OpCode op);
  void EmitI32(int32_t v);
  void EmitConst(const Value& v);
};

}  // namespace cilly

#endif  // CILLY_VM_CPP_GENERATOR_H_
