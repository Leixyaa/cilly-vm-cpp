#include <atomic>
#include <vector>

#include "gtest/gtest.h"
#include "src/gc/gc.h"
#include "src/object.h"
#include "src/value.h"

namespace cilly {
namespace {

// 一个用来测试并行 GC 的大型对象树节点
class TestNode final : public gc::GcObject {
 public:
  explicit TestNode(int id) : id_(id) {}

  // 挂载子节点
  void AddChild(GcObject* child) { children_.push_back(child); }

  void Trace(gc::Collector& c) override {
    for (auto* child : children_) {
      c.Mark(child);
    }
  }

  int id() const { return id_; }

 private:
  int id_;
  std::vector<GcObject*> children_;
};

// 基础测试：并行标记能否正确处理简单的可达性
TEST(GcParallelTest, ParallelMarkBasicReachability) {
  gc::Collector c;

  // 构建一个引用链: root -> n1 -> n2 -> n3
  auto* root = c.New<TestNode>(0);
  auto* n1 = c.New<TestNode>(1);
  auto* n2 = c.New<TestNode>(2);
  auto* n3 = c.New<TestNode>(3);
  auto* dead = c.New<TestNode>(99);  // 垃圾对象

  root->AddChild(n1);
  n1->AddChild(n2);
  n2->AddChild(n3);

  EXPECT_EQ(c.object_count(), 5u);

  // 使用 CollectParallel 进行回收
  c.CollectParallel([&](gc::Collector& gc) {
    gc.Mark(root);  // 只有 root 是根
  });

  // 应该剩下 4 个存活对象 (root, n1, n2, n3)，dead 被回收
  EXPECT_EQ(c.object_count(), 4u);
  EXPECT_EQ(c.last_swept_count(), 1u);
}

// 压力测试：构建一个大规模对象树，验证多线程并行扫描的完整性和正确性
TEST(GcParallelTest, ParallelMarkLargeTreeStressTest) {
  gc::Collector c;

  // 参数设置
  const int kDepth = 5;   // 树的深度
  const int kBranch = 8;  // 每个节点的分支数
  // 节点总数大致为 (Branch^(Depth+1) - 1) / (Branch - 1)
  // 8^6 / 7 ≈ 37000 个对象，足够触发多线程工作窃取

  // 递归构建树的 helper
  std::function<TestNode*(int)> build_tree = [&](int depth) -> TestNode* {
    auto* node = c.New<TestNode>(depth);
    if (depth < kDepth) {
      for (int i = 0; i < kBranch; ++i) {
        node->AddChild(build_tree(depth + 1));
      }
    }
    return node;
  };

  auto* root = build_tree(0);
  size_t total_nodes = c.object_count();

  // 再创建一些游离的垃圾对象，验证它们能被正确清除
  const int kGarbageCount = 1000;
  for (int i = 0; i < kGarbageCount; ++i) {
    (void)c.New<TestNode>(-1);
  }

  EXPECT_EQ(c.object_count(), total_nodes + kGarbageCount);

  // 执行并行 GC
  c.CollectParallel([&](gc::Collector& gc) { gc.Mark(root); });

  // 验证结果：
  // 1. 树上的所有节点都必须存活（并行标记没有漏标）
  EXPECT_EQ(c.object_count(), total_nodes);
  // 2. 所有的垃圾对象都必须被清除
  EXPECT_EQ(c.last_swept_count(), static_cast<size_t>(kGarbageCount));
}

// 验证并发清除 (Concurrent Sweep) 的安全性
// 虽然我们无法精确控制后台线程的调度，但我们可以验证 CollectParallel 返回后
// 主线程能否立即继续分配对象，且最终内存统计正确。
TEST(GcParallelTest, ConcurrentSweepSafety) {
  gc::Collector c;

  // 1. 制造大量垃圾
  const int kGarbageCount = 10000;
  for (int i = 0; i < kGarbageCount; ++i) {
    (void)c.New<TestNode>(i);
  }

  // 制造少量存活对象
  auto* root = c.New<TestNode>(0);

  // 2. 触发并行 GC
  // 注意：Sweep 是并发的，CollectParallel 返回时，后台线程可能还在 delete 垃圾
  c.CollectParallel([&](gc::Collector& gc) { gc.Mark(root); });

  // 3. 立即分配新对象
  // 如果 Concurrent Sweep 实现有锁竞争或指针失效问题，这里可能会 crash
  for (int i = 0; i < 1000; ++i) {
    (void)c.New<TestNode>(100 + i);
  }

  // 4. 稍微等待一下后台线程（虽然不是必须的，但为了统计数据的稳定性）
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // 5. 再次 GC，确保堆状态一致
  c.CollectParallel([&](gc::Collector& gc) { gc.Mark(root); });

  // 此时只应该剩下 root
  EXPECT_EQ(c.object_count(), 1u);
}

}  // namespace
}  // namespace cilly
