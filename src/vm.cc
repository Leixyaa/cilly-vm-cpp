#include "vm.h"

#include <cassert>
#include <iostream>

namespace cilly {

VM::VM() = default;

void VM::Run(const Function& fn) {
  const Chunk& ch = fn.chunk();
  stack_.ResetStats();
  ip_ = 0;

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
      // 先弹右操作数，再弹左操作数,便于将来支持非交换运算。
      Value rhs = stack_.Pop();
      Value lhs = stack_.Pop();
      assert(lhs.IsNum() && rhs.IsNum());
      stack_.Push(Value::Num(lhs.AsNum() + rhs.AsNum()));
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
      Value v = stack_.Pop();
      std::cout << "Return value: " << v.ToRepr() << std::endl;
      return false;  
    }

    default:
      assert(false && "没有相关命令（未知或未实现的 OpCode）");
  }
  return true;  // 当前没有 OP_RETURN：持续执行直到字节码结束
}

}  // namespace cilly
