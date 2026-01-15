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

  // __gc_touch(x)
  // 仅用于测试：验证 “native 调用期间 argv 保活” 是否正确。
  // 行为：触发一次 GC，然后把参数 x 原样返回。
  // 如果 VM 在进入 native 前没有把 argv 里的 Obj 临时压入 roots，
  // 那么 CollectGarbage() 可能会把 x 指向的对象 sweep 掉，导致 UAF/崩溃。
  int i7 = vm.RegisterNative("__gc_touch", 1,
                             [](VM& vm, const Value* args, int argc) {
                               (void)argc;
                               // 在 native 内触发 GC，模拟最危险的窗口期
                               vm.CollectGarbage();
                               // 直接返回参数
                               return args[0];
                             });
  assert(i7 == 7);

  // __make_big_dict(n)
  // 仅用于测试/bring-up：创建一个“内部 reserve 很大”的 dict
  // 目的：用极少对象制造较大 heap_bytes，占用在“构造阶段”就可见，从而稳定触发
  // bytes-budget 的自动 GC。
  int i8 = vm.RegisterNative(
      "__make_big_dict", 1, [](VM& vm, const Value* args, int argc) {
        assert(argc == 1);
        assert(args[0].IsNum());

        // 这里传入“期望的 reserve 条目数”，越大 bucket_count
        // 越大，占用估算越大
        const std::size_t n = static_cast<std::size_t>(args[0].AsNum());

        // 走 VM 的 GC 管理创建路径（保证进入 GC 的 all_objects_ 链表）
        auto dict = vm.NewDictReservedForTest(n);

        // Value::Obj 你在 vm.cc 里已经用过（绑定方法就是这么 push
        // 的），因此这里完全匹配
        return Value::Obj(dict);
      });
  assert(i8 == 8);
}

}  // namespace cilly
