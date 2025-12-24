//覆盖：变量/赋值、if/while/for、break/continue、block+shadowing、list/dict、index get/set、函数参数/return、递归、互递归、
// call virtual（函数一等值+索引调用）、native builtins（len/str/type/abs/clock）

#include <gtest/gtest.h>

#include <chrono>
#include <string>
#include <vector>

#include "frontend/generator.h"
#include "frontend/lexer.h"
#include "frontend/parser.h"
#include "function.h"
#include "value.h"
#include "vm.h"

namespace {

static void RegisterBuiltins(cilly::VM& vm) {
  using namespace cilly;

  int i0 = vm.RegisterNative("len", 1, [](VM&, const Value* args, int argc) {
    assert(argc == 1);
    const Value& v = args[0];
    if (v.IsStr())   return Value::Num((double)v.AsStr().size());
    if (v.IsList())  return Value::Num((double)v.AsList()->Size());
    if (v.IsDict())  return Value::Num((double)v.AsDict()->Size());
    if (v.IsString())return Value::Num((double)v.AsString()->ToRepr().size());
    assert(false && "len() unsupported type");
    return Value::Null();
  });
  assert(i0 == 0);

  int i1 = vm.RegisterNative("str", 1, [](VM&, const Value* args, int argc) {
    assert(argc == 1);
    const Value& v = args[0];
    if (v.IsStr()) return v;
    return Value::Str(v.ToRepr());
  });
  assert(i1 == 1);

  int i2 = vm.RegisterNative("type", 1, [](VM&, const Value* args, int argc) {
    assert(argc == 1);
    const Value& v = args[0];
    if (v.IsNull()) return Value::Str("null");
    if (v.IsBool()) return Value::Str("bool");
    if (v.IsNum())  return Value::Str("number");
    if (v.IsStr() || v.IsString()) return Value::Str("str");
    if (v.IsList()) return Value::Str("list");
    if (v.IsDict()) return Value::Str("dict");
    if (v.IsObj())  return Value::Str("object");
    return Value::Str("unknown");
  });
  assert(i2 == 2);

  int i3 = vm.RegisterNative("abs", 1, [](VM&, const Value* args, int argc) {
    assert(argc == 1);
    assert(args[0].IsNum());
    double x = args[0].AsNum();
    return Value::Num(x < 0 ? -x : x);
  });
  assert(i3 == 3);

  int i4 = vm.RegisterNative("clock", 0, [](VM&, const Value*, int) {
    using clock = std::chrono::steady_clock;
    double s = std::chrono::duration_cast<std::chrono::duration<double>>(
                 clock::now().time_since_epoch()).count();
    return Value::Num(s);
  });
  assert(i4 == 4);
}

// 允许输出里夹杂其它行，只要求关键 token 按顺序出现
static void ExpectInOrder(const std::string& out, const std::vector<std::string>& seq) {
  size_t pos = 0;
  for (const auto& s : seq) {
    size_t found = out.find(s, pos);
    ASSERT_NE(found, std::string::npos)
        << "Missing token: [" << s << "]\n=== output ===\n" << out;
    pos = found + s.size();
  }
}

}  // namespace

TEST(Milestone, FullSmoke_AllFeatures) {
  using namespace cilly;

  // 注意：不要用 // 注释，避免 lexer 不支持
  const std::string source = R"(
print "BEGIN";

fun add(a, b) { return a + b; }

fun fact(n) {
  if (n == 0) return 1;
  return n * fact(n - 1);
}

fun is_even(n) {
  if (n == 0) return true;
  return is_odd(n - 1);
}
fun is_odd(n) {
  if (n == 0) return false;
  return is_even(n - 1);
}

var f = add;
print f(1, 2);

var o = { "m": add };
print o["m"](20, 22);

print fact(5);
print is_even(10);
print is_odd(9);
print is_odd(10);

var xs = [1, 2, 3];
xs[1] = 99;
print xs[1];

var d = { "a": 1 };
d["b"] = 2;
print d["a"] + d["b"];

var sum = 0;
for (var i = 0; i < 5; i = i + 1) {
  if (i == 2) continue;
  if (i == 4) break;
  sum = sum + i;
}
print sum;

{
  var x = 1;
  {
    var x = 2;
  }
  print x;
}

print len([1, 2, 3]);
print abs(0 - 9);
print type(123);
print type(clock());
print str(123);

print "END";
)";

  testing::internal::CaptureStdout();

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

  const std::string out = testing::internal::GetCapturedStdout();

  // 关键输出顺序校验（不关心中间是否夹杂“Return value: null”等额外行）
  ExpectInOrder(out, {
    "BEGIN",
    "3",       // f(1,2)
    "42",      // o["m"](20,22)
    "120",     // fact(5)
    "true",    // is_even(10)
    "true",    // is_odd(9)
    "false",   // is_odd(10)
    "99",      // list index set/get
    "3",       // dict a+b
    "4",       // for + break/continue: 0+1+3
    "1",       // shadowing 后外层 x 仍为 1
    "3",       // len([1,2,3])
    "9",       // abs(-9)
    "number",  // type(123)
    "number",  // type(clock())
    "123",     // str(123)
    "END",
  });
}
