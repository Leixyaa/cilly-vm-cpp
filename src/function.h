#ifndef CILLY_VM_CPP_FUNCTION_H_
#define CILLY_VM_CPP_FUNCTION_H_

#include <memory>
#include <string>
#include "chunk.h"

namespace cilly {

// 函数对象：封装一段可执行字节码（Chunk）及其元信息。
class Function {
 public:
  // 构造与生命周期 

  // 默认：name="script"，arity=0，内部创建一个空的 Chunk
  Function();

  // 自定义名称与形参数量；Chunk 仍由内部创建。
  explicit Function(std::string name, int arity);

  // 禁止拷贝（函数对象较重，避免意外复制）
  Function(const Function&) = delete;
  Function& operator=(const Function&) = delete;

  // 允许移动（便于容器或返回值传递）
  Function(Function&&) noexcept;
  Function& operator=(Function&&) noexcept;

  ~Function();

  // 访问器
  const std::string& name() const;
  int arity() const;

  // 访问底层 Chunk  读写版和只读版
  Chunk& chunk();
  const Chunk& chunk() const;

  // 便捷转发：直接写入字节码或常量（内部转发到 chunk_）。
  void Emit(OpCode op, int src_line);
  void EmitI32(int32_t v, int src_line);
  int AddConst(const Value& v);

 private:
  std::string name_;
  int arity_ = 0;
  std::unique_ptr<Chunk> chunk_;  // 唯一拥有字节码块
};

}  // namespace cilly

#endif  // CILLY_VM_CPP_FUNCTION_H_
