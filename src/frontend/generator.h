#ifndef CILLY_VM_CPP_GENERATOR_H_
#define CILLY_VM_CPP_GENERATOR_H_

#include <memory>
#include <unordered_map>
#include <vector>

#include "../function.h"
#include "../opcodes.h"
#include "ast.h"

namespace cilly::gc {
class Collector;
}

namespace cilly {

class ObjClass;

// 负责把 AST (Stmt/Expr) 生成字节码（一个 Function）
class Generator {
 public:
  // 生成器不一定需要 VM，先不依赖 VM，只生成 Function。
  Generator();
  explicit Generator(gc::Collector* gc);

  // 输入：一整棵 AST
  // 输出：一个可在 VM 中执行的 Function
  Function Generate(const std::vector<StmtPtr>& program);

  const std::vector<std::unique_ptr<Function>>& Functions() const {
    return functions_;
  }  // 只读接口

  int FindFunctionIndex(const std::string& name) const;

 private:
  // 当前正在生成字节码的函数
  Function* current_fn_;
  std::unordered_map<std::string, int> local_;
  std::unordered_map<std::string, int> globals_;
  int next_local_index_;
  int max_local_index_;

  // 工具
  void EmitStmt(const StmtPtr& stmt);
  void PatchJump(int jump_pos);
  void PatchJumpTo(int jump_pos, int32_t target);

  void PredeclareFunctions(const std::vector<StmtPtr>& program);
  void CompileFunctionBody(const FunctionStmt* stmt);
  void CompileFunctionBody(const FunctionStmt* stmt,
                           const std::string& compiled_name);  // 重载函数编译

  // 命名粉碎
  static std::string MangleMethodName(const std::string& cls,
                                      const std::string& method) {
    return cls + "::" + method;
  }

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
  void EmitPropAssignStmt(const PropAssignStmt* stmt);
  void EmitClassStmt(const ClassStmt* stmt);

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
  void EmitGetPropExpr(const GetPropExpr* expr);
  void EmitThisExpr(const ThisExpr* expr);
  void EmitSuperExpr(const SuperExpr* expr);

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
    std::unordered_map<std::string, std::shared_ptr<ObjClass>> class_shadows;
    std::vector<std::string> names;
  };

  std::vector<LoopContext> loop_stack_;
  std::vector<Scope> scope_stack_;
  std::vector<std::unique_ptr<Function>> functions_;  // 函数表
  std::unordered_map<std::string, int>
      func_name_to_index_;  // 函数名和索引的映射

  // 原生函数相关
  // 目前内建函数索引：
  // 0..4  : len/str/type/abs/clock
  // 5     : __test_emit（测试桥）
  // 6     : __gc_collect（GC bring-up）
  // 注意：这里的数量必须与 VM 注册的 builtin
  // 一致，否则生成字节码时会越界或映射错误。
  static constexpr int kBuiltinCount = 9;
  std::unordered_map<std::string, int> builtin_name_to_index_;
  std::unordered_map<std::string, int> builtin_name_to_arity_;

  void InitBuiltins();
  bool IsBuiltin(const std::string& name) const;
  int BuiltinIndex(const std::string& name) const;
  int BuiltinArity(const std::string& name) const;

  std::unordered_map<std::string, std::shared_ptr<ObjClass>>
      class_env_;  // 可见类对象表

  // 编译期类对象表：className -> ObjClass
  std::unordered_map<std::string, std::shared_ptr<ObjClass>> class_map_;

  // className -> superclassName（如果没有父类就不在表里）
  std::unordered_map<std::string, std::string> super_name_map_;

  // 当前正在编译的 method 属于哪个类（仅在 CompileFunctionBody(method)
  // 期间有效）
  std::string current_class_name_;

  // GC 相关
  gc::Collector* gc_ = nullptr;
};

}  // namespace cilly

#endif  // CILLY_VM_CPP_GENERATOR_H_
