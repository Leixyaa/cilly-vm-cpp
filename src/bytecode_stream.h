#ifndef CILLY_BYTECODE_STREAM_H_
#define CILLY_BYTECODE_STREAM_H_

#include <fstream>
#include <string>
#include <type_traits>  // 用于 std::is_trivially_copyable
#include <vector>

namespace cilly {

// 写入器：负责把内存里的数据变成磁盘上的二进制
class BytecodeWriter {
 public:
  explicit BytecodeWriter(const std::string& path) {
    // ios::binary 是关键，防止 Windows 下 \n 被自动转换
    out_.open(path, std::ios::out | std::ios::binary | std::ios::trunc);
  }

  bool IsOpen() const { return out_.is_open(); }

  // ** 模板函数：可以写入 int, double, bool, enum 等基础类型
  template <typename T>
  void Write(const T& value) {
    // 编译期检查：防止误传入 std::string 等复杂对象导致崩溃
    static_assert(std::is_trivially_copyable_v<T>, "Only basic types allowed!");

    // reinterpret_cast 将变量的地址强转为 char* 指针，直接写入内存字节
    // 没有改变 value 的类型，相当于给value的地址贴上“char *”
    // 假地址标签，地址还是那个地址
    out_.write(reinterpret_cast<const char*>(&value), sizeof(T));
  }

  // 字符串不能直接写内存，必须自定义：先写长度，再写内容
  void WriteString(const std::string& s) {
    int32_t len = static_cast<int32_t>(s.size());
    Write(len);  // 1. 先存长度
    if (len > 0) {
      out_.write(s.data(), len);  // 2. 再存内容
    }
  }

 private:
  std::ofstream out_;
};

// 读取器：负责把磁盘上的二进制还原回内存
class BytecodeReader {
 public:
  explicit BytecodeReader(const std::string& path) {
    in_.open(path, std::ios::in | std::ios::binary);
  }

  bool IsOpen() const { return in_.is_open(); }

  // ** 模板函数：读取基础类型
  template <typename T>
  T Read() {
    static_assert(std::is_trivially_copyable_v<T>, "Only basic types allowed!");
    T value;
    in_.read(reinterpret_cast<char*>(&value), sizeof(T));
    return value;
  }

  // 读取字符串：先读长度，再分配内存读内容
  std::string ReadString() {
    int32_t len = Read<int32_t>();
    if (len == 0)
      return "";

    std::string s;
    s.resize(len);  // 预分配空间
    in_.read(s.data(), len);
    return s;
  }

 private:
  std::ifstream in_;
};

}  // namespace cilly

#endif  // CILLY_BYTECODE_STREAM_H_