// cilly-vm-cpp
// Author: Leixyaa
// Date: 2024-11-06
// Description: Entry point for testing environment.

#include <iostream>
#include <chrono>

#include "chunk.h"
#include "function.h"
#include "opcodes.h"
#include "stack_stats.h"
#include "value.h"
#include "vm.h"
#include "bytecode_stream.h" 
#include "object.h"
#include "frontend/lexer.h"
#include "frontend/parser.h"
#include "frontend/generator.h"
#include "util/io.h"

// 注册原生函数
static void RegisterBuiltins(cilly::VM& vm) {
  using namespace cilly;

  // len(x)
  int i0 = vm.RegisterNative("len", 1, [](VM&, const Value* args, int argc) {
    assert(argc == 1);
    const Value& v = args[0];
    if (v.IsStr())   return Value::Num((double)v.AsStr().size());
    if (v.IsList())  return Value::Num((double)v.AsList()->Size());
    if (v.IsDict())  return Value::Num((double)v.AsDict()->Size());
    if (v.IsString())return Value::Num((double)v.AsString()->ToRepr().size());
    assert(false && "len() unsupported type");
    return Value::Null();
  });
  assert(i0 == 0);

  //str(x)
  int i1 = vm.RegisterNative("str", 1, [](VM&, const Value* args, int argc) {
    assert(argc == 1);
    const Value& v = args[0];
    if (v.IsStr()) return v;
    return Value::Str(v.ToRepr());
  });
  assert(i1 == 1);

  //type(x)
  int i2 = vm.RegisterNative("type", 1, [](VM&, const Value* args, int argc) {
    assert(argc == 1);
    const Value& v = args[0];
    if (v.IsNull()) return Value::Str("null");
    if (v.IsBool()) return Value::Str("bool");
    if (v.IsNum())  return Value::Str("number");
    if (v.IsStr() || v.IsString()) return Value::Str("str");
    if (v.IsList()) return Value::Str("list");
    if (v.IsDict()) return Value::Str("dict");
    if (v.IsObj())  return Value::Str("object");
    return Value::Str("unknown");
  });
  assert(i2 == 2);

  //abs(x)
  int i3 = vm.RegisterNative("abs", 1, [](VM&, const Value* args, int argc) {
    assert(argc == 1);
    assert(args[0].IsNum());
    double x = args[0].AsNum();
    return Value::Num(x < 0 ? -x : x);
  });
  assert(i3 == 3);

  //clock()
  int i4 = vm.RegisterNative("clock", 0, [](VM&, const Value*, int) {
    using clock = std::chrono::steady_clock;
    double s = std::chrono::duration_cast<std::chrono::duration<double>>(
                 clock::now().time_since_epoch()).count();
    return Value::Num(s);
  });
  assert(i4 == 4);
}





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


  std::cout << "---------------------------------------------\n";
}


// ---------------- 带一个参数的函数调用自测 ----------------
void CallWithArgTest() {
  using namespace cilly;

  std::cout << "带一个参数的函数调用自测:\n" << std::endl;

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
  main.Emit(OpCode::OP_RETURN, 1);         // 返回 42 + 1 = 42
  
  vm.Run(main);

  std::cout << "---------------------------------------------\n";
}


// ---------------- 相等命令自测 ----------------
void Eqtest() {
  using namespace cilly;
  
  std::cout << "相等命令自测:\n" << std::endl;
  
  Function main("main", 0);
  main.SetLocalCount(0);

  int c1 = main.AddConst(Value::Num(1));
  int c2 = main.AddConst(Value::Num(2));
  
  main.Emit(OpCode::OP_CONSTANT, 1);  // 1
  main.EmitI32(c1, 1);
  main.Emit(OpCode::OP_CONSTANT, 1);  // 1
  main.EmitI32(c1, 1);
  main.Emit(OpCode::OP_EQ, 1);        // 1 == 1
  main.Emit(OpCode::OP_PRINT, 1);     // true

  main.Emit(OpCode::OP_CONSTANT, 1);  // 1
  main.EmitI32(c1, 1);
  main.Emit(OpCode::OP_CONSTANT, 1);  // 2
  main.EmitI32(c2, 1);  
  main.Emit(OpCode::OP_EQ, 1);        // 1 == 2
  main.Emit(OpCode::OP_PRINT, 1);     // false

  main.Emit(OpCode::OP_RETURN, 1);    // 随便 return 一个值，这里返回最后一次比较结果

  VM vm;
  vm.Run(main);  

  std::cout << "---------------------------------------------\n";
}


void ForLoopTest() {
  using namespace cilly;
  std::cout << "for 循环字节码自测:\n" << std::endl;

  Function fn("for_test", 0);
  fn.SetLocalCount(1);  // local[0] 用作 i

  // 1. 常量：0,1,3
  // 2. 初始化 i = 0;
  // 3. 记录 loop_start 位置
  // 4. 条件 i < 3
  // 5. JUMP_IF_FALSE 跳到 end_label（占位 + 回填）
  // 6. 循环体：print(i)
  // 7. i = i + 1
  // 8. 无条件跳回 loop_start
  // 9. end_label: 回填 JUMP_IF_FALSE 的目标
  // 10. OP_RETURN

  int c0 = fn.AddConst(Value::Num(0));
  int c1 = fn.AddConst(Value::Num(1));
  int c3 = fn.AddConst(Value::Num(3));

  fn.Emit(OpCode::OP_CONSTANT, 1);
  fn.EmitI32(c0, 1);
  fn.Emit(OpCode::OP_STORE_VAR, 1);
  fn.EmitI32(0, 1);

  int loop_start = fn.CodeSize();
  fn.Emit(OpCode::OP_LOAD_VAR, 1);
  fn.EmitI32(0, 1);
  fn.Emit(OpCode::OP_CONSTANT, 1);
  fn.EmitI32(c3, 1);
  fn.Emit(OpCode::OP_LESS, 1);
  fn.Emit(OpCode::OP_JUMP_IF_FALSE, 1);
  int end_lable = fn.CodeSize();
  fn.EmitI32(0, 1);
  
  fn.Emit(OpCode::OP_LOAD_VAR, 1);
  fn.EmitI32(0, 1);
  fn.Emit(OpCode::OP_PRINT, 1);
  fn.Emit(OpCode::OP_POP, 1);
  fn.Emit(OpCode::OP_LOAD_VAR, 1);
  fn.EmitI32(0, 1);
  fn.Emit(OpCode::OP_CONSTANT, 1);
  fn.EmitI32(c1, 1);
  fn.Emit(OpCode::OP_ADD, 1);
  fn.Emit(OpCode::OP_STORE_VAR, 1);
  fn.EmitI32(0, 1);
  
  fn.Emit(OpCode::OP_JUMP, 1);
  fn.EmitI32(loop_start, 1);
  
  fn.PatchI32(end_lable, fn.CodeSize());

  fn.Emit(OpCode::OP_LOAD_VAR, 1);
  fn.EmitI32(0, 1);
  fn.Emit(OpCode::OP_RETURN, 1);

  VM vm;
  vm.Run(fn);

  std::cout << "For 循环自测 (预期 print: 0,1,2; return: 3)\n";
  std::cout << "---------------------------------------------\n";
}




