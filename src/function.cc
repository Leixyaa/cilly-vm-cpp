#include "function.h"

#include <cassert>
#include <utility>

namespace cilly {

Function::Function()
    : name_("script"), arity_(0), chunk_(std::make_unique<Chunk>()) {}

Function::Function(std::string name, int arity)
    : name_(std::move(name)), arity_(arity), chunk_(std::make_unique<Chunk>()) {}

Function::Function(Function&& other) noexcept
    : name_(std::move(other.name_)),
      arity_(other.arity_),
      chunk_(std::move(other.chunk_)) {}

Function& Function::operator=(Function&& other) noexcept {
  if (this != &other) {
    name_ = std::move(other.name_);
    arity_ = other.arity_;
    chunk_ = std::move(other.chunk_);
    // 可选：清理 other 的可观察状态
    other.arity_ = 0;
  }
  return *this;
}

Function::~Function() = default;

const std::string& Function::name() const { return name_; }

int Function::arity() const { return arity_; }

Chunk& Function::chunk() { return *chunk_; }

const Chunk& Function::chunk() const { return *chunk_; }

void Function::Emit(OpCode op, int src_line) { chunk_->Emit(op, src_line); }

void Function::EmitI32(int32_t v, int src_line) { chunk_->EmitI32(v, src_line); }

int Function::AddConst(const Value& v) { return chunk_->AddConst(v); }

int Function::CodeSize() const { return chunk_ -> CodeSize(); }

void Function::SetLocalCount(int count){
  load_count_ = count;
}

int Function::LocalCount() const{
    return load_count_;
}

void Function::PatchI32(int index, int32_t value) {
  chunk_ -> PatchI32(index, value);
}

}  // namespace cilly
