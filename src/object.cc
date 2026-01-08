#include "object.h"

#include <iterator>

namespace cilly {

std::string ObjList::ToRepr() const {
  std::string s;
  s += "[";
  int n = static_cast<int>(element.size());
  for (int i = 0; i < n; i++) {
    s += element[i].ToRepr();
    if (i != n - 1) {
      s += ", ";
    }
  }
  s += "]";
  return s;
}

Value ObjDict::Get(const std::string& key) const {
  auto it = entries_.find(key);
  if (it != entries_.end())
    return it->second;
  else
    return Value::Null();
}

std::string ObjDict::ToRepr() const {
  std::string s;
  s += "{ ";
  for (auto i = entries_.begin(); i != entries_.end(); i++) {
    s += "\"";
    s += i->first;
    s += "\" : ";
    s += i->second.ToRepr();
    // 判断是不是最后一个
    if (std::next(i) != entries_.end()) {
      s += ", ";
    }
  }
  s += " }";
  return s;
}

std::string ObjClass::ToRepr() const {
  std::string s;
  s += "<class ";
  s += name;
  s += ">";
  return s;
}

void ObjClass::DefineMethod(const std::string& name, int32_t callelabe_index) {
  methods[name] = callelabe_index;
}

int32_t ObjClass::GetMethodIndex(const std::string& method_name) {
  const ObjClass* k = this;
  while (k != nullptr) {
    auto it = k->methods.find(method_name);
    if (it != k->methods.end()) {
      return it->second;
    }
    k = k->superclass.get();
  }
  return -1;
}

void ObjClass::SetSuperclass(std::shared_ptr<ObjClass> superclass_) {
  superclass = std::move(superclass_);
}

const std::shared_ptr<ObjClass> ObjClass::Superclass() const {
  return superclass;
}

std::string ObjInstance::ToRepr() const {
  std::string s;
  s += "<";
  s += klass->Name();
  s += " Instance>";
  return s;
}

}  // namespace cilly