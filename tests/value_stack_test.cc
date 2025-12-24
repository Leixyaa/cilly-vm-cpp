#include <gtest/gtest.h>

#include "value.h"
#include "stack_stats.h"

namespace cilly {

TEST(Value, CallableRoundTrip) {
  Value f = Value::Callable(123);
  EXPECT_TRUE(f.IsCallable());
  EXPECT_EQ(f.AsCallable(), 123);
}

TEST(StackStats, PushPopCount) {
  StackStats st;
  st.Push(Value::Num(1));
  st.Push(Value::Num(2));

  EXPECT_EQ(st.Depth(), 2);

  Value b = st.Pop();
  EXPECT_TRUE(b.IsNum());
  EXPECT_EQ(b.AsNum(), 2);

  Value a = st.Pop();
  EXPECT_TRUE(a.IsNum());
  EXPECT_EQ(a.AsNum(), 1);

  EXPECT_EQ(st.Depth(), 0);
}

}  // namespace cilly
