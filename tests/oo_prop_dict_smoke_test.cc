#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "src/builtins.h"
#include "src/frontend/generator.h"
#include "src/frontend/lexer.h"
#include "src/frontend/parser.h"
#include "src/function.h"
#include "src/value.h"
#include "src/vm.h"

namespace cilly::test {

TEST(OOFoundation, DotProp_OnDict) {
  const std::string src = R"(
var d = {};
d.x = 1;
__test_emit(d.x);
return 0;
)";

  Lexer lexer(src);
  auto tokens = lexer.ScanAll();
  Parser parser(tokens);
  auto program = parser.ParseProgram();

  Generator gen;
  Function main_fn = gen.Generate(program);

  VM vm;
  RegisterBuiltins(vm);

  std::vector<Value> emitted;
  vm.SetTestEmitSink([&](const Value& v) { emitted.push_back(v); });

  for (const auto& fn : gen.Functions()) {
    vm.RegisterFunction(fn.get());
  }
  vm.Run(main_fn);

  ASSERT_EQ(emitted.size(), 1u);
  ASSERT_TRUE(emitted[0].IsNum());
  EXPECT_DOUBLE_EQ(emitted[0].AsNum(), 1);
}

}  // namespace cilly::test
