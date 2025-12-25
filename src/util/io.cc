#include "io.h"

#include <fstream>
#include <stdexcept>
#include <string>

namespace cilly {

std::string ReadFileToString(const std::string path) {
  std::ifstream file(path);
  if (!file.is_open()) {
    throw std::runtime_error("无法打开文件: " + path);
  }
  std::string str;
  char i;
  while (file.get(i)) {
    str += i;
  }
  return str;
}

}  // namespace cilly
