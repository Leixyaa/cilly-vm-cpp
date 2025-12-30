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

TEST(ScriptBridge, TestEmit_CanCaptureValues) {
  const std::string src = R"(
__test_emit(1);
__test_emit(2 + 3);
__test_emit([1, 2, 3]);
return 0;
)";

  // Compile
  Lexer lexer(src);
  std::vector<Token> tokens = lexer.ScanAll();
  Parser parser(tokens);
  std::vector<StmtPtr> program = parser.ParseProgram();

  Generator generator;
  Function main_fn = generator.Generate(program);

  // VM with builtins
  VM vm;
  RegisterBuiltins(vm);

  // Capture emitted values
  std::vector<Value> emitted;
  vm.SetTestEmitSink([&](const Value& v) { emitted.push_back(v); });

  // Register user functions (must be after builtins)
  for (const auto& fnptr : generator.Functions()) {
    vm.RegisterFunction(fnptr.get());
  }

  vm.Run(main_fn);

  ASSERT_EQ(emitted.size(), 3u);

  ASSERT_TRUE(emitted[0].IsNum());
  EXPECT_DOUBLE_EQ(emitted[0].AsNum(), 1);

  ASSERT_TRUE(emitted[1].IsNum());
  EXPECT_DOUBLE_EQ(emitted[1].AsNum(), 5);

  ASSERT_TRUE(emitted[2].IsObj());
  auto list = emitted[2].AsList();
  ASSERT_EQ(list->Size(), 3);
  EXPECT_TRUE(list->At(0).IsNum());
  EXPECT_DOUBLE_EQ(list->At(0).AsNum(), 1);
}

}  // namespace cilly::test
