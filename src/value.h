// cilly-vm-cpp
// Author: Leixyaa
// Date: 11.6

//value.h

// Value 语义说明（当前版本）
//
// - Value 是一个小型动态类型容器，内部用 std::variant 存放：
//   - null / bool / double / std::string 等基础类型
// - 在当前实现中：
//   - 所有 Value 都是值类型（copy 时会复制内部的数据，例如 double、string）
//   - 虚拟机中的栈、局部变量、函数参数，都是按值传递 Value
// - 未来扩展：
//   - 我们会在 Value 中加入“堆对象引用”（例如 list / dict）
//   - 对这些堆对象采用“引用语义”：多个 Value 可以共享同一个堆上的对象
//   - 这样就能实现：修改 list / dict 的内容能在多个变量之间共享


#ifndef CILLY_VM_CPP_VALUE_H_
#define CILLY_VM_CPP_VALUE_H_

#include <string>
#include <variant>
#include <memory> 
#include "bytecode_stream.h"

namespace cilly {

// 前向声明堆对象结构
class Object;
class ObjList;
class ObjString;
class ObjDict;

// 运行时值的类型枚举。
// 用于标识当前 Value 中存放的数据类型。
enum class ValueType { kNull, kBool, kNum, kStr, kObj};

// 运行时的通用值类型。
// 可存放 null、bool、number、string 四种数据。
class Value {
 public:
  // 构造与工厂方法
  Value();  // 默认构造成 Null。
  Value(ValueType x, std::variant<std::monostate, bool, double, std::string, std::shared_ptr<Object>> y);
  static Value Null();
  static Value Bool(bool b);
  static Value Num(double d);
  static Value Str(std::string s);
  static Value Obj(std::shared_ptr<ObjList> object);
  static Value Obj(std::shared_ptr<ObjString> object);
  static Value Obj(std::shared_ptr<ObjDict> object);

  // 类型判断
  ValueType type() const;
  bool IsNull() const;
  bool IsBool() const;
  bool IsNum() const;
  bool IsStr() const;
  bool IsObj() const;
  bool IsList() const;
  bool IsString() const;
  bool IsDict() const;

  // 取值接口
  // 类型不匹配时，可选择断言或异常（实现中保持一致）。
  bool AsBool() const;
  double AsNum() const;
  const std::string& AsStr() const;
  std::shared_ptr<Object> AsObj() const;

  std::shared_ptr<ObjList> AsList() const;
  std::shared_ptr<ObjString> AsString() const;
  std::shared_ptr<ObjDict> AsDict() const;

  // 文本表示
  // 用于打印或调试：
  // Null -> "null"
  // Bool -> "true"/"false"
  // Num  -> 去掉多余小数
  // Str  -> 原样返回（不加引号）
  std::string ToRepr() const;


  // 比较运算
  // 仅在类型相同情况下比较；类型不同直接返回 false。
  bool operator==(const Value& rhs) const;
  bool operator!=(const Value& rhs) const;



  // 序列化接口
  // Load 是静态的，因为我们还不知道读出来的是什么，
  // 所以要通过工厂模式生产出一个新的 Value 对象返回。
  // static:不需要创建对象，直接用类名就可以使用
  void Save(BytecodeWriter& writer) const;
  static Value Load(BytecodeReader& reader);



 private:
  ValueType type_ = ValueType::kNull;  // 当前值类型。
  std::variant<std::monostate, bool, double, std::string, std::shared_ptr<Object>> data_;  // 存储实际数据。
};

}  // namespace cilly

#endif  // CILLY_VM_CPP_VALUE_H_
