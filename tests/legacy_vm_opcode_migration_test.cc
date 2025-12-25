#include <gtest/gtest.h>

#include "src/vm.h"
#include "src/function.h"
#include "src/value.h"

// 如果你的 RegisterBuiltins 定义在某个头文件里，按实际 include
#include "src/builtins.h"  // 可能是 "builtins.h" / "src/builtins.h"，按你工程实际改

namespace cilly {

TEST(LegacyVMOpcodeMigration, VMTest_Returns2) {
  Function fn("main", 0);
  fn.SetLocalCount(0);

  int c6 = fn.AddConst(Value::Num(6));
  int c3 = fn.AddConst(Value::Num(3));

  fn.Emit(OpCode::OP_CONSTANT, 1);
  fn.EmitI32(c6, 1);
  fn.Emit(OpCode::OP_CONSTANT, 1);
  fn.EmitI32(c3, 1);
  fn.Emit(OpCode::OP_DIV, 1);

  fn.Emit(OpCode::OP_RETURN, 1);

  VM vm;
  Value ret = vm.Run(fn);  // ? 这里假设 vm.Run 会返回 Value（如果你的 Run 是 void，看下一段“解释为什么”）
  EXPECT_TRUE(ret.IsNum());
  EXPECT_DOUBLE_EQ(ret.AsNum(), 2);
}

}  // namespace cilly
