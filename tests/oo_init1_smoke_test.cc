#include <gtest/gtest.h>

#include "run_script.h"

namespace cilly {

TEST(oo_init1_smoke_test, ClassCallPassesArgsToInit) {
  const char* src = R"(
class A {
  fun init(x) { this.x = x; }
  fun get() { return this.x; }
}
var a = A(7);
__test_emit(a.get());
return 0;
)";

  auto result = test::RunScript(src);
  ASSERT_EQ(result.ret.ToRepr(), "0");
  ASSERT_EQ(result.emitted.size(), 1u);
  EXPECT_EQ(result.emitted[0].ToRepr(), "7");
}

}  // namespace cilly