// ---------------- 条件跳转命令自测 ----------------
void IfTest() {
  using namespace cilly;

  std::cout << "条件跳转命令自测:\n" << std::endl;

  Function fn("if_test", 0);
  fn.SetLocalCount(0);

  // 常量
  int c1 = fn.AddConst(Value::Num(1));
  int c2 = fn.AddConst(Value::Num(2));
  int c888 = fn.AddConst(Value::Num(888));
  int c999 = fn.AddConst(Value::Num(999));

  // 如果 (1 == 2)
  fn.Emit(OpCode::OP_CONSTANT, 1); fn.EmitI32(c1, 1);
  fn.Emit(OpCode::OP_CONSTANT, 1); fn.EmitI32(c2, 1);
  fn.Emit(OpCode::OP_EQ, 1);                      // 栈顶 = false

  // JUMP_IF_FALSE <else_label>
  fn.Emit(OpCode::OP_JUMP_IF_FALSE, 1);

  int if_else_pos = fn.CodeSize();
  fn.EmitI32(0, 1);

  // then: print(999)
  fn.Emit(OpCode::OP_CONSTANT, 1); fn.EmitI32(c999, 1);
  fn.Emit(OpCode::OP_PRINT, 1);

  // JUMP end
  fn.Emit(OpCode::OP_JUMP, 1);
  
  int if_end_pos = fn.CodeSize();
  fn.EmitI32(0, 1);

  fn.PatchI32(if_else_pos, fn.CodeSize());

  // else_label:
  // print(888)
  // ——此处就是 else_label 的位置——
  // 真实位置 = fn.CodeSize()
  fn.Emit(OpCode::OP_CONSTANT, 1); fn.EmitI32(c888, 1);
  fn.Emit(OpCode::OP_PRINT, 1);

  fn.PatchI32(if_end_pos, fn.CodeSize());

  // end_label:
  fn.Emit(OpCode::OP_RETURN, 1);

  VM vm;
  vm.Run(fn);

 std::cout << "---------------------------------------------\n";
}


// ---------------- odd/even 互递归自测 ----------------
void OddEvenTest() {
  using namespace cilly;
  std::cout << "odd/even 互递归自测:\n" << std::endl;

  // 1. 创建三个函数对象：odd, even, main
  Function odd_fn("odd", 1);      // 1 个参数：n
  Function even_fn("even", 1);    // 1 个参数：n
  Function main_fn("main", 0);    // 无参数

  // 每个函数至少有与参数个数一致的 locals
  odd_fn.SetLocalCount(1);   // locals_[0] = n
  even_fn.SetLocalCount(1);  // locals_[0] = n
  main_fn.SetLocalCount(0);

  // 2. 创建 VM，并注册可被调用的函数
  VM vm;
  int odd_id = vm.RegisterFunction(&odd_fn);
  int even_id = vm.RegisterFunction(&even_fn);

  // 3. 下面我们要分三部分填字节码：
  //    (1) even_fn 的函数体
  int c0 = even_fn.AddConst(Value::Num(0));
  int c1 = even_fn.AddConst(Value::Num(1));
  int c_true = even_fn.AddConst(Value::Bool(true));

  even_fn.Emit(OpCode::OP_LOAD_VAR, 1);  even_fn.EmitI32(0, 1);
  even_fn.Emit(OpCode::OP_CONSTANT, 1);  even_fn.EmitI32(c0, 1);
  even_fn.Emit(OpCode::OP_EQ, 1);
 
  // if(n == 0)
  even_fn.Emit(OpCode::OP_JUMP_IF_FALSE, 1);
  int if_else_pos = even_fn.CodeSize();  
  even_fn.EmitI32(0, 1);
  
  even_fn.Emit(OpCode::OP_CONSTANT, 1);  even_fn.EmitI32(c_true, 1);
  even_fn.Emit(OpCode::OP_RETURN, 1);
  
  even_fn.PatchI32(if_else_pos, even_fn.CodeSize());

  //else
  even_fn.Emit(OpCode::OP_LOAD_VAR, 1);  even_fn.EmitI32(0, 1);
  even_fn.Emit(OpCode::OP_CONSTANT, 1);  even_fn.EmitI32(c1, 1);
  even_fn.Emit(OpCode::OP_SUB, 1);
  even_fn.Emit(OpCode::OP_CALL, 1);
  even_fn.EmitI32(odd_id, 1);

  even_fn.Emit(OpCode::OP_RETURN, 1);



  //    (2) odd_fn 的函数体
  c0 = odd_fn.AddConst(Value::Num(0));
  c1 = odd_fn.AddConst(Value::Num(1));
  c_true = odd_fn.AddConst(Value::Bool(false));

  odd_fn.Emit(OpCode::OP_LOAD_VAR, 1);  odd_fn.EmitI32(0, 1);
  odd_fn.Emit(OpCode::OP_CONSTANT, 1);  odd_fn.EmitI32(c0, 1);
  odd_fn.Emit(OpCode::OP_EQ, 1);
 
  // if(n == 0)
  odd_fn.Emit(OpCode::OP_JUMP_IF_FALSE, 1);
  if_else_pos = odd_fn.CodeSize();  odd_fn.EmitI32(0, 1);
  
  odd_fn.Emit(OpCode::OP_CONSTANT, 1);
  odd_fn.EmitI32(c_true, 1);
  odd_fn.Emit(OpCode::OP_RETURN, 1);
  
  odd_fn.PatchI32(if_else_pos, odd_fn.CodeSize());

  //else
  odd_fn.Emit(OpCode::OP_LOAD_VAR, 1);  odd_fn.EmitI32(0, 1);
  odd_fn.Emit(OpCode::OP_CONSTANT, 1);  odd_fn.EmitI32(c1, 1);
  odd_fn.Emit(OpCode::OP_SUB, 1);
  odd_fn.Emit(OpCode::OP_CALL, 1);
  odd_fn.EmitI32(even_id, 1);

  odd_fn.Emit(OpCode::OP_RETURN, 1);

  //    (3) main_fn：调用 even(3) 和 odd(3)
  int c3 = main_fn.AddConst(Value::Num(3));

  main_fn.Emit(OpCode::OP_CONSTANT, 1);
  main_fn.EmitI32(c3, 1);
  main_fn.Emit(OpCode::OP_CALL, 1);
  main_fn.EmitI32(even_id, 1);
  main_fn.Emit(OpCode::OP_PRINT, 1);
  
  main_fn.Emit(OpCode::OP_CONSTANT, 1);
  main_fn.EmitI32(c3, 1);
  main_fn.Emit(OpCode::OP_CALL, 1);
  main_fn.EmitI32(odd_id, 1);
  main_fn.Emit(OpCode::OP_PRINT, 1);

 main_fn.Emit(OpCode::OP_RETURN, 1);

  // TODO: 填 even_fn 的字节码

  // TODO: 填 odd_fn 的字节码

  // TODO: 填 main_fn 的字节码

  // 4. 运行 main_fn
  vm.Run(main_fn);

 std::cout << "---------------------------------------------\n";
}

