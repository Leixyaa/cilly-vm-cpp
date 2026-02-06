#include <gtest/gtest.h>

#include "tests/run_script.h"

namespace cilly::test {

TEST(GlobalVariableTest, BasicGetSet) {
  const std::string src = R"(
var g = 10;
fun modify() {
  g = 20;
}
modify();
return g;
)";
  RunResult res = RunScript(src);
  ASSERT_TRUE(res.ret.IsNum());
  EXPECT_EQ(res.ret.AsNum(), 20.0);
}

TEST(GlobalVariableTest, GlobalShadowing) {
  const std::string src = R"(
var g = 10;
fun test() {
  var g = 20; // local shadows global
  return g;
}
return test();
)";
  RunResult res = RunScript(src);
  ASSERT_TRUE(res.ret.IsNum());
  EXPECT_EQ(res.ret.AsNum(), 20.0);
}

TEST(GlobalVariableTest, GlobalAccessInFunction) {
  const std::string src = R"(
var g = 10;
fun test() {
  return g;
}
return test();
)";
  RunResult res = RunScript(src);
  ASSERT_TRUE(res.ret.IsNum());
  EXPECT_EQ(res.ret.AsNum(), 10.0);
}

}  // namespace cilly::test
