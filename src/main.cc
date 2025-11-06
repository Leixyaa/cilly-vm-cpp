// cilly-vm-cpp
// Author: Leixyaa
// Date: 11.6
// Description: Entry point for testing environment.

#include <iostream>
#include "value.h"

int main() {
  cilly::Value vnull = cilly::Value::Null();
  cilly::Value vtrue = cilly::Value::Bool(true);
  cilly::Value vnum  = cilly::Value::Num(10);
  cilly::Value vstr  = cilly::Value::Str("hi");

  std::cout << vnull.ToRepr() << std::endl;
  std::cout << vtrue.ToRepr() << std::endl;
  std::cout << vnum.ToRepr() << std::endl;
  std::cout << vstr.ToRepr() << std::endl;
  std::cout << (vnum == cilly::Value::Num(10)) << std::endl;
  std::cout << (vnum == vstr) << std::endl;
}