#include "gc/gc.h"
#include "gtest/gtest.h"
#include "tests/run_script.h"

namespace cilly {
namespace {

TEST(GcTraceRootsFramesOnlySmokeTest, ActiveFramesConstPoolIsEnough) {
  auto r = cilly::test::RunScript(
      R"(
    fun make(x) {
      // 这里会分配对象，增加 GC 压力
      var t = [x, x + 1, x + 2];
      return t;
    }

    var keep = make(10);

    var i = 0;
    while (i < 5000) {
      var tmp = make(i);
      tmp = null;      // 让 tmp 不可达，给 sweep 机会
      // 每轮都访问 keep，确保它始终可用
      if (len(keep) != 3) {
        return 999;
      }
      i = i + 1;
    }

    __test_emit(len(keep));
    return len(keep);
  )",
      [](cilly::VM& vm) {
        vm.SetNextGcBytesThresholdForTest(1024);  // 1KB，保证一定触发
      });

  ASSERT_TRUE(r.ret.IsNum());
  EXPECT_EQ(r.ret.AsNum(), 3);

  ASSERT_EQ(r.emitted.size(), 1u);
  ASSERT_TRUE(r.emitted[0].IsNum());
  EXPECT_EQ(r.emitted[0].AsNum(), 3);

  // 不强制 sweep 次数，但一般会 >0（取决于阈值）
  ASSERT_TRUE(r.gc_keepalive);
}
}  // namespace
}  // namespace cilly
