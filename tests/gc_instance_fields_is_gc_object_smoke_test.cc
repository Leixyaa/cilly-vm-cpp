#include "gtest/gtest.h"
#include "src/gc/gc.h"
#include "src/object.h"
#include "src/value.h"

namespace cilly {
namespace {

TEST(GcInstanceFieldsSmokeTest, InstanceKeepsFieldsAndNestedObjectsAlive) {
  gc::Collector c;

  // 1) 分配 klass / fields / list / instance，全部走 GC 的 New（MakeShared
  // no-op deleter）
  auto klass = gc::MakeShared<ObjClass>(c, "A");
  auto fields = gc::MakeShared<ObjDict>(c);
  auto list = gc::MakeShared<ObjList>(c);

  list->Push(Value::Num(1));
  list->Push(Value::Num(2));

  // fields["l"] = list
  fields->Set("l", Value::Obj(list));

  // instance(klass, fields)
  auto inst = gc::MakeShared<ObjInstance>(c, klass, fields);

  // 2) 只把 instance 当 root，跑一次 GC：不应该 sweep 掉任何东西（都可达）
  {
    gc::RootGuard g(c, inst.get());
    c.CollectWithRoots([](gc::Collector&) {});
    EXPECT_EQ(c.last_swept_count(), static_cast<std::size_t>(0));
  }

  // 3) 现在没有任何 root，再跑一次 GC：至少应 sweep 4 个对象
  //    - ObjInstance
  //    - ObjClass
  //    - ObjDict(fields)
  //    - ObjList
  c.CollectWithRoots([](gc::Collector&) {});
  EXPECT_GE(c.last_swept_count(), static_cast<std::size_t>(4));
  EXPECT_EQ(c.object_count(), static_cast<std::size_t>(0));
}

}  // namespace
}  // namespace cilly
