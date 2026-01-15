#include "gtest/gtest.h"

// run_script.h 里 forward declare 了 Collector，为了访问统计接口要 include 定义
#include "gc/gc.h"
#include "tests/run_script.h"

namespace cilly {
namespace {

TEST(GcVmAutoCollectSmokeTest, AutoGcRunsDuringExecutionWithoutExplicitCall) {
  // 设计：
  // - 循环创建大量临时 list，让 object_count 快速超过阈值
  // - 不调用 __gc_collect()
  // - 期待 VM 的自动 GC 至少发生一次 sweep
  //
  // 注意：
  // - 这里不强制精确回收数，因为实现细节会影响对象数量
  // - 只要 last_swept_count() > 0，就说明自动 GC 确实发生过
  auto r = cilly::test::RunScript(R"(
    var i = 0;
    while (i < 800) {
      var t = [i, i + 1, i + 2];  // 每次都会分配一个 ObjList
      t = null;                  // 立刻丢弃引用，使其不可达
      i = i + 1;
    }
    return 123;
  )");

  ASSERT_TRUE(r.ret.IsNum());
  EXPECT_EQ(r.ret.AsNum(), 123);

  ASSERT_TRUE(r.gc_keepalive);
  // 核心断言：没有显式 __gc_collect，也应发生过 sweep
  EXPECT_GT(r.gc_keepalive->last_swept_count(), static_cast<std::size_t>(0));
}

}  // namespace
}  // namespace cilly
