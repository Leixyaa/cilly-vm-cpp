#ifndef CILLY_VM_CPP_DEBUG_LOG_H_
#define CILLY_VM_CPP_DEBUG_LOG_H_

#include <iostream>

// 编译期开关：默认关闭。
// 打开方式：
//   bazelisk test //tests:all --cxxopt=-DCILLY_DEBUG_LOG=1 --test_output=all
#ifndef CILLY_DEBUG_LOG
#define CILLY_DEBUG_LOG 0
#endif

#if CILLY_DEBUG_LOG
// 用法：CILLY_DLOG("[debug] x=" << x << "\n");
#define CILLY_DLOG(stream_expr) \
  do {                          \
    std::cerr << stream_expr;   \
  } while (0)
#else
#define CILLY_DLOG(stream_expr) \
  do {                          \
  } while (0)
#endif

#endif  // CILLY_VM_CPP_DEBUG_LOG_H_
