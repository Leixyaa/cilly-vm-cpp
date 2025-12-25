#include "src/value.h"

#include <gtest/gtest.h>

namespace cilly {

TEST(Value, DefaultIsNull) {
  Value v;
  EXPECT_TRUE(v.IsNull());
  EXPECT_EQ(v.ToRepr(), "null");
}

TEST(Value, BoolRepr) {
  Value t = Value::Bool(true);
  Value f = Value::Bool(false);

  EXPECT_TRUE(t.IsBool());
  EXPECT_TRUE(f.IsBool());
  EXPECT_EQ(t.AsBool(), true);
  EXPECT_EQ(f.AsBool(), false);
  EXPECT_EQ(t.ToRepr(), "true");
  EXPECT_EQ(f.ToRepr(), "false");
}

TEST(Value, NumBasic) {
  Value x = Value::Num(3.0);
  EXPECT_TRUE(x.IsNum());
  EXPECT_DOUBLE_EQ(x.AsNum(), 3.0);
  EXPECT_EQ(x.ToRepr(), "3");
}

TEST(Value, StrBasic) {
  Value s = Value::Str("hello");
  EXPECT_TRUE(s.IsStr());
  EXPECT_EQ(s.AsStr(), "hello");
  EXPECT_EQ(s.ToRepr(), "hello");
}

TEST(Value, EqualitySameType) {
  EXPECT_TRUE(Value::Num(1) == Value::Num(1));
  EXPECT_TRUE(Value::Bool(true) == Value::Bool(true));
  EXPECT_TRUE(Value::Str("a") == Value::Str("a"));
  EXPECT_TRUE(Value::Null() == Value::Null());

  EXPECT_TRUE(Value::Num(1) != Value::Num(2));
  EXPECT_TRUE(Value::Bool(true) != Value::Bool(false));
  EXPECT_TRUE(Value::Str("a") != Value::Str("b"));
}

TEST(Value, EqualityDifferentTypeIsFalse) {
  EXPECT_FALSE(Value::Num(1) == Value::Bool(true));
  EXPECT_FALSE(Value::Str("1") == Value::Num(1));
  EXPECT_FALSE(Value::Null() == Value::Bool(false));
}

}  // namespace cilly
