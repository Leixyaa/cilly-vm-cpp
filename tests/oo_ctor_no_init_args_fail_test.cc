#include <gtest/gtest.h>

#include "run_script.h"

namespace cilly {

TEST(oo_ctor_no_init_args_fail_test, CtorWithArgsWithoutInitShouldFail) {
  const char* src = R"(
class A { }
var a = A(1);
return 0;
)";

  EXPECT_DEATH(
      {
        auto r = test::RunScript(src);
        (void)r;
      },
      "supports 0 args only");
}

}  // namespace cilly
