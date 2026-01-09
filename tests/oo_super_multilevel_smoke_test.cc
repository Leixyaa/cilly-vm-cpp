#include <gtest/gtest.h>

#include "run_script.h"

namespace cilly {

TEST(oo_super_multilevel_smoke_test, SuperFallsBackAlongSuperclassChain) {
  const char* src = R"(
class C { fun f() { return 1; } }
class B : C { }          // B 没有 f
class A : B {
  fun f() { return super.f(); }  // super 从 B 开始找，应该回退到 C::f
}

var a = A();
__test_emit(a.f()); // 1
return 0;
)";
  auto result = test::RunScript(src);
  ASSERT_EQ(result.ret.ToRepr(), "0");
  ASSERT_EQ(result.emitted.size(), 1u);
  EXPECT_EQ(result.emitted[0].ToRepr(), "1");
}

}  // namespace cilly
