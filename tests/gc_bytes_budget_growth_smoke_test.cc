#include <gtest/gtest.h>

#include "tests/run_script.h"

namespace cilly {
namespace {

TEST(GcBytesBudgetGrowthSmokeTest, DictGrowthCanTriggerAutoGcWithoutReserve) {
  // 设计：
  // - 构造 1 个持续增长的 dict（不使用 reserve），让 bucket_count
  // 在运行中多次扩容
  // - 同时少量制造“可回收垃圾”（几个临时 dict），确保一旦 GC 触发会有 sweep>0
  // - 不显式调用 __gc_collect
  //
  // 核心目的：
  // - 验证 bytes-budget 观察到 “dict 扩容后的真实 SizeBytes()”
  // - 从而让 VM 的自动 GC 在对象数不多时也能发生

  auto r = cilly::test::RunScript(
      R"(
    var keep = {};
    var i = 0;
    var tick = 0;

    while (i < 20000) {
        var k = str(i);
        keep[k] = i;

        tick = tick + 1;
        if (tick == 2000) {
        var g = {"x": i};
        g = null;
        tick = 0;
        }

        i = i + 1;
    }

    return 123;
    )",
      [](cilly::VM& vm) { vm.SetNextGcBytesThresholdForTest(1024); });

  ASSERT_TRUE(r.ret.IsNum());
  EXPECT_EQ(r.ret.AsNum(), 123);

  ASSERT_TRUE(r.gc_keepalive);

  // 核心断言：自动 GC 至少发生过一次 sweep
  EXPECT_GT(r.gc_keepalive->total_swept_count(), static_cast<std::size_t>(0));
}

}  // namespace
}  // namespace cilly
