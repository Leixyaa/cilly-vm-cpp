#include "chunk.h"
#include <cassert>

namespace cilly {

void Chunk::Emit(OpCode op, int src_line) {
  // C++ 强枚举不能自动把类型值转换为整型，需要显式转换。
  code_.push_back(static_cast<int32_t>(op));
  line_info_.push_back(src_line);
}

void Chunk::EmitI32(int32_t v, int src_line) {
  code_.push_back(v);
  line_info_.push_back(src_line);
}

int Chunk::AddConst(const Value& v) {
  const_pool_.push_back(v);
  int idx = static_cast<int>(const_pool_.size());
  return idx - 1;
}

void Chunk::PatchI32(int index, int32_t value) {
  assert(index >= 0 && index < static_cast<int>(code_.size()));
  code_[index] = value; 
}

int Chunk::CodeSize() const {
  return static_cast<int>(code_.size());
}

int Chunk::ConstSize() const {
  return static_cast<int>(const_pool_.size());
}

int32_t Chunk::CodeAt(int index) const {
  assert(index >= 0 && index < static_cast<int>(code_.size()));
  return code_[index];
}

const Value& Chunk::ConstAt(int index) const {
  assert(index >= 0 && index < static_cast<int>(const_pool_.size()));
  return const_pool_[index];
}

int Chunk::LineAt(int index) const {
  assert(index >= 0 && index < static_cast<int>(line_info_.size()));
  return line_info_[index];
}



}  // namespace cilly