// ---------------- 比较指令自测 ----------------
void CompareTest() {
  using namespace cilly;
  std::cout << "补全比较指令自测:\n" << std::endl;

  Function fn("compare_test", /*arity*/0);
  fn.SetLocalCount(0);

  // 创建常量
  int c2 = fn.AddConst(Value::Num(2));
  int c3 = fn.AddConst(Value::Num(3));
  int cfalse = fn.AddConst(Value::Bool(false));

  // ----- print(3 > 2) -----
  fn.Emit(OpCode::OP_CONSTANT, 1);  fn.EmitI32(c3, 1);
  fn.Emit(OpCode::OP_CONSTANT, 1);  fn.EmitI32(c2, 1);
  fn.Emit(OpCode::OP_GREATER, 1);
  fn.Emit(OpCode::OP_PRINT, 1);

  // ----- print(3 < 2) -----
  fn.Emit(OpCode::OP_CONSTANT, 1);  fn.EmitI32(c3, 1);
  fn.Emit(OpCode::OP_CONSTANT, 1);  fn.EmitI32(c2, 1);
  fn.Emit(OpCode::OP_LESS, 1);
  fn.Emit(OpCode::OP_PRINT, 1);

  // ----- print(3 != 2) -----
  fn.Emit(OpCode::OP_CONSTANT, 1);  fn.EmitI32(c3, 1);
  fn.Emit(OpCode::OP_CONSTANT, 1);  fn.EmitI32(c2, 1);
  fn.Emit(OpCode::OP_NOT_EQUAL, 1);
  fn.Emit(OpCode::OP_PRINT, 1);

  // ----- print(!false) -----
  fn.Emit(OpCode::OP_CONSTANT, 1);  fn.EmitI32(cfalse, 1);
  fn.Emit(OpCode::OP_NOT, 1);
  fn.Emit(OpCode::OP_PRINT, 1);

  // return
  fn.Emit(OpCode::OP_RETURN, 1);

  VM vm;
  vm.Run(fn);
  std::cout << "---------------------------------------------\n";
}

//---------------- 二进制文件读写 ----------------
void StreamTest() {
  std::cout << "二进制文件读写..." << std::endl;
  using namespace cilly;
  // 写入测试
  {
    BytecodeWriter writer("test.bin"); // 会在目录下生成 test.bin
    writer.Write<int>(12345);                 // 写入整数
    writer.Write<double>(3.14159);            // 写入浮点数
    writer.WriteString("Hello C++");          // 写入字符串
  } // writer 析构，文件自动关闭

  // 读取测试
  {
    BytecodeReader reader("test.bin");
    int i = reader.Read<int>();
    double d = reader.Read<double>();
    std::string s = reader.ReadString();

    // 验证
    if (i == 12345 && d == 3.14159 && s == "Hello C++") {
      std::cout << "成功写入读取！\n";
    } else {
      std::cout << "失败，文件类型不匹配！\n";
      std::cout << "Read: " << i << ", " << d << ", " << s << "\n";
    }
  }
  std::cout << "---------------------------------------------\n";
}

void ValueSerializationTest() {
  std::cout << "值序列化测试...\n";
  
  // 准备数据：包含 Null, Bool, Num, Str 混合情况
  std::vector<cilly::Value> values;
  values.push_back(cilly::Value::Null());
  values.push_back(cilly::Value::Bool(true));
  values.push_back(cilly::Value::Num(123.456));
  values.push_back(cilly::Value::Str("C++ Variant Magic"));

  // 写入文件
  {
    cilly::BytecodeWriter writer("value_test.bin");
    // 手动模拟写入一个 vector 的过程
    writer.Write<int32_t>(values.size()); 
    for (const auto& v : values) {
      v.Save(writer); // 调用 Save
    }
  }

  // 读取文件
  {
    cilly::BytecodeReader reader("value_test.bin");
    int32_t count = reader.Read<int32_t>();
    
    if (count != values.size()) std::cout << "FAIL: 数量不匹配!\n";

    for (int i = 0; i < count; ++i) {
      cilly::Value loaded = cilly::Value::Load(reader); // 调用 Load
      
      // 验证是否相等
      if (loaded != values[i]) {
        std::cout << "FAIL: 同一索引 Value 不匹配 " << i << "\n";
        std::cout << "Expected: " << values[i].ToRepr() << "\n";
        std::cout << "Got: " << loaded.ToRepr() << "\n";
        return;
      }
    }
  }
  
  std::cout << "所有Value值都被正确存入!\n";
  std::cout << "---------------------------------------------\n";
}




