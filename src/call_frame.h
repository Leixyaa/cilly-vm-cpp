#ifndef CILLY_VM_CPP_CALL_FRAME_H_
#define CILLY_VM_CPP_CALL_FRAME_H_

#include <cstdint>
#include "function.h"

namespace cilly {

// 表示一次函数调用的执行上下文。
// 仅保存最基本的信息：
// - 当前执行的函数（fn）
// - 指令指针（ip）
// - 返回地址（ret_ip）
// - 局部变量数量由 fn.LocalCount() 决定
struct CallFrame {
  const Function* fn = nullptr;  // 当前执行的函数
  int ip = 0;                    // 指属于 fn 的指令指针
  int ret_ip = -1;               // 返回地址（返回后跳回的位置）

  std::vector<Value> locals_;     // 每个调用帧自己的变量表
};

}  // namespace cilly

#endif  // CILLY_VM_CPP_CALL_FRAME_H_
