#ifndef CILLY_GC_GC_OBJECT_H_
#define CILLY_GC_GC_OBJECT_H_

#include <cstddef>
#include <cstdint>

namespace cilly::gc {
class Collector;
// 所有 GC 托管对象的基类：
// - intrusive list: next_
// - mark bit: marked_
// - Trace(): 子类实现，把引用的其它对象交给 Collector 标记
class GcObject {
 public:
  GcObject() = default;
  virtual ~GcObject() = default;

  // intrusive list：Collector 用它把对象挂到 all_objects_ 链表上
  GcObject* next() const { return next_; }
  void set_next(GcObject* n) { next_ = n; }

  bool marked() { return marked_; }
  void set_marked(bool flag) { marked_ = flag; }

  // ==================== 近似“堆字节数”统计 ====================
  // 说明：
  // - 用 sizeof(T) 作为该对象在 GC 堆上的“近似占用”
  // - 这不包含对象内部动态分配（vector/string capacity），但足以提供稳定触发器
  std::size_t size_bytes() const { return size_bytes_; }
  void set_size_bytes(std::size_t n) { size_bytes_ = n; }

  // 子类覆写：告诉GC我引用了哪些对象
  // 只负责“报告引用”，不做业务逻辑
  /*
    Trace：派生类必须实现。
    语义：当 GC 在标记阶段访问到“当前对象”时，会调用
    Trace，让对象把它引用的其它对象交给 GC。
  */
  virtual void Trace(Collector& c) = 0;

 private:
  GcObject* next_ = nullptr;
  bool marked_ = false;
  // 近似字节数：由 Collector::New<T>() 设置为 sizeof(T)
  std::size_t size_bytes_ = 0;
};

}  // namespace cilly::gc

#endif  // CILLY_GC_GC_OBJECT_H_