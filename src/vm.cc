#include "vm.h"

#include <cassert>
#include <iostream>

namespace cilly {

VM::VM() = default;

void VM::Run(const Function& fn) {
  frames_.clear();
  CallFrame frame;
  frame.fn = &fn;
  frame.ip = 0;
  frame.ret_ip = -1;   // 主函数没有返回地址
  frame.locals_.assign(fn.LocalCount(), Value::Null());  //初始化locals变量表
  frames_.push_back(frame);

  // 每次运行前清空栈
  stack_.Clear();

  while (true) {
    CallFrame& cf = CurrentFrame();
    const Chunk& ch = cf.fn->chunk();

    if (cf.ip >= ch.CodeSize()) {
      // 当前帧到头了：对主函数来说可以结束；
      // 后面支持多帧时，可以决定是报错还是自动返回。
      break;
    }

    bool cont = Step_();  // 注意：这里先改成不传 fn
    if (!cont) break;
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

int VM::PushCount() const { return stack_.PushCount(); }

int VM::PopCount() const { return stack_.PopCount(); }

int VM::Depth() const { return stack_.Depth(); }

int VM::MaxDepth() const { return stack_.MaxDepth(); }

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
    
    case OpCode::OP_DUP: {
      Value v = stack_.Top();  
      stack_.Push(v);      
      break;
    }

    case OpCode::OP_CONSTANT : {
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
      if(cond.AsBool() == false) {
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
      Value ret = stack_.Pop();          // 先弹出返回值；
      frames_.pop_back();
      if(frames_.empty()) {              // 如果已经没有上层调用帧了，说明返回的是最外层函数
        std::cout << "Return value: " << ret.ToRepr() << std::endl;  // 打印返回值
        return false;                    // 结束整个 VM（返回 false）
      }
      else {                             // 如果还有上层调用帧：
        stack_.Push(ret);                // 把返回值压回栈顶，留给调用者继续使用
        return true;                     // 继续执行
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
      // 读取要调用的函数 ID（
      int func_index = ReadOpnd_();
      assert(func_index >= 0 && func_index < static_cast<int>(functions_.size()));

      const Function* callee = functions_[func_index];
      assert(callee != nullptr);

      int argc = callee -> arity();
      assert(argc >= 0);

      // 为被调用函数创建一个新的调用帧
      CallFrame frame;
      frame.fn = callee; 
      frame.ip = 0;       // 被调用函数从头开始执行
      frame.ret_ip = -1;  // 暂时不用，后续如有跳转再扩展

      int local_count = callee->LocalCount();
      frame.locals_.assign(local_count, Value::Null());

      for (int i = argc - 1; i >= 0; i--) {
        Value arg = stack_.Pop();
        assert(i < static_cast<int>(frame.locals_.size()));
        frame.locals_[i] = arg;
      }

      frames_.push_back(std::move(frame));
      break;
    }
    

    // List相关
    // 创建新列表
    case OpCode::OP_LIST_NEW: {
      auto list = std::make_shared<ObjList>();
      stack_.Push(Value::Obj(list));
      break;
    }
    
    // 在list中加入数据
    case OpCode::OP_LIST_PUSH : {
      Value value = stack_.Pop();
      Value list_v = stack_.Pop();
      auto list = list_v.AsList();
      list->Push(value);
      stack_.Push(list_v);
      break;
    }
      
    //// 获取对应索引的数据
    //case OpCode::OP_LIST_GET : {
    //  Value index_v = stack_.Pop();
    //  assert(index_v.IsNum() && "索引输入错误！");
    //  int index = static_cast<int>(index_v.AsNum());
    //  Value list_v = stack_.Pop();
    //  auto list = list_v.AsList();
    //  const Value& elem = list->At(index);
    //  stack_.Push(elem);
    //  break;
    //}
   
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
      stack_.Push(Value::Obj(dict));
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
          dict->Set(key, value);
          break;;
        }
        default:
          assert(false && "未找到该类型变量！");
          break;
      }
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

int VM::RegisterFunction(const Function* fn) {
  functions_.push_back(fn);
  return static_cast<int> (functions_.size() - 1);
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
        std::cout << "OP_CONSTANT " << index
                  << " (" << chunk.ConstAt(index).ToRepr() << ")";
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
