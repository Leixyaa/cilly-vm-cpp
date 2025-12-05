#ifndef CILLY_VM_CPP_VM_H_
#define CILLY_VM_CPP_VM_H_

#include <cstdint>
#include <string>
#include <vector>

#include "call_frame.h"
#include "function.h"
#include "opcodes.h"
#include "stack_stats.h"
#include "value.h"
#include "object.h"

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

  int RegisterFunction(const Function* fn); // 注册函数并且返回索引 
  
 private:
  // 取下一条指令（从 code_ 读取，并自增 ip_）。
  int32_t ReadI32_();

  // 取下一条操作数（32 位整型，自增 ip_）。
  int32_t ReadOpnd_();

  // 执行一条指令；返回是否继续执行。
  bool Step_();

  CallFrame& CurrentFrame();
  const CallFrame& CurrentFrame() const;


  StackStats stack_;  // 运行时栈（带统计）
  std::vector<CallFrame> frames_;
  std::vector<const Function*> functions_;

};

}  // namespace cilly

#endif  // CILLY_VM_CPP_VM_H_
