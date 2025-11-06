// cilly-vm-cpp
// Author: Leixyaa
// Date: 11.6

#ifndef CILLY_VM_CPP_VALUE_H_
#define CILLY_VM_CPP_VALUE_H_

#include <string>
#include <variant>

namespace cilly {

// 运行时值的类型枚举。
// 用于标识当前 Value 中存放的数据类型。
enum class ValueType { kNull, kBool, kNum, kStr };

// 运行时的通用值类型。
// 可存放 null、bool、number、string 四种数据。
class Value {
 public:
  // 构造与工厂方法 --------------------------------------------
  Value();  // 默认构造成 Null。
  Value(ValueType x, std::variant<std::monostate, bool, double, std::string> y);
  static Value Null();
  static Value Bool(bool b);
  static Value Num(double d);
  static Value Str(std::string s);


  // 类型判断 --------------------------------------------------
  ValueType type() const;
  bool IsNull() const;
  bool IsBool() const;
  bool IsNum() const;
  bool IsStr() const;


  // 取值接口 --------------------------------------------------
  // 类型不匹配时，可选择断言或异常（实现中保持一致）。
  bool AsBool() const;
  double AsNum() const;
  const std::string& AsStr() const;


  // 文本表示 --------------------------------------------------
  // 用于打印或调试：
  // Null -> "null"
  // Bool -> "true"/"false"
  // Num  -> 去掉多余小数
  // Str  -> 原样返回（不加引号）
  std::string ToRepr() const;


  // 比较运算 --------------------------------------------------
  // 仅在类型相同情况下比较；类型不同直接返回 false。
  bool operator==(const Value& rhs) const;
  bool operator!=(const Value& rhs) const;

 private:
  ValueType type_ = ValueType::kNull;  // 当前值类型。
  std::variant<std::monostate, bool, double, std::string> data_;  // 存储实际数据。
};

}  // namespace cilly

#endif  // CILLY_VM_CPP_VALUE_H_
