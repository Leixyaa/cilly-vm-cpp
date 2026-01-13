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
};

}  // namespace cilly::gc

#endif  // CILLY_GC_GC_OBJECT_H_