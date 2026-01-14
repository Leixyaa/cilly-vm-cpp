#ifndef TESTS_RUN_SCRIPT_H_
#define TESTS_RUN_SCRIPT_H_

#include <memory>
#include <string>
#include <vector>

#include "src/value.h"

namespace cilly::gc {
class Collector;
}

namespace cilly::test {

struct RunResult {
  Value ret = Value::Null();
  std::vector<Value> emitted;

  // 仅用于保活gc
  std::shared_ptr<gc::Collector> gc_keepalive;
};

// 编译并运行一段脚本，自动：RegisterBuiltins + 捕获 __test_emit
RunResult RunScript(const std::string& src);

}  // namespace cilly::test

#endif  // TESTS_RUN_SCRIPT_H_
