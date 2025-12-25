#ifndef CILLY_BUILTINS_H_
#define CILLY_BUILTINS_H_

namespace cilly {

class VM;

// 注册内建函数（固定顺序：len/str/type/abs/clock -> index 0..4）
void RegisterBuiltins(VM& vm);

}  // namespace cilly

#endif  // CILLY_BUILTINS_H_
