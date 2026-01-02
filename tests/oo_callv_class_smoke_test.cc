#include <gtest/gtest.h>

#include <vector>

#include "src/builtins.h"
#include "src/function.h"
#include "src/object.h"
#include "src/opcodes.h"
#include "src/value.h"
#include "src/vm.h"

namespace cilly {

TEST(OOFoundation, CallV_OnClass_CreatesInstance_AndPropWorks) {
  auto klass = std::make_shared<ObjClass>("A");

  // bytecode script:
  // var o = A(); o.x = 42; __test_emit(o.x); return 0;
  Function fn("script", 0);
  fn.SetLocalCount(1);  // local[0] = o

  const int k_class = fn.AddConst(Value::Obj(klass));
  const int k_name_x = fn.AddConst(Value::Str("x"));
  const int k_42 = fn.AddConst(Value::Num(42));
  const int k_0 = fn.AddConst(Value::Num(0));

  // o = A()
  fn.Emit(OpCode::OP_CONSTANT, 1);
  fn.EmitI32(k_class, 1);
  fn.Emit(OpCode::OP_CALLV, 1);
  fn.EmitI32(0, 1);          // argc = 0
  fn.Emit(OpCode::OP_STORE_VAR, 1);
  fn.EmitI32(0, 1);          // store to local 0

  // o.x = 42
  fn.Emit(OpCode::OP_LOAD_VAR, 1);
  fn.EmitI32(0, 1);          // load o
  fn.Emit(OpCode::OP_CONSTANT, 1);
  fn.EmitI32(k_42, 1);
  fn.Emit(OpCode::OP_SET_PROP, 1);
  fn.EmitI32(k_name_x, 1);

  // __test_emit(o.x)
  fn.Emit(OpCode::OP_LOAD_VAR, 1);
  fn.EmitI32(0, 1);
  fn.Emit(OpCode::OP_GET_PROP, 1);
  fn.EmitI32(k_name_x, 1);
  fn.Emit(OpCode::OP_CALL, 1);
  fn.EmitI32(5, 1);          // builtin __test_emit index == 5
  fn.Emit(OpCode::OP_POP, 1);

  // return 0
  fn.Emit(OpCode::OP_CONSTANT, 1);
  fn.EmitI32(k_0, 1);
  fn.Emit(OpCode::OP_RETURN, 1);

  VM vm;
  RegisterBuiltins(vm);

  std::vector<Value> emitted;
  vm.SetTestEmitSink([&](const Value& v) { emitted.push_back(v); });

  vm.Run(fn);

  ASSERT_EQ(emitted.size(), 1u);
  ASSERT_TRUE(emitted[0].IsNum());
  EXPECT_DOUBLE_EQ(emitted[0].AsNum(), 42);
}

}  // namespace cilly
