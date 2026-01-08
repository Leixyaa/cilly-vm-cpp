#include <gtest/gtest.h>

#include "run_script.h"

namespace cilly {

TEST(oo_inherit_basic_smoke_test, MethodLookupFallsBackToSuperclass) {
  const char* src = R"(
class B { fun f(){ return 1; } }
class A : B { 
  fun f(){ return super.f() + 1; }
}
var a=A();
__test_emit(a.f()); // 2
return 0;
)";
  auto result = test::RunScript(src);
  ASSERT_EQ(result.ret.ToRepr(), "0");
  ASSERT_EQ(result.emitted.size(), 1u);
  EXPECT_EQ(result.emitted[0].ToRepr(), "2");
}

}  // namespace cilly
