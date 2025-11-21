// cilly-vm-cpp
// Author: Leixyaa
// Date: 11.6
// Description: Entry point for testing environment.

#include "vm.h"
#include "chunk.h"
#include "opcodes.h"
#include "value.h"
#include "stack_stats.h"
#include "function.h"
#include <iostream>

// ---------------- Value 封装自测 ----------------
void ValueTest(){
  std::cout << "Value 封装自测 :\n" << std::endl;

  cilly::Value vnull = cilly::Value::Null();
  cilly::Value vtrue = cilly::Value::Bool(true);
  cilly::Value vnum  = cilly::Value::Num(10);
  cilly::Value vstr  = cilly::Value::Str("hi");

  std::cout << vnull.ToRepr() << std::endl;
  std::cout << vtrue.ToRepr() << std::endl;
  std::cout << vnum.ToRepr() << std::endl;
  std::cout << vstr.ToRepr() << std::endl;
  std::cout << (vnum == cilly::Value::Num(10)) << std::endl;
  std::cout << (vnum == vstr) << std::endl;
  std::cout << "---------------------------------------------\n";
}

// ---------------- Stack 封装自测 ----------------
void StackTest(){
  std::cout << "Stack 封装自测 :\n" << std::endl;

  cilly::StackStats s;
  s.Push(cilly::Value::Num(10));
  s.Push(cilly::Value::Num(10));
  s.Push(cilly::Value::Str("stack_test_"));
  auto v = s.Pop();
  std::cout << s.PushCount() << "," << s.PopCount() << "," << s.Depth()  << "," << s.MaxDepth() << "," << v.ToRepr() << std::endl;
  std::cout << "---------------------------------------------\n";
}

// ---------------- Chunk 封装自测 ----------------
void ChunkTest() {
  using namespace cilly;
  std::cout << "Chunk 封装自测 :\n" << std::endl;

  Chunk ch;

  int c0 = ch.AddConst(Value::Num(10));
  int c1 = ch.AddConst(Value::Num(20));

  ch.Emit(OpCode::OP_CONSTANT, 1);  
  ch.EmitI32(c0, 1);
  ch.Emit(OpCode::OP_CONSTANT, 1); 
  ch.EmitI32(c1, 1);
  ch.Emit(OpCode::OP_ADD, 1);       
  ch.Emit(OpCode::OP_PRINT, 1);     

  std::cout << "CodeSize = " << ch.CodeSize() << "\n";
  std::cout << "ConstSize = " << ch.ConstSize() << "\n\n";

  std::cout << "Code stream (index : value [line]):\n";
  for (int i = 0; i < ch.CodeSize(); ++i) {
    std::cout << "  " << i << " : " << ch.CodeAt(i)
              << " [line " << ch.LineAt(i) << "]\n";
  }

  std::cout << "\nConst pool:\n";
  for (int i = 0; i < ch.ConstSize(); ++i) {
    std::cout << "  " << i << " : " << ch.ConstAt(i).ToRepr() << "\n";
  }
  std::cout << "---------------------------------------------\n";
}

// ---------------- Function 封装自测 ----------------
void FunctionTest() {
  using namespace cilly;
  std::cout << "Function 封装自测 :\n" << std::endl;

  Function fn("main", /*arity=*/0);

  int c0 = fn.AddConst(Value::Num(10));
  int c1 = fn.AddConst(Value::Num(20));

  fn.Emit(OpCode::OP_CONSTANT, 1);
  fn.EmitI32(c0, 1);
  fn.Emit(OpCode::OP_CONSTANT, 1);
  fn.EmitI32(c1, 1);
  fn.Emit(OpCode::OP_ADD, 1);
  fn.Emit(OpCode::OP_PRINT, 1);

  std::cout << "Function name=" << fn.name()
            << ", arity=" << fn.arity() << "\n";
  std::cout << "CodeSize=" << fn.chunk().CodeSize()
            << ", ConstSize=" << fn.chunk().ConstSize() << "\n";

  for (int i = 0; i < fn.chunk().CodeSize(); ++i) {
    std::cout << "  code[" << i << "]=" << fn.chunk().CodeAt(i)
              << " @line " << fn.chunk().LineAt(i) << "\n";
  }
  for (int i = 0; i < fn.chunk().ConstSize(); ++i) {
    std::cout << "  const[" << i << "]=" << fn.chunk().ConstAt(i).ToRepr()
              << "\n";
  }
  std::cout << "---------------------------------------------\n";
}

