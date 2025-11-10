#include "stack_stats.h"
#include <cassert>

namespace cilly {

StackStats::StackStats() : push_count_(0), pop_count_(0), max_depth_(0) {}

void StackStats::Push(const Value& v) {
  stack_.push_back(v);
  ++push_count_;
  if (static_cast<int>(stack_.size()) > max_depth_) {
    max_depth_ = static_cast<int>(stack_.size());
  }
}

Value StackStats::Pop() {
  assert(!stack_.empty() && "Pop on empty stack!");
  Value v = stack_.back();
  stack_.pop_back();
  ++pop_count_;
  return v;
}

const Value& StackStats::Top() const {
  assert(!stack_.empty() && "Top on empty stack!");
  return stack_.back();
}

int StackStats::Depth() const {
  return static_cast<int>(stack_.size());
}

int StackStats::MaxDepth() const {
  return max_depth_;
}

int StackStats::PushCount() const {
  return push_count_;
}

int StackStats::PopCount() const {
  return pop_count_;
}

void StackStats::ResetStats() {
  max_depth_ = 0;
  push_count_ = 0;
  pop_count_ = 0;
}

}  // namespace cilly