void ChunkSerializationTest() {
  using namespace cilly;

  std::cout << "Chunk 序列化自测..." << std::endl;

  //  构造一个简单的 Function，并往里面填充一段可执行的字节码
  //  ((0 + 1) - 2) + 1，然后打印结果并 return。
  Function fn("main", 0);
  fn.SetLocalCount(0);

  // 常量池：0, 1, 2
  int c0 = fn.AddConst(Value::Num(0));
  int c1 = fn.AddConst(Value::Num(1));
  int c2 = fn.AddConst(Value::Num(2));

  // 生成字节码：
  // push 0
  fn.Emit(OpCode::OP_CONSTANT, 1);
  fn.EmitI32(c0, 1);

  // push 1
  fn.Emit(OpCode::OP_CONSTANT, 1);
  fn.EmitI32(c1, 1);

  // 0 + 1
  fn.Emit(OpCode::OP_ADD, 1);

  // push 2
  fn.Emit(OpCode::OP_CONSTANT, 1);
  fn.EmitI32(c2, 1);

  // (0 + 1) - 2
  fn.Emit(OpCode::OP_SUB, 1);

  // push 1
  fn.Emit(OpCode::OP_CONSTANT, 1);
  fn.EmitI32(c1, 1);

  // ((0 + 1) - 2) + 1
  fn.Emit(OpCode::OP_ADD, 1);

  // print 结果
  fn.Emit(OpCode::OP_PRINT, 1);

  // return
  fn.Emit(OpCode::OP_RETURN, 1);

  // 将 fn 对应的 Chunk 序列化到文件
  {
    BytecodeWriter writer("chunk_test.bin");
    fn.chunk().Save(writer);
  }

  // 从文件中反序列化出一个新的 Chunk
  BytecodeReader reader("chunk_test.bin");
  Chunk loaded = Chunk::Load(reader);

  // 依次对比：代码长度、常量池长度
  if (fn.CodeSize() != loaded.CodeSize()) {
    std::cout << "错误：代码长度不一致" << std::endl;
    return;
  }

  if (fn.ConstSize() != loaded.ConstSize()) {
    std::cout << "错误：常量数量不一致" << std::endl;
    return;
  }

  // 对比每一个字节码指令/操作数
  for (int i = 0; i < fn.CodeSize(); ++i) {
    if (fn.chunk().CodeAt(i) != loaded.CodeAt(i)) {
      std::cout << "错误：第 " << i << " 个字节码不一致" << std::endl;
      return;
    }
  }

  // 对比每一个常量值
  for (int i = 0; i < fn.ConstSize(); ++i) {
    if (fn.chunk().ConstAt(i) != loaded.ConstAt(i)) {
      std::cout << "错误：第 " << i << " 个常量不一致" << std::endl;
      return;
    }
  }

  // 对比每一个行号
  for (int i = 0; i < fn.CodeSize(); ++i) {
    if (fn.chunk().LineAt(i) != loaded.LineAt(i)) {
      std::cout << "错误：第 " << i << " 个行号不一致" << std::endl;
      return;
    }
  }

  std::cout << "Chunk 序列化通过！" << std::endl;
}


void FunctionSerializationTest() {
  using namespace cilly;

  std::cout << "Function 序列化自测..." << std::endl;

  // 构造一个简单的函数：((0 + 1) - 2) + 1，然后 print 再 return
  Function fn("main", 0);
  fn.SetLocalCount(0);

  // 常量池：0, 1, 2
  int c0 = fn.AddConst(Value::Num(0));
  int c1 = fn.AddConst(Value::Num(1));
  int c2 = fn.AddConst(Value::Num(2));

  // 生成字节码：
  // push 0
  fn.Emit(OpCode::OP_CONSTANT, 1);
  fn.EmitI32(c0, 1);

  // push 1
  fn.Emit(OpCode::OP_CONSTANT, 1);
  fn.EmitI32(c1, 1);

  // 0 + 1
  fn.Emit(OpCode::OP_ADD, 1);

  // push 2
  fn.Emit(OpCode::OP_CONSTANT, 1);
  fn.EmitI32(c2, 1);

  // (0 + 1) - 2
  fn.Emit(OpCode::OP_SUB, 1);

  // push 1
  fn.Emit(OpCode::OP_CONSTANT, 1);
  fn.EmitI32(c1, 1);

  // ((0 + 1) - 2) + 1
  fn.Emit(OpCode::OP_ADD, 1);

  // 打印结果
  fn.Emit(OpCode::OP_PRINT, 1);

  // 返回
  fn.Emit(OpCode::OP_RETURN, 1);

  // 将 Function 序列化到二进制文件
  {
    BytecodeWriter writer("function_test.bin");
    fn.Save(writer);
  }

  // 从二进制文件中反序列化得到一个新的 Function
  BytecodeReader reader("function_test.bin");
  Function loaded = Function::Load(reader);

  // 检查函数元信息是否一致（名称 / 形参个数 / 局部变量个数）
  if (fn.name() != loaded.name()) {
    std::cout << "错误：函数名称不一致" << std::endl;
    return;
  }

  if (fn.arity() != loaded.arity()) {
    std::cout << "错误：形参数量不一致" << std::endl;
    return;
  }

  if (fn.LocalCount() != loaded.LocalCount()) {
    std::cout << "错误：局部变量数量不一致" << std::endl;
    return;
  }

  // 进一步检查底层 Chunk：常量池 + 指令流 + 行号信息
  const Chunk& ch0 = fn.chunk();
  const Chunk& ch1 = loaded.chunk();

  if (ch0.ConstSize() != ch1.ConstSize()) {
    std::cout << "错误：常量池大小不一致" << std::endl;
    return;
  }

  if (ch0.CodeSize() != ch1.CodeSize()) {
    std::cout << "错误：指令序列长度不一致" << std::endl;
    return;
  }

  // 对比每一个常量
  for (int i = 0; i < ch0.ConstSize(); ++i) {
    if (ch0.ConstAt(i) != ch1.ConstAt(i)) {
      std::cout << "错误：常量池内容不一致，索引 = " << i << std::endl;
      return;
    }
  }

  // 对比每一条指令
  for (int i = 0; i < ch0.CodeSize(); ++i) {
    if (ch0.CodeAt(i) != ch1.CodeAt(i)) {
      std::cout << "错误：指令序列不一致，索引 = " << i << std::endl;
      return;
    }
  }

  // 对比每一条行号（这里假设两边 CodeSize 一致）
  for (int i = 0; i < ch0.CodeSize(); ++i) {
    if (ch0.LineAt(i) != ch1.LineAt(i)) {
      std::cout << "错误：行号信息不一致，索引 = " << i << std::endl;
      return;
    }
  }

  std::cout << "Function 序列化通过！" << std::endl;
}

