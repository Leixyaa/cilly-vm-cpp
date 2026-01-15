#include "tests/run_script.h"

#include "src/builtins.h"
#include "src/frontend/generator.h"
#include "src/frontend/lexer.h"
#include "src/frontend/parser.h"
#include "src/function.h"
#include "src/gc/gc.h"
#include "src/vm.h"

namespace cilly::test {

RunResult RunScript(const std::string& src, VmInitHook vm_init) {
  // 先创建一个Collector并放进RunResult保活
  auto gc = std::make_shared<gc::Collector>();

  // Capture emit
  RunResult r;
  // 放入RunResult 保活
  r.gc_keepalive = gc;

  // Compile
  Lexer lexer(src);
  std::vector<Token> tokens = lexer.ScanAll();
  Parser parser(tokens);
  auto program = parser.ParseProgram();

  Generator gen(r.gc_keepalive.get());
  Function main_fn = gen.Generate(program);

  // VM + builtins
  VM vm(r.gc_keepalive.get());
  RegisterBuiltins(vm);

  vm.SetTestEmitSink([&](const Value& v) { r.emitted.push_back(v); });

  // Register user functions
  for (const auto& fn : gen.Functions()) {
    vm.RegisterFunction(fn.get());
  }

  // 注入阈值
  if (vm_init) {
    vm_init(vm);
  }

  // Run
  vm.Run(main_fn);
  r.ret = vm.last_return_value();
  return r;
}

}  // namespace cilly::test
