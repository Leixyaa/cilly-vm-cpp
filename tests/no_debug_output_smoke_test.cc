#include <gtest/gtest.h>

#include "run_script.h"

namespace cilly {

TEST(no_debug_output_smoke_test, RunScriptProducesNoStdoutOrStderrByDefault) {
  const char* src = R"(
class A { fun init() { this.x = 1; } }
var a = A();
return 0;
)";

  testing::internal::CaptureStdout();
  testing::internal::CaptureStderr();

  auto result = test::RunScript(src);

  std::string out = testing::internal::GetCapturedStdout();
  std::string err = testing::internal::GetCapturedStderr();

  ASSERT_EQ(result.ret.ToRepr(), "0");
  EXPECT_TRUE(out.empty());
  EXPECT_TRUE(err.empty());
}

}  // namespace cilly
