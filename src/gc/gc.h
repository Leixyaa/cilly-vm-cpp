#ifndef CILLY_GC_GC_H_
#define CILLY_GC_GC_H_

#include <cassert>
#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

#include "gc/gc_object.h"

namespace cilly::gc {

class RootGuard;

// GC收集器：维护堆对象链、root栈、以及mark-sweep实现
class Collector {
 public:
  Collector();
  ~Collector();

  Collector(const Collector&) = delete;
  Collector& operator=(const Collector&) = delete;

  //////////////////////// 分配 ////////////////////////////
  template <class T, class... Args>
  T* New(Args&&... args) {
    // 编译期断言       // GcObject是T的基类
    static_assert(std::is_base_of_v<GcObject, T>,
                  "T must derive from GcObject");
    T* obj = new T(std::forward<Args>(
        args)...);  // 避免引用折叠 避免期望触发移动反而触发拷贝、

    // 记录该对象的“近似占用字节数”
    obj->set_size_bytes(sizeof(T));

    obj->set_next(all_objects_);
    all_objects_ = obj;
    ++object_count_;

    // 总字节数统计
    heap_bytes_ += obj->size_bytes();

    return obj;
  }

  template <class F>
  void CollectWithRoots(F&& trace_roots) {
    // 统计重置
    last_swept_count_ = 0;
    last_marked_count_ = 0;
    gray_stack_.clear();

    // 1.标记临时root
    for (auto i : temp_roots_) {
      Mark(i);
    }

    // 2.再标记外部roots
    trace_roots(*this);

    // 扫描对象图
    DrainGrayStack();

    // 清理
    Sweep();
  }

  /*
   临时 roots（给 RootGuard 用）：
   - 解决坑：对象如果只存在于 C++ 局部变量里，GC 扫不到就会误回收 -> UAF
   - 使用方式：RootGuard 构造 Push，析构 Pop
   - 严格 LIFO：保证作用域嵌套的正确性，并尽早暴露“多 pop/少 pop/pop 错对象”的
   bug
 */
  void PushRoot(GcObject* obj);
  void PopRoot(GcObject* obj);

  // 统计：为了让 GC 可测（gtest 能断言 sweep/mark 的数量）
  std::size_t object_count() const;
  std::size_t heap_bytes() const;
  std::size_t last_swept_count() const;
  std::size_t last_marked_count() const;

  /*
    Collect：一次完整 GC
  */
  void Collect();

  /*
    FreeAll：用于 VM 退出/析构时统一释放。
    注意：它不做可达性分析，直接把 all_objects_ 全删了（类似“关机清空堆”）。
  */
  void FreeAll();

  /*
    Mark：给 Trace() 用的 API。
    语义：如果 obj 未标记，则标记并入 gray_stack_，等待 DrainGrayStack()
    处理它的 Trace。
  */
  void Mark(GcObject* obj);

 private:
  // 标记阶段：把 gray_stack_ 里的对象逐个取出，调用 Trace 扩展可达集合
  void DrainGrayStack();

  // 清扫阶段：遍历 all_objects_，删掉未标记对象；活对象清掉 mark bit 留到下一轮
  void Sweep();

 private:
  // 堆对象链表头（intrusive list）
  GcObject* all_objects_ = nullptr;

  // 当前堆上对象总数（用于测试与调试）
  std::size_t object_count_ = 0;

  // 临时 root 栈：RootGuard 会把对象压在这里，保证 GC 看得见
  std::vector<GcObject*> temp_roots_;

  // 标记工作栈：避免递归
  std::vector<GcObject*> gray_stack_;

  // 每次 Collect 的统计数据（用于 gtest 断言）
  std::size_t last_swept_count_ = 0;
  std::size_t last_marked_count_ = 0;

  // 当前堆上对象近似字节数（sum(sizeof(T)) - swept）
  std::size_t heap_bytes_ = 0;

  friend class RootGuard;
};

class RootGuard {
 public:
  RootGuard(Collector& c, GcObject* obj);
  ~RootGuard();

  RootGuard(const RootGuard&) = delete;
  RootGuard& operator=(const RootGuard&) = delete;

 private:
  Collector& c_;
  GcObject* obj_ = nullptr;
};

// 生成一个“由 GC 拥有内存”的 shared_ptr 句柄。
// 注意：这个 shared_ptr 只用于在现阶段尽量少改 Value 的表示方式；
// deleter 是 no-op，真正释放在将来由 Collector::Sweep/FreeAll 负责。
template <class T, class... Args>
std::shared_ptr<T> MakeShared(Collector& c, Args&&... args) {
  T* obj = c.New<T>(std::forward<Args>(args)...);
  return std::shared_ptr<T>(obj, [](T*) {});
}

}  // namespace cilly::gc

#endif  // CILLY_GC_GC_H_