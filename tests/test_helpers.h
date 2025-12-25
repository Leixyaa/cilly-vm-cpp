#ifndef CILLY_TEST_HELPERS_H_
#define CILLY_TEST_HELPERS_H_

#include <gtest/gtest.h>

#include "src/builtins.h"
#include "src/function.h"
#include "src/value.h"
#include "src/vm.h"

namespace cilly::test {

// 运行脚本并取返回值
inline Value RunAndGetReturn(VM& vm, const Function& fn) {
  vm.Run(fn);
  return vm.last_return_value();
}

// 构造一个带 builtins 的 VM
inline VM MakeVMWithBuiltins() {
  VM vm;
  RegisterBuiltins(vm);
  return vm;
}

// 常用断言：数值
inline void ExpectNum(const Value& v, double expected) {
  ASSERT_TRUE(v.IsNum());
  EXPECT_DOUBLE_EQ(v.AsNum(), expected);
}

// 常用断言：布尔
inline void ExpectBool(const Value& v, bool expected) {
  ASSERT_TRUE(v.IsBool());
  EXPECT_EQ(v.AsBool(), expected);
}

// 常用断言：null
inline void ExpectNull(const Value& v) {
  ASSERT_TRUE(v.IsNull());
}

}  // namespace cilly::test

#endif  // CILLY_TEST_HELPERS_H_
