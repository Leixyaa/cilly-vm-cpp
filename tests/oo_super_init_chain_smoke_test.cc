#include <gtest/gtest.h>

#include "run_script.h"

namespace cilly {

TEST(oo_super_init_chain_smoke_test, SuperInitIsCalledInsideSubclassInit) {
  const char* src = R"(
class B {
  fun init(v) { this.x = v; }
}

class A : B {
  fun init(v) {
    super.init(v);
    this.y = v + 1;
  }
}

var a = A(10);
__test_emit(a.x); // 10
__test_emit(a.y); // 11
return 0;
)";
  auto result = test::RunScript(src);
  ASSERT_EQ(result.ret.ToRepr(), "0");
  ASSERT_EQ(result.emitted.size(), 2u);
  EXPECT_EQ(result.emitted[0].ToRepr(), "10");
  EXPECT_EQ(result.emitted[1].ToRepr(), "11");
}

}  // namespace cilly
