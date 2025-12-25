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

static Value RunSourceAndGetReturn(const std::string& source) {
  Lexer lexer(source);
  std::vector<Token> tokens = lexer.ScanAll();

  Parser parser(tokens);
  std::vector<StmtPtr> program = parser.ParseProgram();

  Generator generator;
  Function main_fn = generator.Generate(program);

  VM vm;
  RegisterBuiltins(vm);
  for (const auto& fnptr : generator.Functions()) {
    vm.RegisterFunction(fnptr.get());
  }

  vm.Run(main_fn);
  return vm.last_return_value();
}

static void ExpectNum(const Value& v, double expected) {
  ASSERT_TRUE(v.IsNum()) << "actual: " << v.ToRepr();
  EXPECT_DOUBLE_EQ(v.AsNum(), expected);
}

static void ExpectBool(const Value& v, bool expected) {
  ASSERT_TRUE(v.IsBool()) << "actual: " << v.ToRepr();
  EXPECT_EQ(v.AsBool(), expected);
}

static void ExpectStr(const Value& v, const std::string& expected) {
  ASSERT_TRUE(v.IsStr()) << "actual: " << v.ToRepr();
  EXPECT_EQ(v.AsStr(), expected);
}

}  // namespace cilly::test

// ---------------- E2E: variables / assignment ----------------
TEST(FrontendE2E, VarAndAssign_Returns30) {
  const std::string src = R"(
var a = 20;
var b = 10;
return a + b;
)";
  cilly::Value ret = cilly::test::RunSourceAndGetReturn(src);
  cilly::test::ExpectNum(ret, 30);
}

// ---------------- E2E: block scope + shadowing ----------------
TEST(FrontendE2E, BlockShadowing_Returns1) {
  const std::string src = R"(
var x = 1;
{
  var x = 2;
}
return x;
)";
  cilly::Value ret = cilly::test::RunSourceAndGetReturn(src);
  cilly::test::ExpectNum(ret, 1);
}

// ---------------- E2E: for + break/continue ----------------
TEST(FrontendE2E, ForBreakContinue_Returns4) {
  // sum = 0 + 1 + 3 = 4  (skip 2, stop at 4)
  const std::string src = R"(
var sum = 0;
for (var i = 0; i < 5; i = i + 1) {
  if (i == 2) continue;
  if (i == 4) break;
  sum = sum + i;
}
return sum;
)";
  cilly::Value ret = cilly::test::RunSourceAndGetReturn(src);
  cilly::test::ExpectNum(ret, 4);
}

// ---------------- E2E: recursion ----------------
TEST(FrontendE2E, Recursion_Fact5_Returns120) {
  const std::string src = R"(
fun fact(n) {
  if (n == 0) return 1;
  return n * fact(n - 1);
}
return fact(5);
)";
  cilly::Value ret = cilly::test::RunSourceAndGetReturn(src);
  cilly::test::ExpectNum(ret, 120);
}

// ---------------- E2E: mutual recursion ----------------
TEST(FrontendE2E, MutualRecursion_OddEven_ReturnsTrue) {
  const std::string src = R"(
fun is_even(n) {
  if (n == 0) return true;
  return is_odd(n - 1);
}
fun is_odd(n) {
  if (n == 0) return false;
  return is_even(n - 1);
}
return is_odd(9);
)";
  cilly::Value ret = cilly::test::RunSourceAndGetReturn(src);
  cilly::test::ExpectBool(ret, true);
}

// ---------------- E2E: call-virtual (function as value) ----------------
TEST(FrontendE2E, CallV_FunctionAsValue_Returns3) {
  const std::string src = R"(
fun add(a, b) { return a + b; }
var f = add;
return f(1, 2);
)";
  cilly::Value ret = cilly::test::RunSourceAndGetReturn(src);
  cilly::test::ExpectNum(ret, 3);
}

// ---------------- E2E: call-virtual (dict index -> function) ----------------
TEST(FrontendE2E, CallV_FromDictIndex_Returns42) {
  const std::string src = R"(
fun add(a, b) { return a + b; }
var o = { "m": add };
return o["m"](20, 22);
)";
  cilly::Value ret = cilly::test::RunSourceAndGetReturn(src);
  cilly::test::ExpectNum(ret, 42);
}

// ---------------- E2E: builtins ----------------
TEST(FrontendE2E, Builtins_LenAbsTypeStr_ReturnsLen) {
  // 这里选一个“确定性返回值”做断言：len([1,2,3]) == 3
  const std::string src = R"(
var a = len([1,2,3]);
var b = abs(0 - 9);
var c = type(123);
var d = str(123);
return a;
)";
  cilly::Value ret = cilly::test::RunSourceAndGetReturn(src);
  cilly::test::ExpectNum(ret, 3);
}
