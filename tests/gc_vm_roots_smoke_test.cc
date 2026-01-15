// tests/gc_vm_roots_smoke_test.cc

#include "gtest/gtest.h"

// 关键：run_script.h 里只 forward declare 了 gc::Collector（不完整类型）
// 如果测试里要访问 Collector 的成员函数（比如 last_swept_count），
// 就必须 include 定义，否则会编译失败（你现在遇到的 C2027/C2039）。
#include "gc/gc.h"
#include "tests/run_script.h"

namespace cilly {
namespace {

TEST(GcVmRootsSmokeTest, CollectDuringRunKeepsReachablesAndSweepsGarbage) {
  // 测试设计：
  // 1) 创建两个 list：
  //    - keep：始终可达（有变量引用）
  //    - drop：先创建，再置为 null，使其变成不可达
  // 2) 调用 __gc_collect() 触发一次 GC
  // 3) 断言：
  //    - keep 仍然能用（len(keep) == 3）
  //    - sweep 至少发生过一次（一般会把 drop 对应对象回收）
  //

  auto r = cilly::test::RunScript(R"(
    var keep = [1,2,3];
    var drop = [4,5,6];

    drop = null;
    __gc_collect();

    __test_emit(len(keep));
    return len(keep);
  )");

  // RunScript 里会返回一个 gc_keepalive，保证 Collector 生命周期覆盖断言阶段
  ASSERT_TRUE(r.gc_keepalive);

  // __test_emit 的回传断言：keep 仍然可用，长度应为 3
  ASSERT_EQ(r.emitted.size(), 1u);
  ASSERT_TRUE(r.emitted[0].IsNum());
  EXPECT_EQ(r.emitted[0].AsNum(), 3);

  // return 值也应一致
  ASSERT_TRUE(r.ret.IsNum());
  EXPECT_EQ(r.ret.AsNum(), 3);

  // 验证：本次脚本执行中确实发生了 sweep（回收不可达对象）
  EXPECT_GE(r.gc_keepalive->last_swept_count(), static_cast<std::size_t>(1));
}

}  // namespace
}  // namespace cilly
