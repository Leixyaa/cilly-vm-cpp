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

//VM 最小执行循环自测 
void VMTest() {
  using namespace cilly;

  Function fn("main", 0);
  int c0 = fn.AddConst(Value::Num(10));
  int c1 = fn.AddConst(Value::Num(20));

  fn.Emit(OpCode::OP_CONSTANT, 1);
  fn.EmitI32(c0, 1);
  fn.Emit(OpCode::OP_CONSTANT, 1);
  fn.EmitI32(c1, 1);
  fn.Emit(OpCode::OP_ADD, 1);
  fn.Emit(OpCode::OP_PRINT, 1);

  VM vm;
  vm.Run(fn);  // 期望控制台打印：30
}



int main() {
  ValueTest();
  StackTest();
  ChunkTest();
  FunctionTest();
  VMTest();
}