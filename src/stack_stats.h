#ifndef CILLY_VM_CPP_STACK_STATS_H_
#define CILLY_VM_CPP_STACK_STATS_H_

#include <vector>
#include "value.h"

namespace cilly {

// 运行时栈，带统计信息：push/pop 次数、当前深度、最大深度。
class StackStats {
 public:
  StackStats();

  // 基本操作：压栈 / 弹栈 / 查看栈顶（Top 不弹出）
  void Push(const Value& v);
  Value Pop();
  const Value& Top() const;

  // 查询：当前深度 / 最大深度 / push 次数 / pop 次数
  int Depth() const;
  int MaxDepth() const;
  int PushCount() const;
  int PopCount() const;

  // 清空栈并重置统计数据
  void Clear();

  // 仅重置统计数据（不清空栈元素）
  void ResetStats();

 private:
  std::vector<Value> stack_;
  int push_count_ = 0;
  int pop_count_ = 0;
  int max_depth_ = 0;
};

} 

#endif  // CILLY_VM_CPP_STACK_STATS_H_
