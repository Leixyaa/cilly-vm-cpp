#include "builtins.h"

#include <cassert>
#include <chrono>

#include "value.h"
#include "vm.h"

namespace cilly {

void RegisterBuiltins(VM& vm) {
  // len(x)
  int i0 = vm.RegisterNative("len", 1, [](VM&, const Value* args, int argc) {
    assert(argc == 1);
    const Value& v = args[0];
    if (v.IsStr())
      return Value::Num((double)v.AsStr().size());
    if (v.IsList())
      return Value::Num((double)v.AsList()->Size());
    if (v.IsDict())
      return Value::Num((double)v.AsDict()->Size());
    if (v.IsString())
      return Value::Num((double)v.AsString()->ToRepr().size());
    assert(false && "len() unsupported type");
    return Value::Null();
  });
  assert(i0 == 0);

  // str(x)
  int i1 = vm.RegisterNative("str", 1, [](VM&, const Value* args, int argc) {
    assert(argc == 1);
    const Value& v = args[0];
    if (v.IsStr())
      return v;
    return Value::Str(v.ToRepr());
  });
  assert(i1 == 1);

  // type(x)
  int i2 = vm.RegisterNative("type", 1, [](VM&, const Value* args, int argc) {
    assert(argc == 1);
    const Value& v = args[0];
    if (v.IsNull())
      return Value::Str("null");
    if (v.IsBool())
      return Value::Str("bool");
    if (v.IsNum())
      return Value::Str("number");
    if (v.IsStr() || v.IsString())
      return Value::Str("str");
    if (v.IsList())
      return Value::Str("list");
    if (v.IsDict())
      return Value::Str("dict");
    if (v.IsObj())
      return Value::Str("object");
    return Value::Str("unknown");
  });
  assert(i2 == 2);

  // abs(x)
  int i3 = vm.RegisterNative("abs", 1, [](VM&, const Value* args, int argc) {
    assert(argc == 1);
    assert(args[0].IsNum());
    double x = args[0].AsNum();
    return Value::Num(x < 0 ? -x : x);
  });
  assert(i3 == 3);

  // clock()
  int i4 = vm.RegisterNative("clock", 0, [](VM&, const Value*, int) {
    using clock = std::chrono::steady_clock;
    double s = std::chrono::duration_cast<std::chrono::duration<double>>(
                   clock::now().time_since_epoch())
                   .count();
    return Value::Num(s);
  });
  assert(i4 == 4);

  // __test_emit(x)  (for gtest only; no-op if sink is not set)
  int i5 = vm.RegisterNative("__test_emit", 1,
                             [](VM& vm, const Value* args, int argc) {
                               assert(argc == 1);
                               vm.TestEmit(args[0]);
                               return args[0];
                             });
  assert(i5 == 5);

  // __gc_collect()
  // 仅用于测试/bring-up：让脚本在执行过程中主动触发一次 GC。
  // 这样就能用 gtest 验证：VM roots 扫描是否正确、是否会误回收可达对象。
  int i6 = vm.RegisterNative("__gc_collect", 0, [](VM& vm, const Value*, int) {
    vm.CollectGarbage();
    return Value::Null();
  });

  // builtin 索引要固定，确保 Generator 与 VM
  // 的映射一致（否则脚本解析/生成会错位）
  assert(i6 == 6);
}

}  // namespace cilly
