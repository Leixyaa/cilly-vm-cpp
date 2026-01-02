#include "tests/run_script.h"

#include "src/builtins.h"
#include "src/frontend/generator.h"
#include "src/frontend/lexer.h"
#include "src/frontend/parser.h"
#include "src/function.h"
#include "src/vm.h"

namespace cilly::test {

RunResult RunScript(const std::string& src) {
  // Compile
  Lexer lexer(src);
  std::vector<Token> tokens = lexer.ScanAll();
  Parser parser(tokens);
  auto program = parser.ParseProgram();

  Generator gen;
  Function main_fn = gen.Generate(program);

  // VM + builtins
  VM vm;
  RegisterBuiltins(vm);

  // Capture emit
  RunResult r;
  vm.SetTestEmitSink([&](const Value& v) { r.emitted.push_back(v); });

  // Register user functions
  for (const auto& fn : gen.Functions()) {
    vm.RegisterFunction(fn.get());
  }

  // Run
  vm.Run(main_fn);
  r.ret = vm.last_return_value();
  return r;
}

}  // namespace cilly::test
