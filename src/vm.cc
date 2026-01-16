#include "vm.h"

#include <cassert>
#include <functional>
#include <iostream>

#include "builtins.h"
#include "debug_log.h"
#include "gc/gc.h"

namespace cilly {

VM::VM() = default;
VM::VM(gc::Collector* gc) : gc_(gc) {
}

void VM::Run(const Function& fn) {
  frames_.clear();
  CallFrame frame;
  frame.fn = &fn;
  frame.ip = 0;
  frame.ret_ip = -1;                                     // 主函数没有返回地址
  frame.locals_.assign(fn.LocalCount(), Value::Null());  // 初始化locals变量表
  frames_.push_back(frame);

  // 每次运行前清空栈
  stack_.Clear();
  last_return_value_ = Value::Null();

  while (true) {
    CallFrame& cf = CurrentFrame();
    const Chunk& ch = cf.fn->chunk();

    if (cf.ip >= ch.CodeSize()) {
      // 当前帧到头了：对主函数来说可以结束；
      // 后面支持多帧时，可以决定是报错还是自动返回。
      break;
    }

    bool cont = Step_();  // 注意：这里先改成不传 fn
    if (!cont)
      break;
  }
}

int32_t VM::ReadI32_() {
  CallFrame& cf = CurrentFrame();
  const Chunk& ch = cf.fn->chunk();
  assert(cf.ip >= 0 && cf.ip < ch.CodeSize());
  return ch.CodeAt(cf.ip++);
}

int32_t VM::ReadOpnd_() {
  return ReadI32_();
}

std::shared_ptr<ObjDict> VM::NewDictReservedForTest(
    std::size_t reserve_entries) {
  if (gc_) {
    return gc::MakeShared<ObjDict>(*gc_, reserve_entries);
  }
  return std::make_shared<ObjDict>(reserve_entries);
}

int VM::PushCount() const {
  return stack_.PushCount();
}

int VM::PopCount() const {
  return stack_.PopCount();
}

int VM::Depth() const {
  return stack_.Depth();
}

int VM::MaxDepth() const {
  return stack_.MaxDepth();
}

void VM::DoCallByIndex(int call_index, int argc,
                       const Value* argv) {  // 快速调用函数
  assert(call_index >= 0 && call_index < (int)callables_.size());
  Callable& c = callables_[call_index];
  if (c.type == Callable::Type::kNative) {
    assert(c.arity == argc);
    // 在进入 native 之前，把 argv 里所有 Obj 临时压入 GC 的 temp roots 栈
    std::vector<gc::GcObject*> pinned;
    if (gc_) {
      pinned.reserve(argc);
      for (int i = 0; i < argc; ++i) {
        if (argv[i].IsObj()) {
          gc::GcObject* obj = argv[i].AsObj().get();
          gc_->PushRoot(obj);  // 临时root栈
          pinned.push_back(obj);
        }
      }
    }

    Value ret = c.native(*this, argv, argc);
    if (gc_) {
      // 退出native之后再弹出roots
      for (auto it = pinned.rbegin(); it != pinned.rend(); ++it) {
        gc_->PopRoot(*it);
      }
    }

    stack_.Push(ret);
    return;
  }

  const Function* callee = c.fn;
  assert(callee != nullptr);
  assert(c.arity == argc);

  // 为被调用函数创建一个新的调用帧
  CallFrame frame;
  frame.fn = callee;
  frame.ip = 0;       // 被调用函数从头开始执行
  frame.ret_ip = -1;  // 后续如有跳转再扩展
  int local_count = callee->LocalCount();
  frame.locals_.assign(local_count, Value::Null());

  for (int i = 0; i < argc; i++) {
    assert(i < (int)frame.locals_.size());
    frame.locals_[i] = argv[i];
  }
  frames_.push_back(std::move(frame));
  return;
}

bool VM::Step_() {
  CallFrame& cf = CurrentFrame();
  const Chunk& ch = cf.fn->chunk();
  int32_t raw = ReadI32_();
  OpCode op = static_cast<OpCode>(raw);

  switch (op) {
    case OpCode::OP_POP: {
      stack_.Pop();
      break;
    }

    case OpCode::OP_POPN: {
      int start = ReadOpnd_();
      int count = ReadOpnd_();
      assert(start >= 0 && count >= 0 &&
             start + count <= (int)cf.locals_.size());
      for (int i = 0; i < count; i++) {
        cf.locals_[start + i] = Value::Null();
      }
      break;
    }

    case OpCode::OP_DUP: {
      Value v = stack_.Top();
      stack_.Push(v);
      break;
    }

    case OpCode::OP_CONSTANT: {
      int index = ReadOpnd_();
      stack_.Push(ch.ConstAt(index));
      break;
    }
    case OpCode::OP_ADD: {
      // 先弹右操作数，再弹左操作数（便于将来支持非交换运算）。
      Value rhs = stack_.Pop();
      Value lhs = stack_.Pop();
      assert(lhs.IsNum() && rhs.IsNum());
      stack_.Push(Value::Num(lhs.AsNum() + rhs.AsNum()));
      break;
    }

    case OpCode::OP_SUB: {
      Value rhs = stack_.Pop();
      Value lhs = stack_.Pop();
      assert(rhs.IsNum() && lhs.IsNum());
      stack_.Push(Value::Num(lhs.AsNum() - rhs.AsNum()));
      break;
    }

    case OpCode::OP_MUL: {
      Value rhs = stack_.Pop();
      Value lhs = stack_.Pop();
      assert(rhs.IsNum() && lhs.IsNum());
      stack_.Push(Value::Num(lhs.AsNum() * rhs.AsNum()));
      break;
    }

    case OpCode::OP_DIV: {
      Value rhs = stack_.Pop();
      Value lhs = stack_.Pop();
      assert(rhs.IsNum() && lhs.IsNum());
      assert(rhs.AsNum() != 0 && "除数不可为0");
      stack_.Push(Value::Num(lhs.AsNum() / rhs.AsNum()));
      break;
    }

    case OpCode::OP_EQ: {
      Value rhs = stack_.Pop();
      Value lhs = stack_.Pop();
      bool result = (lhs == rhs);
      stack_.Push(Value::Bool(result));  // 用Value类自己的重载==判断
      break;
    }

    case OpCode::OP_NOT_EQUAL: {
      Value rhs = stack_.Pop();
      Value lhs = stack_.Pop();
      bool result = (lhs != rhs);
      stack_.Push(Value::Bool(result));
      break;
    }

    case OpCode::OP_GREATER: {
      Value rhs = stack_.Pop();
      Value lhs = stack_.Pop();
      // 类型检查：只能比较数字
      assert(lhs.IsNum() && rhs.IsNum() && "OP_GREATER expects two numbers");

      bool result = (lhs.AsNum() > rhs.AsNum());
      stack_.Push(Value::Bool(result));
      break;
    }

    case OpCode::OP_LESS: {
      Value rhs = stack_.Pop();
      Value lhs = stack_.Pop();
      // 类型检查：只能比较数字
      assert(lhs.IsNum() && rhs.IsNum() && "OP_LESS expects two numbers");
      bool result = (lhs.AsNum() < rhs.AsNum());
      stack_.Push(Value::Bool(result));
      break;
    }

    case OpCode::OP_NOT: {
      Value v = stack_.Pop();
      assert(v.IsBool() && "OP_NOT expects a bool");
      bool result = (!v.AsBool());
      stack_.Push(Value::Bool(result));
      break;
    }

    case OpCode::OP_JUMP: {
      int target = ReadOpnd_();
      cf.ip = target;
      break;
    }

    case OpCode::OP_JUMP_IF_FALSE: {
      int target = ReadOpnd_();
      Value cond = stack_.Pop();
      assert(cond.IsBool() && "cond is not bool!");
      if (cond.AsBool() == false) {
        cf.ip = target;
      }
      break;
    }

    case OpCode::OP_PRINT: {
      Value v = stack_.Pop();
      std::cout << v.ToRepr() << std::endl;
      break;
    }

    case OpCode::OP_NEGATE: {
      Value v = stack_.Pop();
      assert(v.IsNum() && "Attempted to negate a non-number value");
      stack_.Push(Value::Num(-v.AsNum()));
      break;
    }

    case OpCode::OP_RETURN: {
      Value ret = stack_.Pop();  // 先弹出返回值；
      last_return_value_ = ret;
      // 保存当前frame
      bool return_instance = frames_.back().return_instance;
      Value instance_to_return = frames_.back().instance_to_return;

      frames_.pop_back();
      if (frames_
              .empty()) {  // 如果已经没有上层调用帧了，说明返回的是最外层函数
        CILLY_DLOG("Return value: " << ret.ToRepr() << "\n");  // 返回值
        return false;  // 结束整个 VM（返回 false）
      } else {         // 如果还有上层调用帧：
        if (return_instance) {
          ret = instance_to_return;  // 返回值修改成实例
        }
        stack_.Push(ret);  // 把返回值压回栈顶，留给调用者继续使用
        return true;       // 继续执行
      }
    }

    case OpCode::OP_LOAD_VAR: {
      int index = ReadOpnd_();
      assert(index >= 0 && index < static_cast<int>(cf.locals_.size()));
      stack_.Push(cf.locals_[index]);
      break;
    }

    case OpCode::OP_STORE_VAR: {
      int index = ReadOpnd_();
      assert(index >= 0 && index < static_cast<int>(cf.locals_.size()));
      Value v = stack_.Pop();
      cf.locals_[index] = v;
      break;
    }

      // 调用时把参数 Value 从调用者栈复制到被调函数的 frame 中
      // 这里是值拷贝（copy Value），未来当 Value 里有“对象引用”时，
      // 参数会共享底层堆对象，实现类似 Python 的引用语义。

    case OpCode::OP_CALL: {
      int func_index = ReadOpnd_();
      assert(func_index >= 0 && func_index < (int)callables_.size());

      const Callable& c = callables_[func_index];
      int argc = (c.type == Callable::Type::kNative) ? c.arity : c.fn->arity();

      std::vector<Value> argv(argc);
      for (int i = argc - 1; i >= 0; --i)
        argv[i] = stack_.Pop();

      DoCallByIndex(func_index, argc, argc ? argv.data() : nullptr);
      // 触发gc节点
      GcSafePoint_();
      break;
    }

    // 运行时调用函数
    case OpCode::OP_CALLV: {
      int argc = ReadI32_();
      std::vector<Value> argv(argc);
      for (int i = argc - 1; i >= 0; --i)
        argv[i] = stack_.Pop();

      Value callee = stack_.Pop();
      if (callee.IsCallable()) {
        DoCallByIndex(callee.AsCallable(), argc, argc ? argv.data() : nullptr);
        // 触发gc节点
        GcSafePoint_();
        break;
      }

      if (callee.IsClass()) {
        auto klass = callee.AsClass();

        std::shared_ptr<ObjDict> fields;
        if (gc_) {
          fields = gc::MakeShared<ObjDict>(*gc_);
        } else {
          fields = std::make_shared<ObjDict>();
        }

        std::shared_ptr<ObjInstance> instance;
        if (gc_)
          instance = gc::MakeShared<ObjInstance>(*gc_, klass);
        else
          instance = std::make_shared<ObjInstance>(klass);

        // 先创建实例
        Value inst_val = Value::Obj(instance);
        // 获取init的index

        auto init_index = klass->GetMethodIndex("init");
        if (init_index == -1) {
          assert(argc == 0 && "Class call without init supports 0 args only");
          stack_.Push(inst_val);
          // 触发gc节点
          GcSafePoint_();

          break;
        }
        // 传入this作为第一个参数
        std::vector<Value> argv2(argc + 1);
        argv2[0] = inst_val;
        for (int i = 0; i < argc; i++) {
          argv2[i + 1] = argv[i];
        }

        // debug
        CILLY_DLOG("[debug] ctor init_index="
                   << init_index << " argc_call=" << (argc + 1)
                   << " callee_arity=" << callables_[init_index].arity
                   << " callee_name=" << callables_[init_index].name << "\n");

        DoCallByIndex(init_index, argc + 1, argv2.data());
        // 触发gc节点
        GcSafePoint_();

        frames_.back().return_instance = true;
        frames_.back().instance_to_return = inst_val;
        break;
      }

      if (callee.IsBoundMethod()) {
        auto bm = callee.AsBoundMethod();

        std::vector<Value> argv2(argc + 1);
        argv2[0] = bm->Receiver();
        for (int i = 0; i < argc; i++)
          argv2[i + 1] = argv[i];

        DoCallByIndex(bm->Method(), argc + 1, argv2.data());
        // 触发gc节点
        GcSafePoint_();
        break;
      }
      assert(false && "Callee is not callable/class/boundmethod");
      break;
    }

    // List相关
    // 创建新列表
    case OpCode::OP_LIST_NEW: {
      auto list = std::make_shared<ObjList>();
      if (gc_)
        list = gc::MakeShared<ObjList>(*gc_);
      stack_.Push(Value::Obj(list));

      // 触发gc节点
      GcSafePoint_();
      break;
    }

    // 在list中加入数据
    case OpCode::OP_LIST_PUSH: {
      Value value = stack_.Pop();
      Value list_v = stack_.Pop();
      auto list = list_v.AsList();

      std::size_t old_bytes = 0;
      if (gc_)
        old_bytes = list->SizeBytes();

      list->Push(value);
      if (gc_) {
        std::size_t new_bytes = list->SizeBytes();
        if (new_bytes != old_bytes) {
          gc_->AddHeapBytesDelta(static_cast<std::ptrdiff_t>(new_bytes) -
                                 static_cast<std::ptrdiff_t>(old_bytes));
        }
      }
      stack_.Push(list_v);

      // 触发gc节点
      GcSafePoint_();
      break;
    }

      //// 获取对应索引的数据
      // case OpCode::OP_LIST_GET : {
      //   Value index_v = stack_.Pop();
      //   assert(index_v.IsNum() && "索引输入错误！");
      //   int index = static_cast<int>(index_v.AsNum());
      //   Value list_v = stack_.Pop();
      //   auto list = list_v.AsList();
      //   const Value& elem = list->At(index);
      //   stack_.Push(elem);
      //   break;
      // }

      // 替换/覆盖对应索引位置的数据
      /*  case OpCode::OP_LIST_SET : {
          Value value = stack_.Pop();
          Value index_v = stack_.Pop();
          assert(index_v.IsNum() && "索引输入错误！");
          int index = static_cast<int>(index_v.AsNum());
          Value list_v = stack_.Pop();
          auto list = list_v.AsList();
          list->Set(index, value);
          break;
        }*/

    // 返回list长度
    case OpCode::OP_LIST_LEN: {
      Value list_v = stack_.Pop();
      auto list = list_v.AsList();
      stack_.Push(Value::Num(list->Size()));
      break;
    }

    // Dict相关
    // 创建新Dict
    case OpCode::OP_DICT_NEW: {
      auto dict = std::make_shared<ObjDict>();
      if (gc_)
        dict = gc::MakeShared<ObjDict>(*gc_);
      stack_.Push(Value::Obj(dict));
      // 触发gc节点
      GcSafePoint_();
      break;
    }

      // 将关键字为key的索引内容替换，如果未添加该关键字则创建
      /*case OpCode::OP_DICT_SET: {
        Value value = stack_.Pop();
        Value key_v = stack_.Pop();
        assert(key_v.IsStr() && "关键词输入错误！");
        std::string key = key_v.AsStr();
        Value dict_v = stack_.Top();
        auto dict = dict_v.AsDict();
        dict->Set(key, value);
        break;
      }*/

      /*case OpCode::OP_DICT_GET: {
        Value key_v = stack_.Pop();
        assert(key_v.IsStr() && "关键词输入错误！");
        std::string key = key_v.AsStr();
        Value dict_v = stack_.Pop();
        auto dict = dict_v.AsDict();
        const Value& elem = dict->Get(key);
        stack_.Push(elem);
        break;
      }*/

    case OpCode::OP_DICT_HAS: {
      Value key_v = stack_.Pop();
      assert(key_v.IsStr() && "关键词输入错误！");
      std::string key = key_v.AsStr();
      Value dict_v = stack_.Pop();
      auto dict = dict_v.AsDict();
      bool exists = dict->Has(key);
      stack_.Push(Value::Bool(exists));
      break;
    }
    case OpCode::OP_INDEX_GET: {
      Value index_v = stack_.Pop();
      Value object_v = stack_.Pop();
      switch (object_v.AsObj()->Type()) {
        case ObjType::kList: {
          assert(index_v.IsNum() && "索引输入错误！");
          int index = static_cast<int>(index_v.AsNum());
          auto list = object_v.AsList();
          const Value& elem = list->At(index);
          stack_.Push(elem);
          break;
        }
        case ObjType::kDict: {
          assert(index_v.IsStr() && "索引输入错误！");
          std::string index = static_cast<std::string>(index_v.AsStr());
          auto dict = object_v.AsDict();
          const Value& elem = dict->Get(index);
          stack_.Push(elem);
          break;
        }
        default:
          assert(false && "未找到此类型！");
          break;
      }
      break;
    }
    case OpCode::OP_INDEX_SET: {
      Value value = stack_.Pop();
      Value index_v = stack_.Pop();
      Value object_v = stack_.Pop();
      switch (object_v.AsObj()->Type()) {
        case ObjType::kList: {
          assert(index_v.IsNum() && "索引输入错误！");
          int index = static_cast<int>(index_v.AsNum());
          auto list = object_v.AsList();
          list->Set(index, value);
          break;
        }
        case ObjType::kDict: {
          assert(index_v.IsStr() && "关键词输入错误！");
          std::string key = index_v.AsStr();
          auto dict = object_v.AsDict();

          std::size_t old_bytes = 0;
          if (gc_)
            old_bytes = dict->SizeBytes();

          dict->Set(key, value);

          if (gc_) {
            std::size_t new_bytes = dict->SizeBytes();
            if (new_bytes != old_bytes) {
              gc_->AddHeapBytesDelta(static_cast<std::ptrdiff_t>(new_bytes) -
                                     static_cast<std::ptrdiff_t>(old_bytes));
            }
          }

          // 触发gc节点
          GcSafePoint_();
          break;
        }
        default:
          assert(false && "未找到该类型变量！");
          break;
      }
      break;
    }

    case OpCode::OP_GET_PROP: {
      int name_index = ReadOpnd_();
      Value name_v = ch.ConstAt(name_index);
      assert(name_v.IsStr() && "暂时不支持其他类型");
      const std::string name = name_v.AsStr();

      Value obj = stack_.Pop();
      assert(obj.IsObj() && "暂时不支持其他类型");
      switch (obj.AsObj()->Type()) {
        case ObjType::kDict: {
          auto dict = obj.AsDict();
          stack_.Push(dict->Get(name));
          break;
        }
        case ObjType::kInstance: {
          auto instance = obj.AsInstance();

          // 先找field
          if (instance->Fields()->Has(name)) {
            stack_.Push(instance->Fields()->Get(name));
            break;
          }
          int32_t method_index = instance->Klass()->GetMethodIndex(name);
          if (method_index >= 0) {
            std::shared_ptr<ObjBoundMethod> bm;
            if (gc_)
              bm = gc::MakeShared<ObjBoundMethod>(*gc_, obj, method_index);
            else
              bm = std::make_shared<ObjBoundMethod>(obj, method_index);
            stack_.Push(Value::Obj(bm));

            // 触发gc节点
            GcSafePoint_();

          } else {
            stack_.Push(Value::Null());

            // 触发gc节点
            GcSafePoint_();
          }
          break;
        }
        default:
          assert(false && "GET_PROP only supports dict for now.");
      }
      break;
    }

    case OpCode::OP_SET_PROP: {
      int name_index = ReadOpnd_();
      Value name_v = ch.ConstAt(name_index);
      assert(name_v.IsStr() && "暂时不支持其他类型");
      const std::string name = name_v.AsStr();

      Value value = stack_.Pop();
      Value obj = stack_.Pop();
      assert(obj.IsObj() && "暂时不支持其他类型");
      switch (obj.AsObj()->Type()) {
        case ObjType::kDict: {
          auto dict = obj.AsDict();

          std::size_t old_bytes = 0;
          if (gc_)
            old_bytes = dict->SizeBytes();

          dict->Set(name, value);

          if (gc_) {
            std::size_t new_bytes = dict->SizeBytes();
            if (new_bytes != old_bytes) {
              gc_->AddHeapBytesDelta(static_cast<std::ptrdiff_t>(new_bytes) -
                                     static_cast<std::ptrdiff_t>(old_bytes));
            }
          }

          // 触发gc节点
          GcSafePoint_();
          break;
        }
        case ObjType::kInstance: {
          auto instance = obj.AsInstance();
          instance->Fields()->Set(name, value);
          break;
        }
        default:
          assert(false && "SET_PROP only supports dict for now.");
      }
      break;
    }

    case OpCode::OP_GET_SUPER: {
      int super_idx = ReadI32_();
      int name_index = ReadI32_();

      Value receiver = stack_.Pop();
      Value super_v = CurrentFrame().fn->chunk().ConstAt(super_idx);
      Value name_v = CurrentFrame().fn->chunk().ConstAt(name_index);

      assert(receiver.IsInstance() && "super receiver must be instance");
      assert(super_v.IsClass() && "super const must be class");
      assert(name_v.IsStr() && "super name must be string");

      auto super_cls = super_v.AsClass();
      std::string name = name_v.AsStr();

      int method_idx = super_cls->GetMethodIndex(name);
      assert(method_idx >= 0 && "Undefined superclass method.");
      std::shared_ptr<ObjBoundMethod> bm;
      if (gc_)
        bm = gc::MakeShared<ObjBoundMethod>(*gc_, receiver, method_idx);
      else
        bm = std::make_shared<ObjBoundMethod>(receiver, method_idx);
      stack_.Push(Value::Obj(bm));

      // 触发gc节点
      GcSafePoint_();

      break;
    }
    default:
      assert(false && "没有相关命令（未知或未实现的 OpCode）");
      break;
  }
  return true;  // 当前没有函数调用栈，持续执行到字节码末尾
}

CallFrame& VM::CurrentFrame() {
  return frames_.back();
}

const CallFrame& VM::CurrentFrame() const {
  return frames_.back();
}

void VM::CollectGarbage() {
  if (!gc_)
    return;
  gc_->CollectWithRoots([this](gc::Collector& c) { this->TraceRoots(c); });
}

void VM::SetNextGcBytesThresholdForTest(std::size_t bytes) {
  next_gc_bytes_threshold_ = bytes;
}

void VM::TraceRoots(gc::Collector& c) const {
  // 工具：标记一个Value（只有对象类型才需要标记）
  auto mark_value = [&c](const Value& v) {
    if (v.IsObj()) {
      // Mark只接受罗指针（GcObject*）
      c.Mark(v.AsObj().get());
    }
  };

  // 1.VM操作数栈是最重要的roots,脚本执行过程中几乎所有临时值都在栈上
  for (const auto& v : stack_.values()) {
    mark_value(v);
  }

  // 2.每个CallFrame 的 locals 也必须扫描
  // - 方法调用的 this、局部变量、临时变量可能保存在 locals 里
  // - 不扫描会导致函数执行中对象被误回收
  for (const auto& f : frames_) {
    for (const auto& v : f.locals_) {
      mark_value(v);
    }

    // init 构造链会用到 instance_to_return，保险起见也将其标记
    mark_value(f.instance_to_return);
  }

  // 3.最近一次的返回值（暂时保守处理）
  mark_value(last_return_value_);

  // 4.保守扫描：把已知函数的 Chunk 常量池也当 roots
  // 说明：
  // - 常量池里可能放 class、string、函数对象等
  // - 暂时为了避免 “常量池对象被扫掉导致后续加载崩溃”，先整体扫描
  auto trace_consts = [&](const Function* fn) {
    if (!fn)
      return;
    const Chunk& ch = fn->chunk();
    for (int i = 0; i < ch.ConstSize(); i++) {
      mark_value(ch.ConstAt(i));
    }
  };

  // 扫描当前活跃的frames对应的函数常量池
  for (const auto& f : frames_) {
    trace_consts(f.fn);
  }

  for (const auto& callee : callables_) {
    if (callee.type != Callable::Type::kBytecode)
      continue;
    const Function* fn = callee.fn;
    if (!fn)
      continue;
    const Chunk& ch = fn->chunk();
    for (int i = 0; i < ch.ConstSize(); ++i) {
      const Value& v = ch.ConstAt(i);
      if (v.IsObj())
        c.Mark(v.AsObj().get());
    }
  }
}

void VM::MaybeCollectGarbage_() {
  if (!gc_)
    return;

  const bool hit_object_budget = (gc_->object_count() >= next_gc_threshold_);
  const bool hit_bytes_budget = (gc_->heap_bytes() >= next_gc_bytes_threshold_);

  if (!hit_object_budget && !hit_bytes_budget)
    return;

  // 触发一次GC
  CollectGarbage();

  // ---- GC 后更新两个阈值，避免频繁抖动 ----
  // 1.对象阈值：沿用你原来的策略（live*2+64，且下限256）
  {
    std::size_t live = gc_->object_count();
    std::size_t next = live * 2 + 64;
    if (next < 256)
      next = 256;
    next_gc_threshold_ = next;
  }

  // 2.字节阈值：按当前存活字节数扩大（live_bytes*2），并给个下限
  {
    std::size_t live_bytes = gc_->heap_bytes();
    std::size_t next_bytes = live_bytes * 2;

    // 下限：避免脚本很小导致 bytes 阈值过低而频繁 GC
    if (next_bytes < 16 * 1024)
      next_bytes = 16 * 1024;

    next_gc_bytes_threshold_ = next_bytes;
  }
}

void VM::GcSafePoint_() {
  if (!gc_)
    return;
  MaybeCollectGarbage_();
}

int VM::RegisterFunction(const Function* fn) {
  Callable f;
  f.type = Callable::Type::kBytecode;
  f.arity = fn->arity();
  f.name = fn->name();
  f.fn = fn;
  callables_.push_back(std::move(f));
  return static_cast<int>(callables_.size() - 1);
}

int VM::RegisterNative(const std::string& name, int arity, NativeFn fn) {
  Callable nf;
  nf.type = Callable::Type::kNative;
  nf.arity = arity;
  nf.name = name;
  nf.native = std::move(fn);

  callables_.push_back(std::move(nf));
  return static_cast<int>(callables_.size() - 1);
}

// 迷你反汇编器：打印每条指令及其操作数与行号。
void DisassembleChunk(const Chunk& chunk) {
  for (int i = 0; i < chunk.CodeSize(); ++i) {
    int ip = i;  // 记录本条“指令”的位置（避免 OP_CONSTANT 消耗操作数后丢位）
    int32_t word = chunk.CodeAt(i);
    OpCode op = static_cast<OpCode>(word);

    std::cout << ip << " : ";

    switch (op) {
      case OpCode::OP_CONSTANT: {
        int index = chunk.CodeAt(++i);  // 消耗 1 个操作数
        std::cout << "OP_CONSTANT " << index << " ("
                  << chunk.ConstAt(index).ToRepr() << ")";
        break;
      }
      case OpCode::OP_ADD: {
        std::cout << "OP_ADD";
        break;
      }
      case OpCode::OP_PRINT: {
        std::cout << "OP_PRINT";
        break;
      }
      case OpCode::OP_NEGATE: {
        std::cout << "OP_NEGATE";
        break;
      }
      case OpCode::OP_RETURN: {
        std::cout << "OP_RETURN";
        break;
      }
      default: {
        std::cout << "Unknown instruction";
        break;
      }
    }

    // 打印本条“指令”的源码行号,不是操作数的行号
    std::cout << " [line " << chunk.LineAt(ip) << "]" << std::endl;
  }
}

}  // namespace cilly