void VarValueSemanticsTest() {
  using namespace cilly;
  std::cout << "变量值语义自测...\n";

  Function fn("main", 0);
  fn.SetLocalCount(2);  // local[0] = a, local[1] = b

  // 常量池：1 和 2
  int c1 = fn.AddConst(Value::Num(1));
  int c2 = fn.AddConst(Value::Num(2));

  // var a = 1;
  fn.Emit(OpCode::OP_CONSTANT, 1);  // push 1
  fn.EmitI32(c1, 1);
  fn.Emit(OpCode::OP_STORE_VAR, 1); // a = 1
  fn.EmitI32(0, 1);                 // local[0]

  // var b = a;
  fn.Emit(OpCode::OP_LOAD_VAR, 1);  // push a
  fn.EmitI32(0, 1);
  fn.Emit(OpCode::OP_STORE_VAR, 1); // b = a
  fn.EmitI32(1, 1);                 // local[1]

  // b = 2;
  fn.Emit(OpCode::OP_CONSTANT, 1);  // push 2
  fn.EmitI32(c2, 1);
  fn.Emit(OpCode::OP_STORE_VAR, 1); // b = 2
  fn.EmitI32(1, 1);                 // local[1]

  // print(a);
  fn.Emit(OpCode::OP_LOAD_VAR, 1);  // push a
  fn.EmitI32(0, 1);
  fn.Emit(OpCode::OP_PRINT, 1);     // 打印 a（期望：1）

  // return
  fn.Emit(OpCode::OP_RETURN, 1);

  VM vm;
  vm.Run(fn);

  std::cout << "变量值语义自测结束（预期输出：1）\n";
  std::cout << "---------------------------------------------\n";
}

void ObjSmokeTest() {
  using namespace cilly;
  std::cout << "测试ObjList ObjString 是否成功定义\n";
  std::cout << "测试ObjList\n";
  auto obj = std::make_shared<ObjList>();
  obj -> Push(Value::Num(1));
  obj -> Push(Value::Str("hello"));
  obj -> Push(Value::Bool(true));
  std::cout << obj -> ToRepr() << std::endl;

  Value v1 = Value::Obj(obj);
  std::cout << v1.ToRepr() << std::endl;


  std::cout << "测试ObjString\n";
  auto obj_string = std::make_shared<ObjString>();
  obj_string -> Set("hello");
  std::cout << obj_string -> ToRepr() << std::endl;
  Value v2 = Value::Obj(obj_string);
  std::cout << v2.ToRepr() << std::endl;


  std::cout << "测试ObjDict\n";
  std::unordered_map<std::string, Value> index;
  index["a"] = Value::Num(1);   // a : 1
  index["b"] = Value::Num(2);   // b : 2
  auto obj_dict = std::make_shared<ObjDict>(index);  // 存入ObjDict
  obj_dict->Set("c", Value::Num(3));  // c : 3
  if (obj_dict->Has("a")) {   // 查找 a
    std::cout << "存在 a" << std::endl;
  }
  if (!obj_dict->Has("d")) {  // 查找 d
    std::cout << "不存在 d" << std::endl;
  }
  std::cout << (obj_dict->Get("a")).ToRepr() << std::endl;   // 返回 a 关键字的值
  std::cout << (obj_dict->Get("d")).ToRepr() << std::endl;   // 返回 d 关键字的值(Null)
  obj_dict->Erase("a");       // 删除 a
  if (!obj_dict->Has("a")) { 
    std::cout << "不存在 a" << std::endl;
  }
  std::cout << obj_dict->Size() << std::endl;  // 返回长度 2
  std::cout << obj_dict->ToRepr() << std::endl;
  std::cout << "---------------------------------------------\n";
}




void ListOpcodeTest() {
  using namespace cilly;
  std::cout << "List 指令自测:\n";

  VM vm;

  Function fn("main", 0);
  fn.SetLocalCount(1);

  int c1 = fn.AddConst(Value::Num(1));
  int c2 = fn.AddConst(Value::Num(2));
  int c0 = fn.AddConst(Value::Num(0));

  fn.Emit(OpCode::OP_LIST_NEW, 1);
  fn.Emit(OpCode::OP_STORE_VAR, 1);
  fn.EmitI32(0, 1);
    
  // 推入1，2
  fn.Emit(OpCode::OP_LOAD_VAR, 1);
  fn.EmitI32(0, 1);
  fn.Emit(OpCode::OP_CONSTANT, 1);
  fn.EmitI32(c1, 1);
  fn.Emit(OpCode::OP_LIST_PUSH, 1);
  fn.Emit(OpCode::OP_CONSTANT, 1);
  fn.EmitI32(c2, 1);
  fn.Emit(OpCode::OP_LIST_PUSH, 1);
  
  // 显示当前列表长度
  fn.Emit(OpCode::OP_LIST_LEN, 1);
  fn.Emit(OpCode::OP_PRINT, 1);
  fn.Emit(OpCode::OP_POP, 1);

  // 将索引为0的值改为2  1->2
  fn.Emit(OpCode::OP_LOAD_VAR,1);
  fn.EmitI32(0, 1);
  fn.Emit(OpCode::OP_CONSTANT, 1);
  fn.EmitI32(c0, 1);
  fn.Emit(OpCode::OP_CONSTANT, 1);
  fn.EmitI32(c2, 1);
  fn.Emit(OpCode::OP_INDEX_SET, 1);

  // 获取索引为0的值打印
  fn.Emit(OpCode::OP_LOAD_VAR, 1);
  fn.EmitI32(0, 1);
  fn.Emit(OpCode::OP_CONSTANT, 1);
  fn.EmitI32(c0, 1);  
  fn.Emit(OpCode::OP_INDEX_GET, 1);
  fn.Emit(OpCode::OP_PRINT, 1);
  

  fn.Emit(OpCode::OP_RETURN, 1);
  
  vm.Run(fn);

  std::cout << "List 指令自测(预期结果：print: 2, 2;  return: 2)\n";
  std::cout << "---------------------------------------------\n";
}



