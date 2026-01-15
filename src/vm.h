#ifndef CILLY_VM_CPP_VM_H_
#define CILLY_VM_CPP_VM_H_

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "call_frame.h"
#include "function.h"
#include "object.h"
#include "opcodes.h"
#include "stack_stats.h"
#include "value.h"

namespace cilly::gc {
class Collector;
}

namespace cilly {

// 一个 C++ 函数/闭包”，输入是 VM + 参数数组，输出是一个 Value
// 别名
using NativeFn = std::function<Value(class VM&, const Value* args, int argc)>;

struct Callable {
  enum class Type { kBytecode, kNative } type;
  std::string name;
  int arity = 0;

  // bytecode
  int fn_index = -1;  // 指向 VM::functions_ 的下标
  const Function* fn = nullptr;

  // native
  NativeFn native;
};

// 最小可运行 VM:OP_CONSTANT / OP_ADD / OP_PRINT。
class VM {
 public:
  // 两种构造方式（原方式保留默认构造）
  VM();
  explicit VM(gc::Collector* gc);

  // 运行入口：执行给定函数（从头到尾）。
  void Run(const Function& fn);
  // 供测试使用：拿到最近一次 OP_RETURN 的返回值
  const Value& last_return_value() const { return last_return_value_; }

  // ---------------- test hook ----------------
  // 仅用于单测：脚本里调用 __test_emit(x) 时，会把 x 送到这里。
  // 默认没有 sink；生产运行不受影响。
  using TestEmitSink = std::function<void(const Value&)>;
  void SetTestEmitSink(TestEmitSink sink) { test_emit_sink_ = std::move(sink); }
  void TestEmit(const Value& v) {
    if (test_emit_sink_)
      test_emit_sink_(v);
  }
  std::shared_ptr<ObjDict> NewDictReservedForTest(std::size_t reserve_entries);

  // 便于调试：取内部栈的统计指标。
  int PushCount() const;
  int PopCount() const;
  int Depth() const;
  int MaxDepth() const;

  /////////// GC相关
  void CollectGarbage();
  // 仅测试使用：把下一次 GC 触发阈值调小，确保测试稳定触发
  void SetNextGcBytesThresholdForTest(std::size_t bytes);

  int RegisterFunction(const Function* fn);  // 注册函数并且返回索引
  int RegisterNative(const std::string& name, int arity,
                     NativeFn fn);  // 注册原生函数

  void DoCallByIndex(int call_index, int argc,
                     const Value* argv);  // 判断是user函数还是builtin函数

 private:
  // 取下一条指令（从 code_ 读取，并自增 ip_）。
  int32_t ReadI32_();

  // 取下一条操作数（32 位整型，自增 ip_）。
  int32_t ReadOpnd_();

  // 执行一条指令；返回是否继续执行。
  bool Step_();

  CallFrame& CurrentFrame();
  const CallFrame& CurrentFrame() const;

  StackStats stack_;  // 运行时栈（带统计）
  std::vector<CallFrame> frames_;
  std::vector<Callable> callables_;

  Value last_return_value_ = Value::Null();
  TestEmitSink test_emit_sink_;

  // GC相关
  gc::Collector* gc_ = nullptr;
  void TraceRoots(gc::Collector& c) const;
  // 在“安全点”尝试触发一次 GC
  // 说明：
  // - 安全点选择在 Step_ 开始处（即将执行本条指令之前）
  // - 这样可保证所有活跃 Value 都还在 VM 栈/locals 中，不会漏标
  void MaybeCollectGarbage_();
  // 默认阈值：工程运行用（建议先 16KB）
  std::size_t next_gc_bytes_threshold_ = 16 * 1024;
};

}  // namespace cilly

#endif  // CILLY_VM_CPP_VM_H_
