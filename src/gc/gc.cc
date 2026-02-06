#include "gc.h"

#include <thread>
#include <vector>

namespace cilly::gc {

Collector::Collector() = default;

Collector::~Collector() {
  // Collector 拥有 all_objects_ 的生命周期：析构时兜底释放，避免泄漏
  FreeAll();
}

void Collector::PushRoot(GcObject* obj) {
  if (!obj)
    return;
  temp_roots_.push_back(obj);
}

void Collector::PopRoot(GcObject* obj) {
  // RootGuard 设计为严格 LIFO：作用域嵌套天然形成栈结构
  // 这样能极早发现错误用法（多 pop/少 pop/pop 错对象），避免隐蔽 UAF
  assert(!temp_roots_.empty());
  assert(temp_roots_.back() == obj);
  temp_roots_.pop_back();
}

std::size_t Collector::object_count() const {
  return object_count_;
}
std::size_t Collector::heap_bytes() const {
  return heap_bytes_;
}
std::size_t Collector::last_swept_count() const {
  return last_swept_count_;
}
std::size_t Collector::total_swept_count() const {
  return total_swept_count_;
}
std::size_t Collector::last_marked_count() const {
  return last_marked_count_;
}

void Collector::AddHeapBytesDelta(std::ptrdiff_t delta) {
  if (delta >= 0) {
    heap_bytes_ += static_cast<std::size_t>(delta);
    return;
  }

  const std::size_t dec = static_cast<std::size_t>(-delta);
  if (heap_bytes_ >= dec) {
    heap_bytes_ -= dec;
    return;
  }

  const std::size_t recomputed = RecomputeHeapBytes_();
  heap_bytes_ = recomputed;
  if (heap_bytes_ >= dec) {
    heap_bytes_ -= dec;
  } else {
    heap_bytes_ = 0;
  }
}

void Collector::Collect() {
  CollectWithRoots([](Collector&) {});
}

void Collector::FreeAll() {
  // “关机清空堆”：不做可达性分析，直接释放所有对象
  GcObject* obj = all_objects_;
  while (obj) {
    GcObject* next = obj->next();
    delete obj;  // 虚析构：会调用派生类析构，释放内部资源
    obj = next;
  }
  all_objects_ = nullptr;
  object_count_ = 0;
  heap_bytes_ = 0;
  total_swept_count_ = 0;
}

void Collector::Mark(GcObject* obj) {
  if (!obj)
    return;

  if (parallel_mode_) {
    MarkParallel(obj);
    return;
  }

  // 已标记就不用重复入栈，否则会导致重复 Trace，浪费时间
  if (obj->marked())
    return;

  obj->set_marked(true);
  ++last_marked_count_;

  // gray_stack_：迭代式遍历对象图，避免递归爆栈
  gray_stack_.push_back(obj);
}

void Collector::MarkParallel(GcObject* obj) {
  if (!obj)
    return;

  if (obj->TryMark()) {  // 尝试抢占对象
    std::lock_guard<std::mutex> lock(work_mutex_);
    global_work_stack_.push_back(obj);
    active_tasks_.fetch_add(1);
  }
}

void Collector::DrainGrayStack() {
  // 典型三色标记的简化版：
  // - gray_stack_ 里是“已标记但尚未扫描引用”的对象（灰）
  // - 弹出后调用 Trace，把引用对象 Mark 掉（推进可达集合）
  while (!gray_stack_.empty()) {
    GcObject* obj = gray_stack_.back();
    gray_stack_.pop_back();
    obj->Trace(*this);
  }
}

void Collector::Sweep() {
  std::vector<GcObject*> garbage;

  GcObject* prev = nullptr;
  GcObject* obj = all_objects_;

  while (obj) {
    if (obj->marked()) {
      obj->set_marked(false);
      prev = obj;
      obj = obj->next();
      continue;
    }

    GcObject* dead = obj;
    obj = obj->next();

    if (prev)
      prev->set_next(obj);
    else
      all_objects_ = obj;

    // 更新统计数据
    AddHeapBytesDelta(-static_cast<std::ptrdiff_t>(dead->SizeBytes()));
    --object_count_;
    ++last_swept_count_;
    ++total_swept_count_;

    garbage.push_back(dead);
  }
  if (!garbage.empty()) {
    std::thread([g = std::move(garbage)]() {
      for (auto* dead : g) {
        delete dead;
      }
    }).detach();
  }
}

void Collector::ProcessWorkStack() {
  while (true) {
    GcObject* obj;

    {
      std::unique_lock<std::mutex> lock(work_mutex_);
      if (global_work_stack_.empty()) {
        if (active_tasks_.load() == 0)
          return;
        lock.unlock();
        std::this_thread::yield();
        continue;
      }

      obj = global_work_stack_.back();
      global_work_stack_.pop_back();
    }

    obj->Trace(*this);
    active_tasks_.fetch_sub(1);
  }
}

std::size_t Collector::RecomputeHeapBytes_() const {
  std::size_t sum = 0;
  for (GcObject* obj = all_objects_; obj; obj = obj->next()) {
    // SizeBytes() 由各对象（ObjList/ObjDict/ObjString...）按 capacity/bucket
    // 等返回
    sum += obj->SizeBytes();
  }
  return sum;
}

RootGuard::RootGuard(Collector& c, GcObject* obj) : c_(c), obj_(obj) {
  // 构造即保护：把 obj 压入临时 roots
  if (obj_)
    c_.PushRoot(obj_);
}

RootGuard::~RootGuard() {
  // 析构即解除保护：从临时 roots 弹出
  if (obj_)
    c_.PopRoot(obj_);
}

}  // namespace cilly::gc
