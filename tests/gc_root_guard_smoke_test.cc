#include <gtest/gtest.h>

#include "gc/gc.h"

namespace cilly {

/*
  TestNode：一个最小的“可追踪对象”用于测试 GC。
  - 它只引用一个 child_
  - Trace 里把 child_ Mark 掉，模拟真实对象（如 list/dict/instance
  fields）持有引用
*/
class TestNode : public gc::GcObject {
 public:
  explicit TestNode(gc::GcObject* child = nullptr) : child_(child) {}
  void set_child(gc::GcObject* c) { child_ = c; }

  void Trace(gc::Collector& c) override {
    // 关键：对象图可达性传播就在这里发生
    // 如果 root 是 a，a.Trace 会 Mark(b)，b 就不会被 sweep
    c.Mark(child_);
  }

 private:
  gc::GcObject* child_ = nullptr;
};

TEST(gc_root_guard_smoke_test, UnreachableObjectsAreSwept) {
  gc::Collector gc;

  auto* a = gc.New<TestNode>();
  auto* b = gc.New<TestNode>();
  a->set_child(b);

  ASSERT_EQ(gc.object_count(), 2u);

  // 没有 root：a/b 都不可达，应被回收
  gc.Collect();

  EXPECT_EQ(gc.object_count(), 0u);
  EXPECT_EQ(gc.last_swept_count(), 2u);
}

TEST(gc_root_guard_smoke_test, RootGuardKeepsReachableGraphAlive) {
  gc::Collector gc;

  auto* a = gc.New<TestNode>();
  auto* b = gc.New<TestNode>();
  a->set_child(b);

  ASSERT_EQ(gc.object_count(), 2u);

  {
    // RootGuard 的意义：
    // a 只是 C++ 局部变量，如果此时 GC 触发，必须让 GC “看得见 a”
    // 通过 PushRoot(a)，a 成为 root；于是 a/b 都应该存活
    gc::RootGuard guard(gc, a);

    gc.Collect();

    EXPECT_EQ(gc.object_count(), 2u);
    EXPECT_EQ(gc.last_swept_count(), 0u);
  }

  // guard 析构后，root 消失：再次 collect 应全部回收
  gc.Collect();

  EXPECT_EQ(gc.object_count(), 0u);
  EXPECT_EQ(gc.last_swept_count(), 2u);
}

}  // namespace cilly
