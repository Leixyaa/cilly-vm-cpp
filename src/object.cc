#include "object.h"

namespace cilly {

ObjType ObjString::Type() const{
  return type_;
}

int ObjString::RefCount() const{
	return ref_count_;
}

std::string ObjString::ToRepr() const{
  return value_;
}


} //namespace cilly