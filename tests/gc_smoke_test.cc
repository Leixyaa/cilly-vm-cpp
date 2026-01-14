#include <memory>

#include "gtest/gtest.h"
#include "src/gc/gc.h"
#include "tests/run_script.h"

namespace cilly {
namespace {

class TestNode final : public gc::GcObject {
 public:
  explicit TestNode(GcObject* child = nullptr) : child_(child) {}

  void Trace(gc::Collector& c) override {
    if (child_)
      c.Mark(child_);
  }

 private:
  GcObject* child_ = nullptr;
};

TEST(GcSmokeTest, RootGuardKeepsTempRootAlive) {
  gc::Collector c;

  auto* b = c.New<TestNode>();
  auto* a = c.New<TestNode>(b);

  EXPECT_EQ(c.object_count(), 2u);

  {
    gc::RootGuard g(c, a);
    c.Collect();
  }

  // a 是 root，a->Trace 会标记 b，所以都活
  EXPECT_EQ(c.object_count(), 2u);
  EXPECT_EQ(c.last_swept_count(), 0u);
}

TEST(GcSmokeTest, CollectWithRootsCanMarkExternalRoots) {
  gc::Collector c;

  auto* b = c.New<TestNode>();
  auto* a = c.New<TestNode>(b);
  (void)c.New<TestNode>();  // garbage：不被引用，应该被 sweep 掉

  EXPECT_EQ(c.object_count(), 3u);

  c.CollectWithRoots([&](gc::Collector& gc) {
    gc.Mark(a);  // 外部 root：从这里开始可达的对象都要活
  });

  // 垃圾对象被清掉，a/b 存活
  EXPECT_EQ(c.object_count(), 2u);
  EXPECT_EQ(c.last_swept_count(), 1u);
  EXPECT_EQ(c.last_marked_count(), 2u);
}

TEST(GcSmokeTest, RunResultKeepsCollectorAlive) {
  auto heap = std::make_shared<gc::Collector>();
  (void)heap->New<TestNode>();

  test::RunResult r;
  r.gc_keepalive = heap;

  heap.reset();  // 如果 RunResult 没保活，这里 heap 就会析构并 FreeAll
  ASSERT_TRUE(r.gc_keepalive);
  EXPECT_EQ(r.gc_keepalive->object_count(), 1u);
}

}  // namespace
}  // namespace cilly
