#include <gtest/gtest.h>

#include "run_script.h"

namespace cilly {

TEST(oo_inherit_override_smoke_test, SubclassOverridesSuperclassMethod) {
  const char* src = R"(
class B { 
  fun f(){ return 1; } 
}

class A : B { 
  fun f(){ return 2; } 
}

var a = A();
__test_emit(a.f());
return 0;
)";
  auto result = test::RunScript(src);
  ASSERT_EQ(result.ret.ToRepr(), "0");
  ASSERT_EQ(result.emitted.size(), 1u);
  EXPECT_EQ(result.emitted[0].ToRepr(), "2");
}

}  // namespace cilly
