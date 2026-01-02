#include <gtest/gtest.h>

#include "tests/run_script.h"

namespace cilly::test {

TEST(OOFoundation, ClassDeclSmoke) {
  auto r = RunScript(R"(
    class A {}
    var a = A();
    a.x = 7;
    __test_emit(a.x);
    return 0;
  )");

  ASSERT_EQ(r.emitted.size(), 1u);
  ASSERT_TRUE(r.emitted[0].IsNum());
  EXPECT_EQ(r.emitted[0].AsNum(), 7);

  ASSERT_TRUE(r.ret.IsNum());
  EXPECT_EQ(r.ret.AsNum(), 0);
}

}  // namespace cilly::test
