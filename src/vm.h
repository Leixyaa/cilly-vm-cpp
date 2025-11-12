#ifndef CILLY_VM_CPP_VM_H_
#define CILLY_VM_CPP_VM_H_

#include <cstdint>
#include <string>
#include "function.h"
#include "opcodes.h"
#include "stack_stats.h"
#include "value.h"

namespace cilly {

// 最小可运行 VM:OP_CONSTANT / OP_ADD / OP_PRINT。
class VM {
 public:
  VM();

  // 运行入口：执行给定函数（从头到尾）。
  void Run(const Function& fn);

  // 便于调试：取内部栈的统计指标。
  int PushCount() const;
  int PopCount() const;
  int Depth() const;
  int MaxDepth() const;

 private:
  // 取下一条指令（从 code_ 读取，并自增 ip_）。
  int32_t ReadI32_(const Chunk& ch);

  // 取下一条操作数（32 位整型，自增 ip_）。
  int32_t ReadOpnd_(const Chunk& ch);

  // 执行一条指令；返回是否继续执行。
  bool Step_(const Function& fn);

 private:
  StackStats stack_;  // 运行时栈（带统计）
  int ip_ = 0;        // 指令指针（index into chunk.code_）
};

}  // namespace cilly

#endif  // CILLY_VM_CPP_VM_H_
