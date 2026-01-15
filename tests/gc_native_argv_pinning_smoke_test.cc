#include "gc/gc.h"
#include "gtest/gtest.h"
#include "tests/run_script.h"

namespace cilly {
namespace {

TEST(GcNativeArgvPinningSmokeTest, ObjectArgSurvivesGcInsideNative) {
  // 设计：
  // - 创建一个 list，并传给 __gc_touch(x)
  // - __gc_touch 在 native 内触发 GC 后返回 x
  // - 返回后继续访问该 list（len==3），证明没有被误回收
  //
  // 这个测试专门验证：DoCallByIndex(native) 在进入 native 前
  // 是否正确把 argv 里的 Obj 临时 PushRoot，避免 GC 中途 sweep。
  auto r = cilly::test::RunScript(R"(
    var x = [1, 2, 3];

    // native 内触发 GC，返回同一个对象
    x = __gc_touch(x);

    __test_emit(len(x));
    return len(x);
  )");

  ASSERT_TRUE(r.ret.IsNum());
  EXPECT_EQ(r.ret.AsNum(), 3);

  ASSERT_EQ(r.emitted.size(), 1u);
  ASSERT_TRUE(r.emitted[0].IsNum());
  EXPECT_EQ(r.emitted[0].AsNum(), 3);
}

}  // namespace
}  // namespace cilly
