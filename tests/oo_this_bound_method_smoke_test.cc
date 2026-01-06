
#include <gtest/gtest.h>

#include "run_script.h"

namespace cilly {

TEST(oo_this_bound_method_smoke_test, ThisWorksWhenCallingStoredBoundMethod) {
  const char* src = R"(
class A {
  fun set() { this.x = 9; }
  fun get() { return this.x; }
}
var a = A();
a.set();
var g = a.get;
__test_emit(g());
return 0;
)";

  auto result = test::RunScript(src);
  ASSERT_EQ(result.ret.ToRepr(), "0");
  ASSERT_EQ(result.emitted.size(), 1u);
  EXPECT_EQ(result.emitted[0].ToRepr(), "9");
}

}  // namespace cilly
