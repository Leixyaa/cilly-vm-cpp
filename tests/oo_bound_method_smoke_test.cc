#include <gtest/gtest.h>

#include <string>

#include "tests/run_script.h"

namespace cilly {

TEST(oo_bound_method_smoke_test, CanStoreBoundMethodAndCallLater) {
  const char* src = R"(
class A {
  fun f() { return 123; }
}

var a = A();
var g = a.f;

__test_emit(g);

__test_emit(g());
return 0;
)";

  auto result = test::RunScript(src);

  ASSERT_EQ(result.ret.ToRepr(), "0");
  ASSERT_EQ(result.emitted.size(), 2u);

  EXPECT_TRUE(result.emitted[0].IsBoundMethod());
  EXPECT_EQ(result.emitted[1].ToRepr(), "123");
}

}  // namespace cilly
