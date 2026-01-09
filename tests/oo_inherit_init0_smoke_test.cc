#include <gtest/gtest.h>

#include "run_script.h"

namespace cilly {

TEST(oo_inherit_init0_smoke_test, InheritedInitIsCalledWhenSubclassHasNoInit) {
  const char* src = R"(
class B {
  fun init() { this.x = 7; }
}
class A : B { }

var a = A();
__test_emit(a.x); // 7
return 0;
)";
  auto result = test::RunScript(src);
  ASSERT_EQ(result.ret.ToRepr(), "0");
  ASSERT_EQ(result.emitted.size(), 1u);
  EXPECT_EQ(result.emitted[0].ToRepr(), "7");
}

}  // namespace cilly
