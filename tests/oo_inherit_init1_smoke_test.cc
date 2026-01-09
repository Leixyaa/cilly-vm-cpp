#include <gtest/gtest.h>

#include "run_script.h"

namespace cilly {

TEST(oo_inherit_init1_smoke_test, InheritedInitReceivesArgsFromCtorCall) {
  const char* src = R"(
class B {
  fun init(v) { this.x = v; }
}
class A : B { }

var a = A(42);
__test_emit(a.x); // 42
return 0;
)";
  auto result = test::RunScript(src);
  ASSERT_EQ(result.ret.ToRepr(), "0");
  ASSERT_EQ(result.emitted.size(), 1u);
  EXPECT_EQ(result.emitted[0].ToRepr(), "42");
}

}  // namespace cilly
