#include "vm.h"

#include <cassert>
#include <iostream>

namespace cilly {

VM::VM() = default;

void VM::Run(const Function& fn) {
  const Chunk& ch = fn.chunk();
  stack_.ResetStats();
  ip_ = 0;

  locals_.assign(fn.LocalCount(), Value::Null());  //初始化locals变量表

  while (ip_ < ch.CodeSize()) {
    bool cont = Step_(fn);
    if (!cont) break;
  }
}

int32_t VM::ReadI32_(const Chunk& ch) {
  assert(ip_ >= 0 && ip_ < ch.CodeSize());
  return ch.CodeAt(ip_++);
}

int32_t VM::ReadOpnd_(const Chunk& ch) {
  return ReadI32_(ch);
}

int VM::PushCount() const { return stack_.PushCount(); }

int VM::PopCount() const { return stack_.PopCount(); }

int VM::Depth() const { return stack_.Depth(); }

int VM::MaxDepth() const { return stack_.MaxDepth(); }

bool VM::Step_(const Function& fn) {
  const Chunk& ch = fn.chunk();
  int32_t raw = ReadI32_(ch);
  OpCode op = static_cast<OpCode>(raw);

  switch (op) {
    case OpCode::OP_CONSTANT: {
      int index = ReadOpnd_(ch);
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
    
    case OpCode::OP_PRINT: {
      Value v = stack_.Top();
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
      Value v = stack_.Pop();
      std::cout << "Return value: " << v.ToRepr() << std::endl;
      return false;  // 终止执行
    }

    case OpCode::OP_LOAD_VAR: {
      int index = ReadOpnd_(ch);
      assert(index >= 0 && index < locals_.size());
      stack_.Push(locals_[index]);
      break;
    }

    case OpCode::OP_STORE_VAR: {
      int index = ReadOpnd_(ch);
      assert(index >= 0 && index < locals_.size());
      Value v = stack_.Pop();
      locals_[index] = v;
      break;
    }
    

    default:
      assert(false && "没有相关命令（未知或未实现的 OpCode）");
  }

  return true;  // 当前没有函数调用栈，持续执行到字节码末尾
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
