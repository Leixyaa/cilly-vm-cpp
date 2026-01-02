#ifndef TESTS_RUN_SCRIPT_H_
#define TESTS_RUN_SCRIPT_H_

#include <string>
#include <vector>

#include "src/value.h"

namespace cilly::test {

struct RunResult {
  Value ret = Value::Null();
  std::vector<Value> emitted;
};

// 编译并运行一段脚本，自动：RegisterBuiltins + 捕获 __test_emit
RunResult RunScript(const std::string& src);

}  // namespace cilly::test

#endif  // TESTS_RUN_SCRIPT_H_
