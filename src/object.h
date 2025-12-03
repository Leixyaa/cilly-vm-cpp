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

  int RefCount() const{ return ref_count_; }

  virtual std::string ToRepr() const = 0;

  virtual ~Object() = default;
  //mark bit,next	 // 以后可添加mark bit(GC 用)，指针next等

 protected:
   ObjType type_;     // 表示类型
   int ref_count_;     // 为gc做准备
};
 


class ObjString : public Object {
public:

  explicit ObjString(std::string string_value) 
    : Object(ObjType::kString, 1), value_(std::move(string_value)) {}

  std::string ToRepr() const override{ return value_; }

private:
  std::string value_;
};




class ObjList : public Object { 
public:

  ObjList() : Object(ObjType::kList, 1), element() {};
  
  explicit ObjList(std::vector<Value> elem) 
    : Object(ObjType::kList, 1), element(std::move(elem)) {};

  int Size() const { return element.size(); }

  void Push(const Value& v) {element.push_back(v); }

  const Value& At(int index) const { return element[index]; }

  void Set(int index, const Value& v) { element[index] = v; }

  std::string ToRepr() const override;

private:
  std::vector<Value> element;
};   // List




class ObjDic : public Object {std::unordered_map<std::string, Value> entries; /*......*/ };  //Dic

}  // namespace cilly

#endif  // CILLY_VM_CPP_OBJECT_H_