void DictOpcodeTest() {
  using namespace cilly;
  std::cout << "Dict 指令自测:" << std::endl;

  Function fn("main", 0);
  fn.SetLocalCount(1);

  int c1 = fn.AddConst(Value::Num(1));
  int c2 = fn.AddConst(Value::Num(2));
  int c_a = fn.AddConst(Value::Str("a"));
  int c_b = fn.AddConst(Value::Str("b"));
  int c_c = fn.AddConst(Value::Str("c"));
  
  // 创建新字典并存入变量表
  fn.Emit(OpCode::OP_DICT_NEW, 1);
  fn.Emit(OpCode::OP_STORE_VAR, 1);
  fn.EmitI32(0, 1);

  // 设置 a = 1，b = 2
  fn.Emit(OpCode::OP_LOAD_VAR, 1);
  fn.EmitI32(0, 1);
  fn.Emit(OpCode::OP_DUP, 1);
  fn.Emit(OpCode::OP_CONSTANT, 1);
  fn.EmitI32(c_a, 1);
  fn.Emit(OpCode::OP_CONSTANT, 1);
  fn.EmitI32(c1, 1);
  fn.Emit(OpCode::OP_INDEX_SET, 1);
  fn.Emit(OpCode::OP_CONSTANT, 1);
  fn.EmitI32(c_b, 1);
  fn.Emit(OpCode::OP_CONSTANT, 1);
  fn.EmitI32(c2, 1);
  fn.Emit(OpCode::OP_INDEX_SET, 1);
  
  // 打印 a 
  fn.Emit(OpCode::OP_LOAD_VAR, 1);
  fn.EmitI32(0, 1);
  fn.Emit(OpCode::OP_CONSTANT, 1);
  fn.EmitI32(c_a, 1);
  fn.Emit(OpCode::OP_INDEX_GET, 1);
  fn.Emit(OpCode::OP_PRINT, 1);
  fn.Emit(OpCode::OP_POP, 1);
  
  // 把 a 改成 2 并且打印
  fn.Emit(OpCode::OP_LOAD_VAR, 1);
  fn.EmitI32(0, 1);
  fn.Emit(OpCode::OP_DUP, 1);
  fn.Emit(OpCode::OP_CONSTANT, 1);
  fn.EmitI32(c_a, 1);
  fn.Emit(OpCode::OP_CONSTANT, 1);
  fn.EmitI32(c2, 1);
  fn.Emit(OpCode::OP_INDEX_SET, 1);
  fn.Emit(OpCode::OP_CONSTANT, 1);
  fn.EmitI32(c_a, 1);
  fn.Emit(OpCode::OP_INDEX_GET, 1);
  fn.Emit(OpCode::OP_PRINT, 1);
  fn.Emit(OpCode::OP_POP, 1);

  // 查找c
  fn.Emit(OpCode::OP_LOAD_VAR, 1);
  fn.EmitI32(0, 1);
  fn.Emit(OpCode::OP_CONSTANT, 1);
  fn.EmitI32(c_c, 1);
  fn.Emit(OpCode::OP_DICT_HAS, 1);
  fn.Emit(OpCode::OP_PRINT, 1);
  
  fn.Emit(OpCode::OP_RETURN, 1);
  
  VM vm;
  vm.Run(fn);

  std::cout << "Dict 指令自测 (预期结果：print: 1, 2,false;  return: false)\n";
  std::cout << "---------------------------------------------\n";
}



void LexerSmokeTest() {
  using namespace cilly;

  std::string source = "var _x = 1; print _x; // comment\n";
  Lexer lexer(source);
  auto tokens = lexer.ScanAll();

  std::cout << "Lexer 自测：token 数量 = " << tokens.size() << std::endl;
  if (!tokens.empty()) {
    std::cout << "第一个 token kind (int) = "
              << static_cast<int>(tokens[0].kind)
              << ", lexeme = \"" << tokens[0].lexeme << "\""
              << std::endl;
  }
  std::cout << "---------------------------------------------\n";
}



void ParserExprSmokeTest() {
  using namespace cilly;

  std::cout << "Parser 表达式自测:\n";

  // 源码字符串（就是“自测输入”）
  std::string source =
      "print 1 + 2 * 3;\n"
      "print (1 + 2) * 3;\n"
      "var x = 10;\n"
      "print x + 5 * 2;\n"
      "print -x + 3;\n";

  // 先词法分析 → tokens
  Lexer lexer(source);
  std::vector<Token> tokens = lexer.ScanAll();

  // 再语法分析 → AST（语句列表）
  Parser parser(std::move(tokens));
  std::vector<StmtPtr> program = parser.ParseProgram();

  // 打印简单信息确认解析成功
  std::cout << "解析成功，语句条数 = " << program.size() << "\n";
  std::cout << "---------------------------------------------\n";
}




void FrontendEndToEndTest() {
  using namespace cilly;

  std::cout << "前端→VM 全链路自测:\n";
  
  std::string source =
  // A. 回归：变量/赋值/四则
  "var x = 1;\n"
  "print x;\n"
  "x = x + 1;\n"
  "print x;\n"
  "x = x * 10;\n"
  "print x;\n"
  
  // B. List 字面量
  "print [];\n"
  "print [1, 2, 3];\n"
  "print [x, x + 1, x * 2, (x - 3) / 7];\n"

  // C. Dict 字面量（key 必须是字符串）
  "print {};\n"
  "print {\"a\": 1};\n"
  "print {\"a\": x, \"b\": x + 1, \"c\": x * 2};\n"

  // D. Bool / Null
  "print true;\n"
  "print false;\n"
  "print null;\n"
  "print [true, null, false];\n"
  "print {\"t\": true, \"n\": null};\n"

  // E. 组合：dict value 是 list
  "print {\"nums\": [x, x + 1, x + 2], \"double\": x * 2};\n"

  // F. 索引功能
  "var a = [10, 20, 30];\n"
  "print a[0];\n"
  "print a[2];\n"
  "var d = {\"a\": 1, \"b\": 2};\n"
  "print d[\"a\"];\n"
  "print d[\"b\"];\n"
  "print {\"nums\": a}[\"nums\"][1];\n";



  //词法分析
  Lexer lexer(source);
  std::vector<Token>tokens_ = lexer.ScanAll();

  //语法分析
  Parser parser(tokens_);
  std::vector<StmtPtr>program = parser.ParseProgram();
  
  //生成字节码
  Generator generator;
  Function main = generator.Generate(program);
  
  VM vm;
  vm.Run(main);
  std::cout << "---------------------------------------------\n";
}


