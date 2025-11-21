#ifndef CILLY_VM_CPP_OPCODES_H_
#define CILLY_VM_CPP_OPCODES_H_

#include <cstdint>  // int32_t

namespace cilly {

//预留了int32_t的存储空间，但是用到的时候还是要进行显式转换
enum class OpCode : int32_t {
  //常量与栈操作
  OP_CONSTANT,   // 将常量池中的一个值压入栈
  OP_POP,        // 弹出栈顶元素
  OP_DUP,        // 复制栈顶元素

  //算术运算
  OP_ADD,        // 加法
  OP_SUB,        // 减法
  OP_MUL,        // 乘法
  OP_DIV,        // 除法
  OP_NEGATE,     // 取负

  //比较与逻辑
  OP_EQUAL,      // 判断相等（==）
  OP_NOT_EQUAL,  // 判断不等（!=）
  OP_GREATER,    // 大于（>）
  OP_LESS,       // 小于（<）
  OP_NOT,        // 逻辑非（!）

  //I/O 与控制
  OP_PRINT,      // 打印栈顶
  OP_RETURN,     // 程序返回

  //特殊
  OP_NOOP,       // 空操作，占位

  //变量系统
  OP_LOAD_VAR,
  OP_STORE_VAR,

  //帧调用
  OP_CALL,       // 调用另一个函数，操作数是函数 ID
};

}

#endif  // CILLY_VM_CPP_OPCODES_H_