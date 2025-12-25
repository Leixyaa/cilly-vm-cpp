#ifndef CILLY_VM_CPP_OPCODES_H_
#define CILLY_VM_CPP_OPCODES_H_

#include <cstdint>  // int32_t

namespace cilly {

// 所有虚拟机指令的枚举。底层类型固定为 int32_t，方便统一存入 code_。
enum class OpCode : int32_t {
  // 常量与栈操作。
  OP_CONSTANT,  // 将常量池中的一个值压入栈。
  OP_POP,       // 弹出栈顶元素。
  OP_POPN,      // 弹出变量
  OP_DUP,       // 复制栈顶元素。

  // 算术运算。
  OP_ADD,     // 加法。
  OP_SUB,     // 减法。
  OP_MUL,     // 乘法。
  OP_DIV,     // 除法。
  OP_NEGATE,  // 取负。

  // 比较与逻辑。
  OP_EQ,         // 比较相等，结果是布尔值。
  OP_NOT_EQUAL,  // 判断不等（!=）。
  OP_GREATER,    // 大于（>）。
  OP_LESS,       // 小于（<）。
  OP_NOT,        // 逻辑非（!）。

  // 条件跳转。
  OP_JUMP,           // 无条件跳转。
  OP_JUMP_IF_FALSE,  // 栈顶为 false 时跳转。

  // I/O 与控制。
  OP_PRINT,   // 打印栈顶。
  OP_RETURN,  // 函数返回。

  // 特殊。
  OP_NOOP,  // 空操作，占位。

  // 变量系统。
  OP_LOAD_VAR,   // 按索引读取局部变量，压入栈。
  OP_STORE_VAR,  // 从栈顶弹出一个值，写入指定局部变量。

  // 函数调用。
  OP_CALL,   // 调用另一个函数，操作数是函数 ID。
  OP_CALLV,  // 运行时调用函数

  // List相关命令
  OP_LIST_NEW,   // 栈: ... -> ..., list
  OP_LIST_PUSH,  // 栈: ..., list, value -> ..., list
  // OP_LIST_GET,      // 栈: ..., list, index -> ..., element
  // OP_LIST_SET,      // 栈: ..., list, index, value -> ...
  OP_LIST_LEN,  // 栈: ..., list -> ..., length

  // Dict 相关指令
  OP_DICT_NEW,  // 栈: ...                -> ..., dict
  // OP_DICT_SET,    // 栈: ..., dict, key, value -> ...
  // OP_DICT_GET,    // 栈: ..., dict, key     -> ..., value
  OP_DICT_HAS,  // 栈: ..., dict, key     -> ..., bool

  OP_INDEX_GET,  // 将List和Dict合并分发处理
  OP_INDEX_SET,
};

}  // namespace cilly

#endif  // CILLY_VM_CPP_OPCODES_H_