void FrontendEndToEndBlockTest() {
  using namespace cilly;

  std::cout << "前端→VM 全链路自测:\n";
  
  std::string source =
  "var a = [10, 20, 30];\n"
  "a[1] = 99;\n"
  "print a;\n"                 // [10, 99, 30]

  "var d = {\"x\": 1};\n"
  "d[\"x\"] = 42;\n"
  "print d;\n"                 // { "x" : 42 }
  "print d[\"x\"];\n"          // 42

  "var x = 1;\n"
  "if (x == 0) print 0;\n"
  "else if (x == 1) print 1;\n"
  "else print 2;\n"            // 1

  "var i = 0;\n"
  "while (i < 6) {\n"
  "  i = i + 1;\n"
  "  if (i == 3) continue;\n"
  "  if (i == 5) break;\n"
  "  print i;\n"
  "}\n"                        // 1 2 4

  "var j = 0;\n"
  "for (j = 0; j < 6; j = j + 1) {\n"
  "  if (j == 2) continue;\n"
  "  if (j == 4) break;\n"
  "  print j;\n"
  "}\n"                        // 0 1 3

  "var outer = 0;\n"
  "while (outer < 2) {\n"
  "  var inner = 0;\n"
  "  while (inner < 3) {\n"
  "    inner = inner + 1;\n"
  "    if (inner == 2) continue;\n"
  "    break;\n"
  "  }\n"
  "  print outer;\n"
  "  outer = outer + 1;\n"
  "}\n";                       // 0 1



  //词法分析
  Lexer lexer(source);
  std::vector<Token>tokens_ = lexer.ScanAll();

  //语法分析
  Parser parser(tokens_);
  std::vector<StmtPtr>program = parser.ParseProgram();
  
  //生成字节码
  Generator generator;
  Function main = generator.Generate(program);
  
  VM vm;
  vm.Run(main);
  std::cout << "---------------------------------------------\n";
}



void FrontendETESmokeTest() {
  using namespace cilly;

  std::cout << "前端→VM 全链路自测:\n";
  
  /*std::string source = ReadFileToString("D:/dev/cilly-vm-cpp/cilly_vm_cpp/file.txt");      */                                                   
  

std::string source = R"(
var x = 1;
print(x);

{
  var x = 2;
  var y = x + 3;
  print(x);
  print(y);
}

print(x);

{
  var z = 10;
  print(z);
  print(x);
}

print(x);
)";



  //词法分析
  Lexer lexer(source);
  std::vector<Token>tokens_ = lexer.ScanAll();

  //语法分析
  Parser parser(tokens_);
  std::vector<StmtPtr>program = parser.ParseProgram();
  
  //生成字节码
  Generator generator;
  Function main = generator.Generate(program);
  
  VM vm;

  for (const auto& fnptr : generator.Functions()) {
    vm.RegisterFunction(fnptr.get());  // 注册所有编译的函数
  }

  vm.Run(main);
  std::cout << "---------------------------------------------\n";
}


void ScopeAllFeatureSmokeTest() {
  using namespace cilly;

  std::cout << "===== Scope / Block / Loop 全覆盖冒烟测试 =====\n";

  std::string source = R"(

  // ---------- 1. 基础变量 ----------
  var x = 1;
  print x;        // 1

  // ---------- 2. block shadow ----------
  {
    var x = 2;
    var y = x + 10;
    print x;      // 2
    print y;      // 12
  }

  print x;        // 1（外层 x 不应被污染）

  // ---------- 3. if / else 作用域 ----------
  if (x == 1) {
    var a = 100;
    print a;      // 100
  } else {
    var a = 200;
    print a;
  }

  // ---------- 4. while + continue ----------
  var i = 0;
  while (i < 5) {
    i = i + 1;
    var t = i * 10;

    if (i == 2) continue;

    print t;      // 10, 30, 40 , 50
  }

  // ---------- 5. while + break ----------
  var j = 0;
  while (true) {
    j = j + 1;
    var k = j + 100;

    if (j == 3) break;

    print k;      // 101, 102
  }

  print j;        // 3

  // ---------- 6. for loop scope ----------
  for (var m = 0; m < 5; m = m + 1) {
    var inner = m * 2;

    if (m == 1) continue;
    if (m == 3) break;

    print inner;  // 0, 4
  }

  // ---------- 7. 嵌套 while + continue + break ----------
  var outer = 0;
  while (outer < 2) {
    var inner = 0;

    while (inner < 3) {
      inner = inner + 1;
      var tmp = outer * 10 + inner;

      if (inner == 2) continue;
      break;
    }

    print outer;   // 0, 1
    outer = outer + 1;
  }

  print outer;     // 2

  )";

  // 词法分析
  Lexer lexer(source);
  std::vector<Token> tokens = lexer.ScanAll();

  // 语法分析
  Parser parser(tokens);
  std::vector<StmtPtr> program = parser.ParseProgram();

  // 生成字节码
  Generator generator;
  Function main_fn = generator.Generate(program);

  // 执行
  VM vm;
  vm.Run(main_fn);

  std::cout << "===== Scope 冒烟测试结束 =====\n";
}


