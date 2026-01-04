#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "../src/frontend/generator.h"
#include "../src/frontend/lexer.h"
#include "../src/frontend/parser.h"
#include "../src/value.h"
#include "../src/vm.h"

// 如果这里报找不到，就去抄你现有能过的测试里 include 的 builtins 头文件名
#include "../src/builtins.h"

namespace cilly {

TEST(oo_method_basic_smoke_test, CallMethodNoThis) {
  const char* source = R"(
class A {
  fun f() { return 123; }
}
var a = A();
__test_emit(a.f());
return 0;
)";

  // 关键：避免 most vexing parse
  std::string src(source);
  Lexer lexer(src);
  std::vector<Token> tokens = lexer.ScanAll();

  Parser parser(std::move(tokens));
  std::vector<StmtPtr> program = parser.ParseProgram();

  Generator gen;
  Function main_fn = gen.Generate(program);

  VM vm;
  RegisterBuiltins(vm);

  for (const auto& fn : gen.Functions()) {
    vm.RegisterFunction(fn.get());
  }

  std::vector<Value> emitted;
  vm.SetTestEmitSink([&](const Value& v) { emitted.push_back(v); });

  vm.Run(main_fn);

  ASSERT_EQ(emitted.size(), 1u);
  ASSERT_TRUE(emitted[0].IsNum());
  EXPECT_EQ(emitted[0].AsNum(), 123);
}

}  // namespace cilly
