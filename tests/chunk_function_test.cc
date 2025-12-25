#include <gtest/gtest.h>

#include "src/chunk.h"
#include "src/function.h"
#include "src/opcodes.h"
#include "src/value.h"

namespace cilly {

TEST(Chunk, AddConstAndReadBack) {
  Chunk c;

  int i0 = c.AddConst(Value::Num(1));
  int i1 = c.AddConst(Value::Str("hi"));
  int i2 = c.AddConst(Value::Bool(true));

  EXPECT_EQ(i0, 0);
  EXPECT_EQ(i1, 1);
  EXPECT_EQ(i2, 2);
  EXPECT_EQ(c.ConstSize(), 3);

  EXPECT_TRUE(c.ConstAt(0).IsNum());
  EXPECT_DOUBLE_EQ(c.ConstAt(0).AsNum(), 1);

  EXPECT_TRUE(c.ConstAt(1).IsStr());
  EXPECT_EQ(c.ConstAt(1).AsStr(), "hi");

  EXPECT_TRUE(c.ConstAt(2).IsBool());
  EXPECT_EQ(c.ConstAt(2).AsBool(), true);
}

TEST(Chunk, EmitAndPatchI32) {
  Chunk c;

  c.Emit(OpCode::OP_JUMP, 123);
  int patch_pos = c.CodeSize();
  c.EmitI32(0, 123);  // placeholder

  EXPECT_EQ(c.CodeSize(), patch_pos + 1);
  EXPECT_EQ(c.LineAt(0), 123);

  c.PatchI32(patch_pos, 777);
  EXPECT_EQ(c.CodeAt(patch_pos), 777);
}

TEST(Function, ForwardsToChunk) {
  Function fn("main", 0);
  fn.SetLocalCount(0);

  int c10 = fn.AddConst(Value::Num(10));
  EXPECT_EQ(fn.ConstSize(), 1);

  fn.Emit(OpCode::OP_CONSTANT, 1);
  fn.EmitI32(c10, 1);
  fn.Emit(OpCode::OP_RETURN, 1);

  EXPECT_EQ(fn.CodeSize(), 3);
  EXPECT_EQ(fn.Name(), "main");
  EXPECT_EQ(fn.arity(), 0);
}

}  // namespace cilly