//---------------- VM 最小执行循环自测 ----------------
void VMTest() {
  using namespace cilly;
  std::cout << "VM 最小执行循环自测 :\n" << std::endl;

  Function fn("main", /*arity=*/0);
  
  int c0 = fn.AddConst(Value::Num(10));
  int c1 = fn.AddConst(Value::Num(2));
  int c2 = fn.AddConst(Value::Num(3));
  int c3 = fn.AddConst(Value::Num(2));
// 10 - 2 * 3 / 2 = 12
fn.Emit(OpCode::OP_CONSTANT, 1); fn.EmitI32(c0, 1);   // 10
fn.Emit(OpCode::OP_CONSTANT, 1); fn.EmitI32(c1, 1);   // 2
fn.Emit(OpCode::OP_SUB, 1);                           // 10 - 2 = 8
fn.Emit(OpCode::OP_CONSTANT, 1); fn.EmitI32(c2, 1);   // 3
fn.Emit(OpCode::OP_MUL, 1);                           // 8 * 3 = 24
fn.Emit(OpCode::OP_CONSTANT, 1); fn.EmitI32(c3, 1);   // 2
fn.Emit(OpCode::OP_DIV, 1);                           // 24 / 2 = 12
fn.Emit(OpCode::OP_PRINT, 1);                         // 打印结果s
fn.Emit(OpCode::OP_RETURN, 1);

  VM vm;
  vm.Run(fn);  //12

std::cout << "PushCount = "   << vm.PushCount() 
          << ", PopCount = "  << vm.PopCount()
          << ", Depth = "     << vm.Depth()
          << ", MaxDepth = "  << vm.MaxDepth() << std::endl;

  std::cout << "---------------------------------------------\n";
}


//---------------- 变量系统自测 ---------------- 
void VarTest() {
  using namespace cilly;
  std::cout << "变量系统自测:\n" << std::endl;

  Function fn("main", /*arity=*/0);
  fn.SetLocalCount(2);  // 我们有两个局部变量

  // locals_[0] = 10
  int c10 = fn.AddConst(Value::Num(10));
  fn.Emit(OpCode::OP_CONSTANT, 1);
  fn.EmitI32(c10, 1);
  fn.Emit(OpCode::OP_STORE_VAR, 1);
  fn.EmitI32(0, 1);   // local[0]

  // locals_[1] = 20
  int c20 = fn.AddConst(Value::Num(20));
  fn.Emit(OpCode::OP_CONSTANT, 1);
  fn.EmitI32(c20, 1);
  fn.Emit(OpCode::OP_STORE_VAR, 1);
  fn.EmitI32(1, 1);

  // print locals_[0] + locals_[1]
  fn.Emit(OpCode::OP_LOAD_VAR, 1); fn.EmitI32(0, 1);
  fn.Emit(OpCode::OP_LOAD_VAR, 1); fn.EmitI32(1, 1);
  fn.Emit(OpCode::OP_ADD, 1);
  fn.Emit(OpCode::OP_PRINT, 1);

  // return final value
  fn.Emit(OpCode::OP_RETURN, 1);

  VM vm;
  vm.Run(fn);

  std::cout << "---------------------------------------------\n";

}

// ---------------- 简单函数调用自测 ----------------
//被调用函数返回 42，主函数调用并打印
void CallTest() {
  using namespace cilly;
  std::cout << "简单函数调用自测:\n" << std::endl;

  // 准备一个被调用函数：返回常量 42
  Function callee("const42", /*arity=*/0);
  int c42 = callee.AddConst(Value::Num(42));
  callee.Emit(OpCode::OP_CONSTANT, 1);
  callee.EmitI32(c42, 1);
  callee.Emit(OpCode::OP_RETURN, 1);  // 返回栈顶  42

  // 准备主函数,调用 callee，打印返回值，然后返回
  Function main_fn("main", /*arity=*/0);

  VM vm;
  int callee_id = vm.RegisterFunction(&callee);  // 注册被调用函数

  // main_fn 的字节码：
  // call const42
  main_fn.Emit(OpCode::OP_CALL, 1);
  main_fn.EmitI32(callee_id, 1);

  // print 返回值, 此时返回值已压回栈顶
  main_fn.Emit(OpCode::OP_PRINT, 1);

  // main 返回
  main_fn.Emit(OpCode::OP_RETURN, 1);

  // 运行主函数
  vm.Run(main_fn);
}


// ---------------- 带一个参数的函数调用自测 ----------------
void CallWithArgTest() {
  using namespace cilly;

  Function add1("add1", 1);  //创建函数add1(n)
  add1.SetLocalCount(1);     //规定变量一共一个

  int c1 = add1.AddConst(Value::Num(1));  // 把1加入常量池
  add1.Emit(OpCode::OP_LOAD_VAR, 1);      // 载入变量n
  add1.EmitI32(0, 1);   
  add1.Emit(OpCode::OP_CONSTANT, 1);      // 1
  add1.EmitI32(c1, 1);
  add1.Emit(OpCode::OP_ADD, 1);           // n + 1
  add1.Emit(OpCode::OP_RETURN, 1);        // 返回 n + 1

  Function main("main", 1);   //创建函数 main
  main.SetLocalCount(0);

  VM vm;

  int add1_id = vm.RegisterFunction(&add1);

  int c42 = main.AddConst(Value::Num(42)); // 压入实参 42
  main.Emit(OpCode::OP_CONSTANT, 1);
  main.EmitI32(c42, 1);

  main.Emit(OpCode::OP_CALL, 1);
  main.EmitI32(add1_id, 1);

  main.Emit(OpCode::OP_PRINT, 1);
  main.Emit(OpCode::OP_RETURN, 1);
  
  vm.Run(main);
}


int main() {
  ValueTest();
  StackTest();
  ChunkTest();
  FunctionTest();
  VMTest();
  VarTest();
  CallTest();
  CallWithArgTest();
}