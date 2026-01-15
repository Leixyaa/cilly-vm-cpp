#include <string>

#include "gc/gc.h"
#include "gtest/gtest.h"
#include "tests/run_script.h"

namespace cilly {
namespace {

TEST(GcBytesBudgetSmokeTest, LargeDictReservedAtConstructionCanTriggerAutoGc) {
  auto r = cilly::test::RunScript(R"(
    // 1) 构造阶段就 reserve 很大的 dict（对象数很少，但 heap_bytes 很快变大）
    var keep = __make_big_dict(50000);

    // 2) 制造一些垃圾对象，让 sweep 有东西可回收
    var i = 0;
    while (i < 2000) {
      var t = [i, i + 1, i + 2];
      t = null;
      i = i + 1;
    }

    return 123;
  )",
                                  [](cilly::VM& vm) {
                                    // 测试里把阈值调小，确保自动 GC
                                    // 在执行中发生（避免不同平台/实现差异导致不触发）
                                    vm.SetNextGcBytesThresholdForTest(1024);
                                  });

  ASSERT_TRUE(r.ret.IsNum());
  EXPECT_EQ(r.ret.AsNum(), 123);

  ASSERT_TRUE(r.gc_keepalive);
  EXPECT_GT(r.gc_keepalive->last_swept_count(), static_cast<std::size_t>(0));
}
}  // namespace
}  // namespace cilly
