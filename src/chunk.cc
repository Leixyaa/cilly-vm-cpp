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

void Chunk::Save(BytecodeWriter& writer) const {
  // 保存常量池大小
  int32_t const_pool_size = static_cast<int32_t>(ConstSize());
  writer.Write<int32_t>(const_pool_size);

  // 按顺序保存每一个常量 Value
  for (int32_t i = 0; i < const_pool_size; ++i) {
    ConstAt(i).Save(writer);
  }

  // 保存指令序列长度（code_ 中元素个数）
  int32_t code_size = static_cast<int32_t>(CodeSize());
  writer.Write<int32_t>(code_size);

  // 依次保存每一个指令或操作数（都是 int32_t）
  for (int32_t i = 0; i < code_size; ++i) {
    writer.Write<int32_t>(CodeAt(i));
  }

  // 保存行号表长度（当前约定 == code_size）
  int32_t line_info_size = code_size;
  writer.Write<int32_t>(line_info_size);

  // 依次保存与每个 code 元素对应的源码行号
  for (int32_t i = 0; i < line_info_size; ++i) {
    writer.Write<int32_t>(LineAt(i));
  }
}

Chunk Chunk::Load(BytecodeReader& reader) {
  Chunk chunk;

  // 读取常量池大小，并按顺序恢复每一个 Value
  int32_t const_pool_size = reader.Read<int32_t>();
  for (int32_t i = 0; i < const_pool_size; ++i) {
    // 保持与正常编译时 AddConst 的行为一致
    chunk.AddConst(Value::Load(reader));
  }

  // 读取指令序列长度，并恢复每一个 int32_t 指令/操作数
  int32_t code_size = reader.Read<int32_t>();
  chunk.code_.resize(code_size);
  for (int32_t i = 0; i < code_size; ++i) {
    chunk.code_[i] = reader.Read<int32_t>();
  }

  // 读取行号表长度，并恢复每一个行号
  int32_t line_info_size = reader.Read<int32_t>();
  chunk.line_info_.resize(line_info_size);
  for (int32_t i = 0; i < line_info_size; ++i) {
    chunk.line_info_[i] = reader.Read<int32_t>();
  }

  return chunk;
}

}  // namespace cilly
