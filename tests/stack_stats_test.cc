#include "src/stack_stats.h"

#include <gtest/gtest.h>

#include "src/value.h"

namespace cilly {

TEST(StackStats, PushPopAndStats) {
  StackStats st;

  EXPECT_EQ(st.Depth(), 0);
  EXPECT_EQ(st.MaxDepth(), 0);
  EXPECT_EQ(st.PushCount(), 0);
  EXPECT_EQ(st.PopCount(), 0);

  st.Push(Value::Num(1));
  st.Push(Value::Num(2));
  st.Push(Value::Num(3));

  EXPECT_EQ(st.Depth(), 3);
  EXPECT_EQ(st.MaxDepth(), 3);
  EXPECT_EQ(st.PushCount(), 3);
  EXPECT_EQ(st.PopCount(), 0);
  EXPECT_TRUE(st.Top().IsNum());
  EXPECT_DOUBLE_EQ(st.Top().AsNum(), 3);

  Value v3 = st.Pop();
  EXPECT_TRUE(v3.IsNum());
  EXPECT_DOUBLE_EQ(v3.AsNum(), 3);

  EXPECT_EQ(st.Depth(), 2);
  EXPECT_EQ(st.MaxDepth(), 3);
  EXPECT_EQ(st.PushCount(), 3);
  EXPECT_EQ(st.PopCount(), 1);
  EXPECT_DOUBLE_EQ(st.Top().AsNum(), 2);

  st.Pop();
  st.Pop();
  EXPECT_EQ(st.Depth(), 0);
  EXPECT_EQ(st.PopCount(), 3);
}

TEST(StackStats, ClearResetsBothStackAndStats) {
  StackStats st;
  st.Push(Value::Num(1));
  st.Push(Value::Num(2));
  EXPECT_EQ(st.Depth(), 2);
  EXPECT_EQ(st.MaxDepth(), 2);
  EXPECT_EQ(st.PushCount(), 2);

  st.Clear();
  EXPECT_EQ(st.Depth(), 0);
  EXPECT_EQ(st.MaxDepth(), 0);
  EXPECT_EQ(st.PushCount(), 0);
  EXPECT_EQ(st.PopCount(), 0);

  // Clear 后还能继续正常使用
  st.Push(Value::Num(7));
  EXPECT_EQ(st.Depth(), 1);
  EXPECT_EQ(st.MaxDepth(), 1);
  EXPECT_EQ(st.PushCount(), 1);
}

TEST(StackStats, ResetStatsDoesNotClearStack) {
  StackStats st;
  st.Push(Value::Num(1));
  st.Push(Value::Num(2));
  EXPECT_EQ(st.Depth(), 2);
  EXPECT_EQ(st.MaxDepth(), 2);
  EXPECT_EQ(st.PushCount(), 2);

  st.ResetStats();
  // 栈还在
  EXPECT_EQ(st.Depth(), 2);
  EXPECT_TRUE(st.Top().IsNum());
  EXPECT_DOUBLE_EQ(st.Top().AsNum(), 2);

  // 统计归零（max_depth 也归零是你的实现约定）
  EXPECT_EQ(st.PushCount(), 0);
  EXPECT_EQ(st.PopCount(), 0);
  EXPECT_EQ(st.MaxDepth(), 0);

  // 再 Push 一次，max_depth 重新从当前深度开始统计
  st.Push(Value::Num(3));
  EXPECT_EQ(st.Depth(), 3);
  EXPECT_EQ(st.MaxDepth(), 3);
  EXPECT_EQ(st.PushCount(), 1);
}

}  // namespace cilly
