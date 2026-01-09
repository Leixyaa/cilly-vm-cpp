#include <gtest/gtest.h>

#include "run_script.h"

namespace cilly {

TEST(oo_inherit_init_arity_fail_test, CtorArgsMustMatchInheritedInitArity) {
  const char* src = R"(
class B { fun init(v) { this.x = v; } }
class A : B { }
var a = A(); // 缺参：应失败
return 0;
)";
  // 你们 VM 里大概率是 assert/abort 方式报错，所以用 death test
  EXPECT_DEATH(
      {
        auto r = test::RunScript(src);
        (void)r;
      },
      "");  // 如果你 VM 有错误信息，可以把正则写得更精确
}

}  // namespace cilly
