#include "function.h"

#include <utility>

namespace cilly {

Function::Function()
    : name_("script"), arity_(0), chunk_(std::make_unique<Chunk>()) {}

Function::Function(std::string name, int arity)
    : name_(std::move(name)),
      arity_(arity),
      chunk_(std::make_unique<Chunk>()) {}

Function::Function(Function&& other) noexcept
    : name_(std::move(other.name_)),
      arity_(other.arity_),
      chunk_(std::move(other.chunk_)),
      local_count_(other.local_count_) {}

Function& Function::operator=(Function&& other) noexcept {
  if (this != &other) {
    name_ = std::move(other.name_);
    arity_ = other.arity_;
    chunk_ = std::move(other.chunk_);
    local_count_ = other.local_count_;
    other.arity_ = 0;
    other.local_count_ = 0;
  }
  return *this;
}

Function::~Function() = default;

const std::string& Function::name() const { return name_; }

int Function::arity() const { return arity_; }

Chunk& Function::chunk() { return *chunk_; }

const Chunk& Function::chunk() const { return *chunk_; }

void Function::Emit(OpCode op, int src_line) {
  chunk_->Emit(op, src_line);
}

void Function::EmitI32(int32_t v, int src_line) {
  chunk_->EmitI32(v, src_line);
}

int Function::AddConst(const Value& v) {
  return chunk_->AddConst(v);
}

int Function::CodeSize() const {
  return chunk_->CodeSize();
}

int Function::ConstSize() const {
  return chunk_->ConstSize();
}

std::string Function::Name() const {
    return name_;
}

void Function::SetLocalCount(int count) {
  local_count_ = count;
}

int Function::LocalCount() const {
  return local_count_;
}

void Function::PatchI32(int index, int32_t value) {
  chunk_->PatchI32(index, value);
}

void Function::Save(BytecodeWriter& writer) const {
  // 写入函数名
  writer.WriteString(name_);

  // 写入参数个数（arity_）
  // 之所以用 int32_t，是为了在文件格式中固定宽度，跨平台更稳定
  writer.Write<int32_t>(static_cast<int32_t>(arity_));

  // 写入局部变量总数（包含参数 + 普通局部变量）
  writer.Write<int32_t>(static_cast<int32_t>(local_count_));

  // 写入函数体字节码（Chunk）
  if (chunk_) {
    chunk_->Save(writer);
  }
}


Function Function::Load(BytecodeReader& reader) {
  // 读取函数名
  Function fn;
  fn.name_ = reader.ReadString();

  // 读取参数个数
  fn.arity_ = static_cast<int>(reader.Read<int32_t>());

  // 读取局部变量总数
  fn.local_count_ = static_cast<int>(reader.Read<int32_t>());

  // 读取函数体字节码（Chunk）
  Chunk loaded_chunk = Chunk::Load(reader);
  fn.chunk_ = std::make_unique<Chunk>(std::move(loaded_chunk));

  // 返回完整构造好的 Function
  return fn;
}


}  // namespace cilly
