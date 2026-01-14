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

////////////////////////////////////////////// Trace
////////////////////////////////////////////////////
// ObjList: list 里每个元素都是 Value；如果 Value 引用堆对象，则继续标记。
void ObjList::Trace(gc::Collector& c) {
  for (const Value& v : element) {
    if (v.IsObj())
      c.Mark(v.AsObj().get());
  }
}

// ObjDict: dict 的 key 是 std::string（不是 Value），不需要 Mark；只需要遍历
// value。
void ObjDict::Trace(gc::Collector& c) {
  for (const auto& [k, v] : entries_) {
    if (v.IsObj())
      c.Mark(v.AsObj().get());
  }
}

// ObjClass: methods 里存的是 callable index（int32），不是对象引用；
// 但 superclass 是 ObjClass 对象引用，需要标记。
void ObjClass::Trace(gc::Collector& c) {
  if (superclass)
    c.Mark(superclass.get());
}

// ObjInstance: instance -> klass（类对象）必须活；
// instance 的字段放在 fields（unique_ptr<ObjDict>）里，虽然 fields 自己不是 GC
// 管理对象（目前）， 但它里面的 Value 可能引用 GC 对象，所以要沿着字段表继续
// Trace。
void ObjInstance::Trace(gc::Collector& c) {
  if (klass) {
    c.Mark(klass.get());
  }
  if (fields) {
    fields->Trace(c);  // 继续标记字段 dict 中引用到的对象
  }
}

// ObjBoundMethod: receiver 是 Value（通常是 instance），若引用对象则需要标记。
// method 是 int32 index，不需要标记。
void ObjBoundMethod::Trace(gc::Collector& c) {
  if (receiver.IsObj())
    c.Mark(receiver.AsObj().get());
}

}  // namespace cilly