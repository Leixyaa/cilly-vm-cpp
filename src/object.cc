#include "object.h"

namespace cilly {



std::string ObjList::ToRepr() const {
  std::string s;
  s += "[";
  int n = static_cast<int>(element.size());
  for(int i = 0; i < n; i++) {
    s += element[i].ToRepr();
    if (i != n - 1) { s += ", "; }
  }
  s += "]";
  return s;
}


} //namespace cilly