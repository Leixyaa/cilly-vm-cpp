//object.h
#ifndef CILLY_VM_CPP_OBJECT_H_
#define CILLY_VM_CPP_OBJECT_H_
//内存所有权
// 
//方案 1（入门版）：std::shared_ptr<Obj> 存在 Value 里
//好处：先不用自己数引用计数。
//坏处：以后做真正 GC 时要把它替换掉。
//
//方案 2（进阶版）：Obj* + VM 自己维护 std::vector<Obj*> heap_
//好处：以后做 mark-sweep 很自然。
//坏处：现在需要你更小心地管理 new/delete（不过我们会渐进过渡）
#include <unordered_map>
#include <vector>
#include <string>
#include <memory>
#include "value.h"


namespace cilly {

enum class ObjType { kString, kList, kDict};  //暂时先实现List Dict

class Object {
 public:

  Object(ObjType type, int ref_count) :  type_(type), ref_count_(ref_count){}

  ObjType Type() const{ return type_; }
  virtual std::string ToRepr() const { return ""; }

  virtual ~Object() = default;
  //mark bit,next	 // 以后可添加mark bit(GC 用)，指针next等

 protected:
   ObjType type_;     // 表示类型
   // 当前阶段：真正的内存管理交给 std::shared_ptr<Object>，
   // 这里的 ref_count_ 暂时不使用，留给以后实现自定义 GC / 引用计数。
   int ref_count_;     
};
 


class ObjString : public Object {
public:

  ObjString() : Object(ObjType::kString, 1), value_(""){};

  explicit ObjString(std::string string_value) 
    : Object(ObjType::kString, 1), value_(std::move(string_value)) {}  
  
  void Set(const std::string& s) { value_ = s; }

  std::string ToRepr() const override{ return value_; }

  ~ObjString() override = default;

private:
  std::string value_;
};




class ObjList : public Object { 
public:
  ObjList() : Object(ObjType::kList, 1), element() {};
  
  explicit ObjList(std::vector<Value> elem) 
    : Object(ObjType::kList, 1), element(std::move(elem)) {};

  int Size() const { return static_cast<int>(element.size()); }
  void Push(const Value& v) {element.push_back(v); }
  const Value& At(int index) const { return element[index]; }
  void Set(int index, const Value& v) { element[index] = v; }

  std::string ToRepr() const override;

  ~ObjList() override = default;
private: 
  std::vector<Value> element;
};   // List




class ObjDict : public Object {
public:
  ObjDict() : Object(ObjType::kDict, 1), entries_() {};
  
  explicit ObjDict(std::unordered_map<std::string, Value> index) 
      : Object(ObjType::kDict, 1), entries_(std::move(index)) {}

  // 写入/修改键值
  // 如果 key 已存在，就覆盖原来的值。
  void Set(const std::string& key, const Value& value) { entries_[key] = value; }

  // 查询是否存在
  bool Has(const std::string& key) const { return entries_.find(key) != entries_.end(); }

  // 读取：返回指定 key 对应的 Value
  // 如果不存在，就返回 Value::Null()
  Value Get(const std::string& key) const;

  void Erase(const std::string& key) { entries_.erase(key); }

  int Size() const { return static_cast<int>(entries_.size()); }

  std::string ToRepr() const override;

  ~ObjDict() override = default;

private:
  std::unordered_map<std::string, Value> entries_;

 };  //Dic

}  // namespace cilly

#endif  // CILLY_VM_CPP_OBJECT_H_