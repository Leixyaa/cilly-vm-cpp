#include "builtins.h"

#include <cassert>
#include <chrono>

#include "vm.h"
#include "value.h"

namespace cilly {

void RegisterBuiltins(VM& vm) {
  // len(x)
  int i0 = vm.RegisterNative("len", 1, [](VM&, const Value* args, int argc) {
    assert(argc == 1);
    const Value& v = args[0];
    if (v.IsStr()) return Value::Num((double)v.AsStr().size());
    if (v.IsList()) return Value::Num((double)v.AsList()->Size());
    if (v.IsDict()) return Value::Num((double)v.AsDict()->Size());
    if (v.IsString()) return Value::Num((double)v.AsString()->ToRepr().size());
    assert(false && "len() unsupported type");
    return Value::Null();
  });
  assert(i0 == 0);

  // str(x)
  int i1 = vm.RegisterNative("str", 1, [](VM&, const Value* args, int argc) {
    assert(argc == 1);
    const Value& v = args[0];
    if (v.IsStr()) return v;
    return Value::Str(v.ToRepr());
  });
  assert(i1 == 1);

  // type(x)
  int i2 = vm.RegisterNative("type", 1, [](VM&, const Value* args, int argc) {
    assert(argc == 1);
    const Value& v = args[0];
    if (v.IsNull()) return Value::Str("null");
    if (v.IsBool()) return Value::Str("bool");
    if (v.IsNum()) return Value::Str("number");
    if (v.IsStr() || v.IsString()) return Value::Str("str");
    if (v.IsList()) return Value::Str("list");
    if (v.IsDict()) return Value::Str("dict");
    if (v.IsObj()) return Value::Str("object");
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
}

}  // namespace cilly
