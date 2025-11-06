// cilly-vm-cpp
// Author: Leixyaa
// Date: 11.6

#include "value.h"
#include <cassert>   // 类型断言
#include <sstream>   // 数字转字符串
#include <iomanip>   // 控制数字格式
#include <string>
#include <utility>   //move()移动语义


namespace cilly {

Value::Value() : type_(ValueType::kNull), data_(std::monostate{}) {}

Value::Value(ValueType x, std::variant<std::monostate, bool, double, std::string> y) : type_(x), data_(std::move(y)) {}

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

bool Value::AsBool() const{
  assert(IsBool() && "这不是Bool类型的数据");
  return std::get<bool>(data_);
}

double Value::AsNum() const {
  assert(IsNum() && "这不是Num类型的数据");
  return std::get<double>(data_);
}
//&返回引用 避免拷贝
const std::string&  Value::AsStr() const {
  assert(IsStr() && "这不是Str类型的数据");
  return std::get<std::string>(data_);
}

std::string Value::ToRepr() const{
  if(IsNull()) {
    return "null";
  } else if(IsBool()) {
    return std::get<bool>(data_)? "true":"false";
  } else if(IsNum()) {
    double num = std::get<double>(data_);
    std::string s;
    std::ostringstream oss;
    oss << std::setprecision(15) << std::fixed << num;
    s = oss.str();
    while(!s.empty() && s.back() == '0') {
      s.pop_back();
    }
    if(!s.empty() && s.back() == '.') {
        s.pop_back();
    }
    return s;
  } 
  //Str
  return std::get<std::string>(data_);
}

bool Value::operator==(const Value& rhs) const{
  if(type_ != rhs.type_) {
    return false;
  }
  switch(type_) {
    case ValueType::kNull: return true;
    case ValueType::kBool: return AsBool() == rhs.AsBool();
    case ValueType::kNum: return AsNum() == rhs.AsNum();
    case ValueType::kStr: return AsStr() == rhs.AsStr();
    default: return false;
  }
}

bool Value::operator!=(const Value& rhs) const{
  return !(*this == rhs); //这里的“==”是 operator== 重载的比较
}


}