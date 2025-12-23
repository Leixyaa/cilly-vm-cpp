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

  const std::vector<std::unique_ptr<Function>>& Functions() const { return functions_; } // 只读接口
  int FindFunctionIndex(const std::string& name) const;
  

 private:
  // 当前正在生成字节码的函数
  Function* current_fn_;
  std::unordered_map<std::string, int> local_;
  int next_local_index_;
  int max_local_index_;
 
  
  // 工具：生成一条语句
  void EmitStmt(const StmtPtr& stmt);
  void PatchJump(int jump_pos);
  void PatchJumpTo(int jump_pos, int32_t target);
  
  void PredeclareFunctions(const std::vector<StmtPtr>& program);
  void CompileFunctionBody(const FunctionStmt* stmt);

  // 语句分支
  void EmitPrintStmt(const PrintStmt* stmt);
  void EmitExprStmt(const ExprStmt* stmt);
  void EmitVarStmt(const VarStmt* stmt);
  void EmitAssignStmt(const AssignStmt* stmt);
  void EmitWhileStmt(const WhileStmt* stmt);
  void EmitForStmt(const ForStmt* stmt);
  void EmitBreakStmt(const BreakStmt* stmt);
  void EmitContinueStmt(const ContinueStmt* stmt);
  void EmitBlockStmt(const BlockStmt* stmt);
  void EmitIndexAssignStmt(const IndexAssignStmt* stmt);
  void EmitIfStmt(const IfStmt* stmt);
  void EmitReturnStmt(const ReturnStmt* stmt);
  void EmitFunctionStmt(const FunctionStmt* stmt);

  // 工具：生成一个表达式的字节码，把结果留在栈顶
  void EmitExpr(const ExprPtr& expr);
  void EmitLiteralExpr(const LiteralExpr* expr);
  void EmitVariableExpr(const VariableExpr* expr);
  void EmitBinaryExpr(const BinaryExpr* expr);
  void EmitUnaryExpr(const UnaryExpr* expr);
  void EmitListExpr(const ListExpr* expr);
  void EmitDictExpr(const DictExpr* expr);
  void EmitIndexExpr(const IndexExpr* expr);
  void EmitCallExpr(const CallExpr* expr);
  // 在运行时路径上清理即将跳出的 block locals（只 emit OP_POPN，不改编译期栈）
  void EmitUnwindToDepth(int target_depth);

  // 以后可能会有 UnaryExpr / AssignExpr 等

  // 往 current_fn_ 里 emit 指令
  void EmitOp(OpCode op);
  void EmitI32(int32_t v);
  void EmitConst(const Value& v);

  // 记录所有 break,continue 的 jump 占位
  struct LoopContext {
    int scope_depth = 0;   
    std::vector<int> break_jumps;  
    std::vector<int> continue_jumps;
  };

  struct Scope {
    int start_local = 0;
    std::unordered_map<std::string, int> shadowns;
    std::vector<std::string> names;
  };
  
  std::vector<LoopContext> loop_stack_;
  std::vector<Scope> scope_stack_;
  std::vector<std::unique_ptr<Function>> functions_;  // 函数表
  std::unordered_map<std::string, int> func_name_to_index_; // 函数名和索引的映射

  // 原生函数相关
  static constexpr int kBuiltinCount = 5;
  std::unordered_map<std::string, int> builtin_name_to_index_;
  std::unordered_map<std::string, int> builtin_name_to_arity_;

  void InitBuiltins();
  bool IsBuiltin(const std::string& name) const;
  int BuiltinIndex(const std::string& name) const;
  int BuiltinArity(const std::string& name) const;
};

}  // namespace cilly

#endif  // CILLY_VM_CPP_GENERATOR_H_