void CallFunctionSmokeTest() {
  using namespace cilly;

  std::cout << "===== Return 冒烟测试 =====\n";

  /* std::string source = ReadFileToString("D:/dev/cilly-vm-cpp/cilly_vm_cpp/file.txt");          */ 
std::string source = R"(
print "BEGIN";
print 111;

fun add(a, b) { return a + b; }
print add(1, 2);
print add(20, 22);

fun sub(a, b) { return a - b; }
print sub(10, 7);

fun abs1(x) {
  if (x < 0) { return 0 - x; }
  else { return x; }
}
print abs1(9);
print abs1(0 - 9);

fun fact(n) {
  if (n < 2) { return 1; }
  return n * fact(n - 1);
}
print fact(5);

print add(add(1, 2), add(3, 4));
print add(fact(3), fact(4));

fun shadow(x) {
  var y = x + 1;
  {
    var y = 100;
  }
  return y;
}
print shadow(9);

fun sum_skip3(n) {
  var i = 0;
  var s = 0;
  while (i < n) {
    i = i + 1;
    if (i == 3) { continue; }
    if (i == 8) { break; }
    s = s + i;
  }
  return s;
}
print sum_skip3(100);

fun list_demo(a) {
  var L = [a, a + 1, a + 2];
  L[1] = L[1] + 10;
  return L[0] + L[1] + L[2];
}
print list_demo(1);

fun dict_demo(dummy) {
  var D = { "a": 1, "b": 2 };
  D["c"] = D["a"] + D["b"];
  return D["c"];
}
print dict_demo(0);

fun for_demo(n) {
  var s = 0;
  for (var i = 0; i < n; i = i + 1) {
    s = s + i;
  }
  return s;
}
print for_demo(5);

print 999;
print "END";
)";



  // 词法分析
  Lexer lexer(source);
  std::vector<Token> tokens = lexer.ScanAll();

  // 语法分析
  Parser parser(tokens);
  std::vector<StmtPtr> program = parser.ParseProgram();

  // 生成字节码
  Generator generator;
  Function main_fn = generator.Generate(program);

  // 执行
  VM vm;
  RegisterBuiltins(vm);
  for (const auto& fnptr : generator.Functions()) {
    vm.RegisterFunction(fnptr.get());  // 注册所有编译的函数
  }

  vm.Run(main_fn);

  std::cout << "===== Scope 冒烟测试结束 =====\n";
}

void NativeFunctionSmokeTest() {
  using namespace cilly;

  std::cout << "===== Full Smoke Test (Native + User Functions) =====\n";

  std::string source = R"(
print "BEGIN_FULL";

print len([1, 2, 3]);
print str(123);
print type({ "a": 1 });
print abs(0 - 9);
print clock();

fun add(a, b) { return a + b; }
print add(1, 2);
print add(20, 22);

fun fact(n) {
  if (n < 2) { return 1; }
  return n * fact(n - 1);
}
print fact(5);

fun isEven(n) {
  if (n == 0) return true;
  return isOdd(n - 1);
}
fun isOdd(n) {
  if (n == 0) return false;
  return isEven(n - 1);
}
print isEven(10);
print isOdd(10);
print isEven(7);
print isOdd(7);

fun shadow(x) {
  var y = x + 1;
  {
    var y = 100;
  }
  return y;
}
print shadow(9);

fun sum_skip3(n) {
  var i = 0;
  var s = 0;
  while (i < n) {
    i = i + 1;
    if (i == 3) { continue; }
    if (i == 8) { break; }
    s = s + i;
  }
  return s;
}
print sum_skip3(100);

fun list_demo(a) {
  var L = [a, a + 1, a + 2];
  L[1] = L[1] + 10;
  return L[0] + L[1] + L[2];
}
print list_demo(1);

fun dict_demo(dummy) {
  var D = { "a": 1, "b": 2 };
  D["c"] = D["a"] + D["b"];
  return D["c"];
}
print dict_demo(0);

print 999;
print "END_FULL";
)";

  Lexer lexer(source);
  std::vector<Token> tokens = lexer.ScanAll();

  Parser parser(tokens);
  std::vector<StmtPtr> program = parser.ParseProgram();

  Generator generator;
  Function main_fn = generator.Generate(program);

  VM vm;

  // builtins 必须先注册，占据 0..4
  RegisterBuiltins(vm);


  for (const auto& fnptr : generator.Functions()) {
    vm.RegisterFunction(fnptr.get());
  }

  vm.Run(main_fn);

  std::cout << "===== Full Smoke Test END =====\n";
}

void CallVirtualFunctionTest() {
  using namespace cilly;

  std::cout << "===== CallVirtualFunctionTest BEGIN =====\n";

  //  不要用 // 注释，避免 lexer 不支持
  std::string source = R"(
print "BEGIN_CALLV";

fun add(a, b) { return a + b; }

var f = add;
print f(1, 2);

var o = { "m": add };
print o["m"](20, 22);

print add(3, 4);

print len([1, 2, 3]);
print abs(0 - 9);
print clock();

print "END_CALLV";
)";

  // 词法分析
  Lexer lexer(source);
  std::vector<Token> tokens = lexer.ScanAll();

  // 语法分析
  Parser parser(tokens);
  std::vector<StmtPtr> program = parser.ParseProgram();

  // 生成字节码
  Generator generator;
  Function main_fn = generator.Generate(program);

  // VM 执行
  VM vm;

  //  必须：先注册 builtin（如果你已经做了 builtin index 前缀）
  // 把这个函数名改成你项目里实际的：RegisterBuiltins / RegisterNativeBuiltins ...
  RegisterBuiltins(vm);

  // 再注册用户函数（顺序必须与 Generator::Functions() 一致）
  for (const auto& fnptr : generator.Functions()) {
    vm.RegisterFunction(fnptr.get());
  }

  vm.Run(main_fn);

  std::cout << "===== CallVirtualFunctionTest END =====\n";
}



void RunUnitTests() {
  ValueTest();
  StackTest();
  ChunkTest();
  FunctionTest();
}

void RunVMTests() {
  VMTest();
  VarTest();
  CallTest();
  CallWithArgTest();
  Eqtest();
  ForLoopTest();
  IfTest();
  OddEvenTest();
  CompareTest();
}

void RunOpcodeTests() {
  ListOpcodeTest();
  DictOpcodeTest();
}

void RunFrontendTests() {
  LexerSmokeTest();
  ParserExprSmokeTest();
}

void RunEndToEndTests() {
  FrontendEndToEndTest();
  FrontendEndToEndBlockTest();
  FrontendETESmokeTest();
  ScopeAllFeatureSmokeTest();
  CallFunctionSmokeTest();
}

void NativeFunctionTest() {
  NativeFunctionSmokeTest();
  CallVirtualFunctionTest();
  CallVirtualFunctionTest();
}





int main() {
  std::cout << "=== cilly-vm-cpp test runner ===\n";
  NativeFunctionTest();
  /*RunEndToEndTests();*/
  // RunFrontendTests();
  // RunOpcodeTests();
  // RunVMTests();
  // RunUnitTests();

  return 0;
}
