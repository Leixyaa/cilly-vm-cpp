#include <gtest/gtest.h>

#include "gc/gc.h"

namespace cilly {

class TestNode : public gc::GcObject {
 public:
  explicit TestNode(gc::GcObject* child = nullptr) : child_(child) {}
  void set_child(gc::GcObject* c) { child_ = c; }

  void Trace(gc::Collector& c) override { c.Mark(child_); }

 private:
  gc::GcObject* child_ = nullptr;
};

TEST(gc_collect_with_roots_smoke_test, ExternalRootsCallbackMarksObjects) {
  gc::Collector gc;

  auto* a = gc.New<TestNode>();
  auto* b = gc.New<TestNode>();
  a->set_child(b);

  ASSERT_EQ(gc.object_count(), 2u);

  // 不用 RootGuard，而是通过 callback 把 a 当作 root
  gc.CollectWithRoots([&](gc::Collector& c) { c.Mark(a); });

  // a 是 root，a->b 可达，所以都应存活
  EXPECT_EQ(gc.object_count(), 2u);
  EXPECT_EQ(gc.last_swept_count(), 0u);

  // 再来一次：不提供 roots，则都应回收
  gc.CollectWithRoots([](gc::Collector&) {});
  EXPECT_EQ(gc.object_count(), 0u);
  EXPECT_EQ(gc.last_swept_count(), 2u);
}

}  // namespace cilly
