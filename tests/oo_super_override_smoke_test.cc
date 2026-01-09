#include <gtest/gtest.h>

#include "run_script.h"

namespace cilly {

TEST(oo_super_override_smoke_test, SuperCallsSuperclassEvenWhenOverridden) {
  const char* src = R"(
class B { fun f() { return 1; } }
class A : B {
  fun f() { return 100; }
  fun g() { return super.f(); }
}
var a = A();
__test_emit(a.f()); // 100
__test_emit(a.g()); // 1
return 0;
)";
  auto result = test::RunScript(src);
  ASSERT_EQ(result.ret.ToRepr(), "0");
  ASSERT_EQ(result.emitted.size(), 2u);
  EXPECT_EQ(result.emitted[0].ToRepr(), "100");
  EXPECT_EQ(result.emitted[1].ToRepr(), "1");
}

}  // namespace cilly
