#include <memory>

#include "gtest/gtest.h"
#include "src/gc/gc.h"
#include "src/object.h"
#include "src/value.h"
#include "tests/run_script.h"

namespace cilly {
namespace {

class TestNode final : public gc::GcObject {
 public:
  explicit TestNode(GcObject* child = nullptr) : child_(child) {}

  void Trace(gc::Collector& c) override {
    if (child_)
      c.Mark(child_);
  }

 private:
  GcObject* child_ = nullptr;
};

TEST(GcSmokeTest, RootGuardKeepsTempRootAlive) {
  gc::Collector c;

  auto* b = c.New<TestNode>();
  auto* a = c.New<TestNode>(b);

  EXPECT_EQ(c.object_count(), 2u);

  {
    gc::RootGuard g(c, a);
    c.Collect();
  }

  // a 是 root，a->Trace 会标记 b，所以都活
  EXPECT_EQ(c.object_count(), 2u);
  EXPECT_EQ(c.last_swept_count(), 0u);
}

TEST(GcSmokeTest, CollectWithRootsCanMarkExternalRoots) {
  gc::Collector c;

  auto* b = c.New<TestNode>();
  auto* a = c.New<TestNode>(b);
  (void)c.New<TestNode>();  // garbage：不被引用，应该被 sweep 掉

  EXPECT_EQ(c.object_count(), 3u);

  c.CollectWithRoots([&](gc::Collector& gc) {
    gc.Mark(a);  // 外部 root：从这里开始可达的对象都要活
  });

  // 垃圾对象被清掉，a/b 存活
  EXPECT_EQ(c.object_count(), 2u);
  EXPECT_EQ(c.last_swept_count(), 1u);
  EXPECT_EQ(c.last_marked_count(), 2u);
}

TEST(GcSmokeTest, RunResultKeepsCollectorAlive) {
  auto heap = std::make_shared<gc::Collector>();
  (void)heap->New<TestNode>();

  test::RunResult r;
  r.gc_keepalive = heap;

  heap.reset();  // 如果 RunResult 没保活，这里 heap 就会析构并 FreeAll
  ASSERT_TRUE(r.gc_keepalive);
  EXPECT_EQ(r.gc_keepalive->object_count(), 1u);
}

TEST(GcSmokeTest, RunScriptAllocatesRuntimeObjectsIntoInjectedCollector) {
  auto r = cilly::test::RunScript(R"(
    class A { fun init() {} }
    var a = A();
    return 0;
  )");

  ASSERT_TRUE(r.gc_keepalive);
  EXPECT_GT(r.gc_keepalive->object_count(), 0u);
  EXPECT_EQ(r.ret.AsNum(), 0);
}

TEST(GcSmokeTest, Trace_ListDictChainKeepsReachablesAlive) {
  gc::Collector c;

  // 分配 4 个 GC 对象：list -> dict -> string，以及一个 garbage string
  auto list = gc::MakeShared<ObjList>(c);
  auto dict = gc::MakeShared<ObjDict>(c);
  auto alive_str = gc::MakeShared<ObjString>(c, "alive");
  auto dead_str = gc::MakeShared<ObjString>(c, "dead");  // 不挂到任何可达结构上

  dict->Set("k", Value::Obj(alive_str));
  list->Push(Value::Obj(dict));

  EXPECT_EQ(c.object_count(), 4u);

  c.CollectWithRoots([&](gc::Collector& gc) {
    gc.Mark(list.get());  // root: list
  });

  // dead_str 应该被 sweep，其他三个对象可达而存活
  EXPECT_EQ(c.object_count(), 3u);
  EXPECT_EQ(c.last_swept_count(), 1u);
}

/////补齐对象图Trace（list/dict/instance/class/boundmethod）/////
TEST(GcSmokeTest, Trace_InstanceMarksKlassAndFieldValues) {
  gc::Collector c;

  auto klass = gc::MakeShared<ObjClass>(c, "A");
  auto inst = gc::MakeShared<ObjInstance>(c, klass);

  auto field_str = gc::MakeShared<ObjString>(c, "field");
  auto dead_str = gc::MakeShared<ObjString>(c, "dead");

  // instance.fields 是 ObjDict（unique_ptr），但 dict 的 entries_ 里保存
  // Value::Obj(field_str)
  inst->Fields()->Set("x", Value::Obj(field_str));

  EXPECT_EQ(c.object_count(), 4u);

  c.CollectWithRoots([&](gc::Collector& gc) {
    gc.Mark(inst.get());  // root: instance
  });

  // inst 可达 -> klass 可达；inst.fields 里的 field_str 可达；dead_str 不可达
  EXPECT_EQ(c.object_count(), 3u);
  EXPECT_EQ(c.last_swept_count(), 1u);
}

}  // namespace
}  // namespace cilly
