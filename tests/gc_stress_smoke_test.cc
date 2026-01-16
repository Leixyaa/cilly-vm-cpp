#include "gc/gc.h"  // 为了访问 total_swept_count()
#include "gtest/gtest.h"
#include "tests/run_script.h"  // RunScript

namespace cilly {
namespace {

TEST(GcStressSmokeTest, LongScript_MultiCollect_NoCrash_AndSweepsSomething) {
  // 设计要点：
  // 1) 用“已验证稳定”的分配路径：make() 里创建 list
  // 2) 循环中不断制造短命对象 tmp，并置 null，确保可 sweep
  // 3) 周期性显式 __gc_collect()，保证多轮 GC 一定发生
  // 4) keep 每轮都被访问，验证 roots 没掉
  const std::string script = R"cilly(
fun make(x) {
  var t = [x, x + 1, x + 2];
  return t;
}

var keep = make(10);

var tick = 0;
var i = 0;
while (i < 20000) {
  var tmp = make(i);
  tmp = null;

  if (tick == 0) {
    __gc_collect();
    tick = 97;
  }
  tick = tick - 1;

  if (len(keep) != 3) {
    return 999;
  }

  i = i + 1;
}

__gc_collect();
__test_emit(len(keep));
return len(keep);
)cilly";
  auto r = cilly::test::RunScript(script, [](cilly::VM& vm) {
    // 阈值压低，让“自动 GC”也参与（即便没有它，脚本里的 __gc_collect
    // 也保证多轮 GC）
    vm.SetNextGcBytesThresholdForTest(1024);  // 1KB
    vm.SetNextGcObjectThresholdForTest(128);  // 对象数阈值
  });

  ASSERT_TRUE(r.gc_keepalive);

  // 行为正确性：keep 必须一直活着
  ASSERT_TRUE(r.ret.IsNum());
  EXPECT_EQ(r.ret.AsNum(), 3);

  ASSERT_EQ(r.emitted.size(), 1u);
  ASSERT_TRUE(r.emitted[0].IsNum());
  EXPECT_EQ(r.emitted[0].AsNum(), 3);

  // 关键断言：确实发生过 sweep（守门员条件）
  EXPECT_GT(r.gc_keepalive->total_swept_count(), static_cast<std::size_t>(0));
}

}  // namespace
}  // namespace cilly
