#ifndef CILLY_GC_GC_OBJECT_H_
#define CILLY_GC_GC_OBJECT_H_

#include <atomic>
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

  bool TryMark() {
    bool expected = false;
    return marked_.compare_exchange_strong(expected, true);
  }

  bool marked() { return marked_.load(std::memory_order_relaxed); }
  void set_marked(bool flag) { marked_.store(flag, std::memory_order_relaxed); }

  // ==================== 堆占用估算 ====================
  // 默认返回基础大小（sizeof(派生类) 由 New<T> 写入 size_bytes_）。
  // 派生类可以 override，把内部动态容量（vector/string 等）也算进去。
  virtual std::size_t SizeBytes() const { return size_bytes_; }
  // 旧计算 防止回归出错故保留
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
  std::atomic<bool> marked_{false};
  // 近似字节数：由 Collector::New<T>() 设置为 sizeof(T)
  std::size_t size_bytes_ = 0;
};

}  // namespace cilly::gc

#endif  // CILLY_GC_GC_OBJECT_H_