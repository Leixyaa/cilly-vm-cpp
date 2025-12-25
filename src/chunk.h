#ifndef CILLY_VM_CPP_CHUNK_H_
#define CILLY_VM_CPP_CHUNK_H_

// chunk是可执行字节码的容器，保存指令序列、常量池和行号信息

#include <cstdint>
#include <vector>

#include "bytecode_stream.h"
#include "opcodes.h"
#include "value.h"

namespace cilly {

// 一段可执行字节码：指令序列 + 常量池 + 行号信息
class Chunk {
 public:
  // 添加一条无操作数指令
  void Emit(OpCode op, int src_line);

  // 添加一个 32 位整数操作数,紧随指令后
  void EmitI32(int32_t v, int src_line);

  // 向常量池添加一个常量，并返回它的索引。
  int AddConst(const Value& v);

  // 占位回填
  void PatchI32(int index, int32_t value);

  // 查询接口
  int CodeSize() const;                   // code_ 元素个数（含指令和操作数）
  int ConstSize() const;                  // 常量数量
  int32_t CodeAt(int index) const;        // 获取指定位置的指令或操作数
  const Value& ConstAt(int index) const;  // 读取常量池中的值
  int LineAt(int index) const;            // 获取行号

  void Save(BytecodeWriter& writer) const;  // 将整个 Chunk 写入二进制流
  static Chunk Load(
      BytecodeReader& reader);  // 从二进制流中读出一个 Chunk（静态工厂函数）

 private:
  std::vector<int32_t> code_;       // 指令序列（包含操作数）
  std::vector<Value> const_pool_;   // 常量池
  std::vector<int32_t> line_info_;  // 每个 code 元素的源代码行号
};

}  // namespace cilly

#endif  // CILLY_VM_CPP_CHUNK_H_
