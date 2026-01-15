// object.h
#ifndef CILLY_VM_CPP_OBJECT_H_
#define CILLY_VM_CPP_OBJECT_H_
// 内存所有权
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "gc/gc.h"
#include "gc/gc_object.h"
#include "value.h"

namespace cilly {

enum class ObjType {
  kString,
  kList,
  kDict,
  kClass,
  kInstance,
  kBoundMethod,
};

class Object : public gc::GcObject {
 public:
  Object(ObjType type, int ref_count) : type_(type), ref_count_(ref_count) {}

  ObjType Type() const { return type_; }
  virtual std::string ToRepr() const { return ""; }

  // 现在先空实现：本步不做真正可达性遍历。
  // 下一步会为 ObjList/ObjDict/ObjInstance/ObjClass/ObjBoundMethod 等补齐
  // Trace。
  void Trace(gc::Collector&) override {}

  virtual ~Object() = default;
  // mark bit,next	 // 以后可添加mark bit(GC 用)，指针next等

 protected:
  ObjType type_;  // 表示类型
  // 当前阶段：真正的内存管理交给 std::shared_ptr<Object>，
  // 这里的 ref_count_ 暂时不使用，留给以后实现自定义 GC / 引用计数。
  int ref_count_;
};

class ObjString : public Object {
 public:
  ObjString() : Object(ObjType::kString, 1), value_("") {};

  explicit ObjString(std::string string_value) :
      Object(ObjType::kString, 1), value_(std::move(string_value)) {}

  void Set(const std::string& s) { value_ = s; }

  std::string ToRepr() const override { return value_; }

  std::size_t SizeBytes() const override;

  ~ObjString() override = default;

 private:
  std::string value_;
};

class ObjList : public Object {
 public:
  ObjList() : Object(ObjType::kList, 1), element() {};

  explicit ObjList(std::vector<Value> elem) :
      Object(ObjType::kList, 1), element(std::move(elem)) {};

  int Size() const { return static_cast<int>(element.size()); }
  void Push(const Value& v) { element.push_back(v); }
  const Value& At(int index) const { return element[index]; }
  void Set(int index, const Value& v) { element[index] = v; }

  std::string ToRepr() const override;

  void Trace(gc::Collector& c) override;

  std::size_t SizeBytes() const override;

  ~ObjList() override = default;

 private:
  std::vector<Value> element;
};  // List

class ObjDict : public Object {
 public:
  ObjDict() : Object(ObjType::kDict, 1), entries_() {};

  // 测试/优化用：预留 n 个元素容量，保证构造结束后 SizeBytes 变大
  explicit ObjDict(std::size_t reserve_entries) : Object(ObjType::kDict, 1) {
    entries_.reserve(reserve_entries);
  }

  explicit ObjDict(std::unordered_map<std::string, Value> index) :
      Object(ObjType::kDict, 1), entries_(std::move(index)) {}

  // 写入/修改键值
  // 如果 key 已存在，就覆盖原来的值。
  void Set(const std::string& key, const Value& value) {
    entries_[key] = value;
  }

  // 查询是否存在
  bool Has(const std::string& key) const {
    return entries_.find(key) != entries_.end();
  }

  // 读取：返回指定 key 对应的 Value
  // 如果不存在，就返回 Value::Null()
  Value Get(const std::string& key) const;

  void Erase(const std::string& key) { entries_.erase(key); }

  int Size() const { return static_cast<int>(entries_.size()); }

  std::string ToRepr() const override;

  void Trace(gc::Collector& c) override;

  std::size_t SizeBytes() const override;

  ~ObjDict() override = default;

 private:
  std::unordered_map<std::string, Value> entries_;

};  // Dic

class ObjClass : public Object {
 public:
  explicit ObjClass(std::string name_) :
      Object(ObjType::kClass, 1), name(std::move(name_)) {}

  const std::string& Name() const { return name; }
  std::string ToRepr() const override;

  void DefineMethod(const std::string& name, int32_t callelabe_index);
  int32_t GetMethodIndex(const std::string& method_name);

  // 继承相关
  void SetSuperclass(std::shared_ptr<ObjClass> superclass_);
  const std::shared_ptr<ObjClass> Superclass() const;

  void Trace(gc::Collector& c) override;

 private:
  std::string name;
  std::shared_ptr<ObjClass> superclass;
  std::unordered_map<std::string, int32_t> methods;
};  // Class

class ObjInstance : public Object {
 public:
  // 约定：fields 也是 GC 管理对象（ObjDict），由 VM 在“知道 gc_
  // 是否存在”时创建好再传进来。 这样 ObjInstance 本身不用关心“怎么分配
  // fields”，避免 ObjInstance 内部偷偷 new 出一个 GC 看不见的 dict会让 bytes
  // budget / 统计体系失真
  ObjInstance(std::shared_ptr<ObjClass> klass_,
              std::shared_ptr<ObjDict> fields_) :
      Object(ObjType::kInstance, 1),
      klass(std::move(klass_)),
      fields(std::move(fields_)) {
    // 防御：理论上 VM 不该传空；但为了鲁棒性，空就退化成普通 shared_ptr（无 GC
    // 时也能工作）
    if (!fields) {
      fields = std::make_shared<ObjDict>();
    }
  }

  // 兼容过去版本
  explicit ObjInstance(std::shared_ptr<ObjClass> klass_) :
      ObjInstance(std::move(klass_), std::make_shared<ObjDict>()) {}

  std::shared_ptr<ObjClass> Klass() const { return klass; }
  ObjDict* Fields() const { return fields.get(); }
  std::string ToRepr() const override;

  void Trace(gc::Collector& c) override;

 private:
  std::shared_ptr<ObjClass> klass;
  std::shared_ptr<ObjDict> fields;
};

class ObjBoundMethod : public Object {
 public:
  explicit ObjBoundMethod(Value receiver_, int32_t index) :
      Object(ObjType::kBoundMethod, 1), receiver(receiver_), method(index) {}

  const Value& Receiver() { return receiver; }
  int32_t Method() { return method; }

  std::string ToRepr() const override {
    return "<bound_method #" + std::to_string(method) + ">";
  }

  void Trace(gc::Collector& c) override;

 private:
  Value receiver;
  int32_t method;
};

}  // namespace cilly

#endif  // CILLY_VM_CPP_OBJECT_H_