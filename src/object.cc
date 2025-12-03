#include <iterator>
#include "object.h"

namespace cilly {



std::string ObjList::ToRepr() const {
  std::string s;
  s += "[";
  int n = static_cast<int>(element.size());
  for (int i = 0; i < n; i++) {
    s += element[i].ToRepr();
    if (i != n - 1) { s += ", "; }
  }
  s += "]";
  return s;
}



Value ObjDict::Get(const std::string& key) const {
  auto it = entries_.find(key);  
  if(it != entries_.end()) return it->second;
  else return Value::Null();
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
    if(std::next(i) != entries_.end()) { s += ", "; }
  }
  s += " }";
  return s;
}

} //namespace cilly