#include <gtest/gtest.h>

#include "run_script.h"

namespace cilly {

TEST(oo_init0_smoke_test, ClassCallAutoInvokesInitZeroArity) {
  const char* src = R"(
class A {
  fun init() { this.x = 7; }
  fun get()  { return this.x; }
}
var a = A();
__test_emit(a.get());
return 0;
)";

  auto result = test::RunScript(src);
  ASSERT_EQ(result.ret.ToRepr(), "0");
  ASSERT_EQ(result.emitted.size(), 1u);
  EXPECT_EQ(result.emitted[0].ToRepr(), "7");
}

}  // namespace cilly
