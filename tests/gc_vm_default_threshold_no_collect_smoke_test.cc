#include "gc/gc.h"
#include "gtest/gtest.h"
#include "tests/run_script.h"

namespace cilly {
namespace {

TEST(GcVmDefaultThresholdSmokeTest, SmallScriptDoesNotAutoCollectByDefault) {
  // 设计：
  // - 不注入阈值（走默认 16KB）
  // - 小脚本只分配少量对象
  // - 预期不触发 sweep（或至少不是必须触发）
  auto r = cilly::test::RunScript(R"(
    var a = [1,2,3];
    var b = [4,5,6];
    return len(a) + len(b);
  )");

  ASSERT_TRUE(r.ret.IsNum());
  EXPECT_EQ(r.ret.AsNum(), 6);

  ASSERT_TRUE(r.gc_keepalive);
  EXPECT_EQ(r.gc_keepalive->last_swept_count(), static_cast<std::size_t>(0));
}

}  // namespace
}  // namespace cilly
