#include <gtest/gtest.h>

#include <vector>

#include "src/builtins.h"
#include "src/function.h"
#include "src/object.h"
#include "src/opcodes.h"
#include "src/value.h"
#include "src/vm.h"

namespace cilly {

TEST(OOFoundation, DotProp_OnInstance) {
  // Keep class alive (ObjInstance stores ObjClass*).
  auto klass = std::make_shared<ObjClass>("A");
  auto inst = std::make_shared<ObjInstance>(klass);


  // Build bytecode:
  // inst.x = 123; __test_emit(inst.x); return 0;
  Function fn("script", 0);
  fn.SetLocalCount(0);

  const int k_inst = fn.AddConst(Value::Obj(inst));
  const int k_123 = fn.AddConst(Value::Num(123));
  const int k_0 = fn.AddConst(Value::Num(0));
  const int k_name_x = fn.AddConst(Value::Str("x"));

  // inst.x = 123
  fn.Emit(OpCode::OP_CONSTANT, 1);
  fn.EmitI32(k_inst, 1);
  fn.Emit(OpCode::OP_CONSTANT, 1);
  fn.EmitI32(k_123, 1);
  fn.Emit(OpCode::OP_SET_PROP, 1);
  fn.EmitI32(k_name_x, 1);

  // __test_emit(inst.x)
  fn.Emit(OpCode::OP_CONSTANT, 1);
  fn.EmitI32(k_inst, 1);
  fn.Emit(OpCode::OP_GET_PROP, 1);
  fn.EmitI32(k_name_x, 1);
  fn.Emit(OpCode::OP_CALL, 1);
  fn.EmitI32(5, 1);  // builtin: __test_emit index == 5
  fn.Emit(OpCode::OP_POP, 1);  // pop return of __test_emit

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
  EXPECT_DOUBLE_EQ(emitted[0].AsNum(), 123);
}

}  // namespace cilly
