// cilly-vm-cpp
// Author: Leixyaa
// Date: 11.6

// value.cc

#include "value.h"

#include <cassert>  // 类型断言
#include <iomanip>  // 控制数字格式
#include <iostream>
#include <memory>
#include <sstream>  // 数字转字符串
#include <string>
#include <utility>  //move()移动语义

#include "bytecode_stream.h"
#include "object.h"

namespace cilly {

Value::Value() : type_(ValueType::kNull), data_(std::monostate{}) {
}

Value::Value(ValueType x,
             std::variant<std::monostate, bool, double, std::string,
                          std::shared_ptr<Object>, int32_t>
                 y) :
    type_(x), data_(std::move(y)) {
}

Value Value::Null() {
  Value v;
  v.type_ = ValueType::kNull;
  v.data_ = std::monostate{};
  return v;
}

Value Value::Bool(bool b) {
  Value v;
  v.type_ = ValueType::kBool;
  v.data_ = b;
  return v;
}

Value Value::Num(double n) {
  Value v;
  v.type_ = ValueType::kNum;
  v.data_ = n;
  return v;
}

Value Value::Str(std::string s) {
  Value v;
  v.type_ = ValueType::kStr;
  v.data_ = std::move(s);
  return v;
}

Value Value::Obj(std::shared_ptr<ObjList> object) {
  Value v;
  v.type_ = ValueType::kObj;
  v.data_ = object;
  return v;
}

Value Value::Obj(std::shared_ptr<ObjString> object) {
  Value v;
  v.type_ = ValueType::kObj;
  v.data_ = object;
  return v;
}

Value Value::Obj(std::shared_ptr<ObjDict> object) {
  Value v;
  v.type_ = ValueType::kObj;
  v.data_ = object;
  return v;
}

Value Value::Callable(int32_t index) {
  Value v;
  v.type_ = ValueType::kCallable;
  v.data_ = index;
  return v;
}

ValueType Value::type() const {
  return type_;
}

bool Value::IsNull() const {
  return type_ == ValueType::kNull;
}

bool Value::IsBool() const {
  return type_ == ValueType::kBool;
}

bool Value::IsNum() const {
  return type_ == ValueType::kNum;
}

bool Value::IsStr() const {
  return type_ == ValueType::kStr;
}

bool Value::IsObj() const {
  return type_ == ValueType::kObj;
}

bool Value::IsList() const {
  return IsObj() && AsObj()->Type() == ObjType::kList;
}

bool Value::IsString() const {
  return IsObj() && AsObj()->Type() == ObjType::kString;
}

bool Value::IsDict() const {
  return IsObj() && AsObj()->Type() == ObjType::kDict;
}

bool Value::IsCallable() const {
  return type_ == ValueType::kCallable;
}

bool Value::AsBool() const {
  assert(IsBool() && "这不是Bool类型的数据");
  return std::get<bool>(data_);
}

double Value::AsNum() const {
  assert(IsNum() && "这不是Num类型的数据");
  return std::get<double>(data_);
}
//&返回引用 避免拷贝
const std::string& Value::AsStr() const {
  assert(IsStr() && "这不是Str类型的数据");
  return std::get<std::string>(data_);
}

std::shared_ptr<Object> Value::AsObj() const {
  assert(IsObj() && "这不是Object类型的数据");
  return std::get<std::shared_ptr<Object>>(data_);
}

std::shared_ptr<ObjList> Value::AsList() const {
  assert(IsList() && "这不是 List 类型的数据");
  return std::static_pointer_cast<ObjList>(AsObj());
}

std::shared_ptr<ObjString> Value::AsString() const {
  assert(IsString() && "这不是 String 对象");
  return std::static_pointer_cast<ObjString>(AsObj());
}

std::shared_ptr<ObjDict> Value::AsDict() const {
  assert(IsDict() && "这不是 Dict 对象");
  return std::static_pointer_cast<ObjDict>(AsObj());
}

int32_t Value::AsCallable() const {
  return std::get<int32_t>(data_);
}

std::string Value::ToRepr() const {
  if (IsNull()) {
    return "null";
  } else if (IsBool()) {
    return std::get<bool>(data_) ? "true" : "false";
  } else if (IsNum()) {
    double num = std::get<double>(data_);
    std::string s;
    std::ostringstream oss;
    oss << std::setprecision(15) << std::fixed << num;
    s = oss.str();
    while (!s.empty() && s.back() == '0') {
      s.pop_back();
    }
    if (!s.empty() && s.back() == '.') {
      s.pop_back();
    }
    return s;
  } else if (IsObj()) {
    auto obj = AsObj();
    return obj->ToRepr();
  } else if (IsCallable()) {
    return "<fn #" + std::to_string(std::get<int32_t>(data_)) + ">";
  }
  // Str
  return std::get<std::string>(data_);
}

bool Value::operator==(const Value& rhs) const {
  if (type_ != rhs.type_) {
    return false;
  }
  switch (type_) {
    case ValueType::kNull:
      return true;
    case ValueType::kBool:
      return AsBool() == rhs.AsBool();
    case ValueType::kNum:
      return AsNum() == rhs.AsNum();
    case ValueType::kStr:
      return AsStr() == rhs.AsStr();
    // case ValueType::kObj:  暂未实现
    default:
      return false;
  }
}

bool Value::operator!=(const Value& rhs) const {
  return !(*this == rhs);  // 这里的“==”是 operator== 重载的比较
}

// 序列化:将 Value 写入字节流
void Value::Save(BytecodeWriter& writer) const {
  // 先写入类型标记 (Tag)。
  writer.Write<uint8_t>(
      static_cast<uint8_t>(type_));  // uint8_t 只占一位字节，节省空间

  // 根据类型写入实际数据 (Payload)。
  switch (type_) {
    case ValueType::kNull:
      break;

    case ValueType::kBool:
      writer.Write<bool>(AsBool());  // 以bool的形式取出
      break;

    case ValueType::kNum:
      writer.Write<double>(AsNum());
      break;

    case ValueType::kStr:
      writer.WriteString(AsStr());
      break;

    default:  // 防御性机制
      break;
  }
}

Value Value::Load(BytecodeReader& reader) {
  // 读取类型标记
  uint8_t raw_tag = reader.Read<uint8_t>();
  ValueType type = static_cast<ValueType>(raw_tag);

  // 根据标记还原对象
  switch (type) {
    case ValueType::kNull:
      return Null();  // 读取相应类型并直接创建

    case ValueType::kBool:
      return Bool(reader.Read<bool>());

    case ValueType::kNum:
      return Num(reader.Read<double>());

    case ValueType::kStr:
      return Str(reader.ReadString());

    case ValueType::kObj:
      assert(false && "对象类型暂不支持序列化");  // 复杂对象暂未实现
      break;

    default:  // 如果读到了未知的
              // Tag，不要让程序崩溃，而是报错并返回一个安全的空值。
      std::cout << "未知或错误的 ValueTag: " << static_cast<int>(raw_tag)
                << std::endl;
      return Null();
      break;
  }
}

}  // namespace cilly