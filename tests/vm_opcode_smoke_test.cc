#include <gtest/gtest.h>

#include "tests/test_helpers.h"

namespace cilly {

// ---------------- Arithmetic ----------------
TEST(VMOpcodes, Arithmetic_Div_Returns2) {
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
  Value ret = test::RunAndGetReturn(vm, fn);
  test::ExpectNum(ret, 2);
}

// ---------------- Variables ----------------
TEST(VMOpcodes, Variables_LoadStore_Returns30) {
  Function fn("main", 0);
  fn.SetLocalCount(2);  // local[0]=a, local[1]=b

  int c20 = fn.AddConst(Value::Num(20));
  int c10 = fn.AddConst(Value::Num(10));

  // a = 20
  fn.Emit(OpCode::OP_CONSTANT, 1);
  fn.EmitI32(c20, 1);
  fn.Emit(OpCode::OP_STORE_VAR, 1);
  fn.EmitI32(0, 1);

  // b = 10
  fn.Emit(OpCode::OP_CONSTANT, 1);
  fn.EmitI32(c10, 1);
  fn.Emit(OpCode::OP_STORE_VAR, 1);
  fn.EmitI32(1, 1);

  // return a + b
  fn.Emit(OpCode::OP_LOAD_VAR, 1);
  fn.EmitI32(0, 1);
  fn.Emit(OpCode::OP_LOAD_VAR, 1);
  fn.EmitI32(1, 1);
  fn.Emit(OpCode::OP_ADD, 1);
  fn.Emit(OpCode::OP_RETURN, 1);

  VM vm;
  Value ret = test::RunAndGetReturn(vm, fn);
  test::ExpectNum(ret, 30);
}

// ---------------- Compare ----------------
TEST(VMOpcodes, Compare_Less_ReturnsTrue) {
  Function fn("main", 0);
  fn.SetLocalCount(0);

  int c1 = fn.AddConst(Value::Num(1));
  int c2 = fn.AddConst(Value::Num(2));

  // return (1 < 2) => true
  fn.Emit(OpCode::OP_CONSTANT, 1);
  fn.EmitI32(c1, 1);
  fn.Emit(OpCode::OP_CONSTANT, 1);
  fn.EmitI32(c2, 1);
  fn.Emit(OpCode::OP_LESS, 1);
  fn.Emit(OpCode::OP_RETURN, 1);

  VM vm;
  Value ret = test::RunAndGetReturn(vm, fn);
  test::ExpectBool(ret, true);
}

TEST(VMOpcodes, Compare_Eq_ReturnsFalse) {
  Function fn("main", 0);
  fn.SetLocalCount(0);

  int c1 = fn.AddConst(Value::Num(1));
  int c2 = fn.AddConst(Value::Num(2));

  // return (1 == 2) => false
  fn.Emit(OpCode::OP_CONSTANT, 1);
  fn.EmitI32(c1, 1);
  fn.Emit(OpCode::OP_CONSTANT, 1);
  fn.EmitI32(c2, 1);
  fn.Emit(OpCode::OP_EQ, 1);
  fn.Emit(OpCode::OP_RETURN, 1);

  VM vm;
  Value ret = test::RunAndGetReturn(vm, fn);
  test::ExpectBool(ret, false);
}

// ---------------- Control Flow ----------------
TEST(VMOpcodes, ControlFlow_If_Returns888) {
  Function fn("if_test", 0);
  fn.SetLocalCount(0);

  int c1 = fn.AddConst(Value::Num(1));
  int c2 = fn.AddConst(Value::Num(2));
  int c888 = fn.AddConst(Value::Num(888));
  int c999 = fn.AddConst(Value::Num(999));

  // (1 == 2) -> false
  fn.Emit(OpCode::OP_CONSTANT, 1);
  fn.EmitI32(c1, 1);
  fn.Emit(OpCode::OP_CONSTANT, 1);
  fn.EmitI32(c2, 1);
  fn.Emit(OpCode::OP_EQ, 1);

  // if false -> else
  fn.Emit(OpCode::OP_JUMP_IF_FALSE, 1);
  int if_else_pos = fn.CodeSize();
  fn.EmitI32(0, 1);

  // then: push 999
  fn.Emit(OpCode::OP_CONSTANT, 1);
  fn.EmitI32(c999, 1);

  // jump end
  fn.Emit(OpCode::OP_JUMP, 1);
  int if_end_pos = fn.CodeSize();
  fn.EmitI32(0, 1);

  // else: push 888
  fn.PatchI32(if_else_pos, fn.CodeSize());
  fn.Emit(OpCode::OP_CONSTANT, 1);
  fn.EmitI32(c888, 1);

  // end: return top
  fn.PatchI32(if_end_pos, fn.CodeSize());
  fn.Emit(OpCode::OP_RETURN, 1);

  VM vm;
  Value ret = test::RunAndGetReturn(vm, fn);
  test::ExpectNum(ret, 888);
}

TEST(VMOpcodes, ControlFlow_ForLoop_Returns3_NoPrint) {
  Function fn("for_test", 0);
  fn.SetLocalCount(1);  // local[0] = i

  int c0 = fn.AddConst(Value::Num(0));
  int c1 = fn.AddConst(Value::Num(1));
  int c3 = fn.AddConst(Value::Num(3));

  // i = 0
  fn.Emit(OpCode::OP_CONSTANT, 1);
  fn.EmitI32(c0, 1);
  fn.Emit(OpCode::OP_STORE_VAR, 1);
  fn.EmitI32(0, 1);

  int loop_start = fn.CodeSize();

  // cond: i < 3
  fn.Emit(OpCode::OP_LOAD_VAR, 1);
  fn.EmitI32(0, 1);
  fn.Emit(OpCode::OP_CONSTANT, 1);
  fn.EmitI32(c3, 1);
  fn.Emit(OpCode::OP_LESS, 1);

  // if false -> end (patch later)
  fn.Emit(OpCode::OP_JUMP_IF_FALSE, 1);
  int end_label_pos = fn.CodeSize();
  fn.EmitI32(0, 1);

  // body: touch i but do not print
  fn.Emit(OpCode::OP_LOAD_VAR, 1);
  fn.EmitI32(0, 1);
  fn.Emit(OpCode::OP_POP, 1);

  // i = i + 1
  fn.Emit(OpCode::OP_LOAD_VAR, 1);
  fn.EmitI32(0, 1);
  fn.Emit(OpCode::OP_CONSTANT, 1);
  fn.EmitI32(c1, 1);
  fn.Emit(OpCode::OP_ADD, 1);
  fn.Emit(OpCode::OP_STORE_VAR, 1);
  fn.EmitI32(0, 1);

  // jump back
  fn.Emit(OpCode::OP_JUMP, 1);
  fn.EmitI32(loop_start, 1);

  // patch end label
  fn.PatchI32(end_label_pos, fn.CodeSize());

  // return i (expect 3)
  fn.Emit(OpCode::OP_LOAD_VAR, 1);
  fn.EmitI32(0, 1);
  fn.Emit(OpCode::OP_RETURN, 1);

  VM vm;
  Value ret = test::RunAndGetReturn(vm, fn);
  test::ExpectNum(ret, 3);
}

// ---------------- Call ----------------
TEST(VMOpcodes, Call_NoArgs_Returns41) {
  // callee(): return 20 + 21 => 41
  Function callee("callee", 0);
  callee.SetLocalCount(0);

  int c20 = callee.AddConst(Value::Num(20));
  int c21 = callee.AddConst(Value::Num(21));

  callee.Emit(OpCode::OP_CONSTANT, 1);
  callee.EmitI32(c20, 1);
  callee.Emit(OpCode::OP_CONSTANT, 1);
  callee.EmitI32(c21, 1);
  callee.Emit(OpCode::OP_ADD, 1);
  callee.Emit(OpCode::OP_RETURN, 1);

  VM vm = test::MakeVMWithBuiltins();
  int fid = vm.RegisterFunction(&callee);

  // main: call callee(); return
  Function fn("main", 0);
  fn.SetLocalCount(0);

  fn.Emit(OpCode::OP_CALL, 1);
  fn.EmitI32(fid, 1);
  fn.Emit(OpCode::OP_RETURN, 1);

  Value ret = test::RunAndGetReturn(vm, fn);
  test::ExpectNum(ret, 41);
}

TEST(VMOpcodes, Call_With2Args_Returns41) {
  // add(a,b): return a + b
  Function add("add", 2);
  add.SetLocalCount(2);  // local[0]=a, local[1]=b

  add.Emit(OpCode::OP_LOAD_VAR, 1);
  add.EmitI32(0, 1);
  add.Emit(OpCode::OP_LOAD_VAR, 1);
  add.EmitI32(1, 1);
  add.Emit(OpCode::OP_ADD, 1);
  add.Emit(OpCode::OP_RETURN, 1);

  VM vm = test::MakeVMWithBuiltins();
  int fid = vm.RegisterFunction(&add);

  // main: return add(20, 21)
  Function fn("main", 0);
  fn.SetLocalCount(0);

  int c20 = fn.AddConst(Value::Num(20));
  int c21 = fn.AddConst(Value::Num(21));

  fn.Emit(OpCode::OP_CONSTANT, 1);
  fn.EmitI32(c20, 1);
  fn.Emit(OpCode::OP_CONSTANT, 1);
  fn.EmitI32(c21, 1);

  fn.Emit(OpCode::OP_CALL, 1);
  fn.EmitI32(fid, 1);
  fn.Emit(OpCode::OP_RETURN, 1);

  Value ret = test::RunAndGetReturn(vm, fn);
  test::ExpectNum(ret, 41);
}

// ---------------- Recursion / Mutual Recursion ----------------
TEST(VMOpcodes, MutualRecursion_OddEven_ReturnsTrue) {
  Function odd_fn("odd", 1);
  Function even_fn("even", 1);
  Function main_fn("main", 0);

  odd_fn.SetLocalCount(1);
  even_fn.SetLocalCount(1);
  main_fn.SetLocalCount(0);

  VM vm = test::MakeVMWithBuiltins();
  int odd_id = vm.RegisterFunction(&odd_fn);
  int even_id = vm.RegisterFunction(&even_fn);

  // even(n): if n==0 return true else return odd(n-1)
  int even_c0 = even_fn.AddConst(Value::Num(0));
  int even_c1 = even_fn.AddConst(Value::Num(1));
  int even_true = even_fn.AddConst(Value::Bool(true));

  even_fn.Emit(OpCode::OP_LOAD_VAR, 1);
  even_fn.EmitI32(0, 1);
  even_fn.Emit(OpCode::OP_CONSTANT, 1);
  even_fn.EmitI32(even_c0, 1);
  even_fn.Emit(OpCode::OP_EQ, 1);

  even_fn.Emit(OpCode::OP_JUMP_IF_FALSE, 1);
  int even_else_pos = even_fn.CodeSize();
  even_fn.EmitI32(0, 1);

  even_fn.Emit(OpCode::OP_CONSTANT, 1);
  even_fn.EmitI32(even_true, 1);
  even_fn.Emit(OpCode::OP_RETURN, 1);

  even_fn.PatchI32(even_else_pos, even_fn.CodeSize());

  even_fn.Emit(OpCode::OP_LOAD_VAR, 1);
  even_fn.EmitI32(0, 1);
  even_fn.Emit(OpCode::OP_CONSTANT, 1);
  even_fn.EmitI32(even_c1, 1);
  even_fn.Emit(OpCode::OP_SUB, 1);
  even_fn.Emit(OpCode::OP_CALL, 1);
  even_fn.EmitI32(odd_id, 1);
  even_fn.Emit(OpCode::OP_RETURN, 1);

  // odd(n): if n==0 return false else return even(n-1)
  int odd_c0 = odd_fn.AddConst(Value::Num(0));
  int odd_c1 = odd_fn.AddConst(Value::Num(1));
  int odd_false = odd_fn.AddConst(Value::Bool(false));

  odd_fn.Emit(OpCode::OP_LOAD_VAR, 1);
  odd_fn.EmitI32(0, 1);
  odd_fn.Emit(OpCode::OP_CONSTANT, 1);
  odd_fn.EmitI32(odd_c0, 1);
  odd_fn.Emit(OpCode::OP_EQ, 1);

  odd_fn.Emit(OpCode::OP_JUMP_IF_FALSE, 1);
  int odd_else_pos = odd_fn.CodeSize();
  odd_fn.EmitI32(0, 1);

  odd_fn.Emit(OpCode::OP_CONSTANT, 1);
  odd_fn.EmitI32(odd_false, 1);
  odd_fn.Emit(OpCode::OP_RETURN, 1);

  odd_fn.PatchI32(odd_else_pos, odd_fn.CodeSize());

  odd_fn.Emit(OpCode::OP_LOAD_VAR, 1);
  odd_fn.EmitI32(0, 1);
  odd_fn.Emit(OpCode::OP_CONSTANT, 1);
  odd_fn.EmitI32(odd_c1, 1);
  odd_fn.Emit(OpCode::OP_SUB, 1);
  odd_fn.Emit(OpCode::OP_CALL, 1);
  odd_fn.EmitI32(even_id, 1);
  odd_fn.Emit(OpCode::OP_RETURN, 1);

  // main: call even(3) and pop; return odd(3)
  int c3 = main_fn.AddConst(Value::Num(3));

  main_fn.Emit(OpCode::OP_CONSTANT, 1);
  main_fn.EmitI32(c3, 1);
  main_fn.Emit(OpCode::OP_CALL, 1);
  main_fn.EmitI32(even_id, 1);
  main_fn.Emit(OpCode::OP_POP, 1);

  main_fn.Emit(OpCode::OP_CONSTANT, 1);
  main_fn.EmitI32(c3, 1);
  main_fn.Emit(OpCode::OP_CALL, 1);
  main_fn.EmitI32(odd_id, 1);
  main_fn.Emit(OpCode::OP_RETURN, 1);

  Value ret = test::RunAndGetReturn(vm, main_fn);
  test::ExpectBool(ret, true);
}

}  // namespace cilly
