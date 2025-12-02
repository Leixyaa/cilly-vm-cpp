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
#include "value.h"


namespace cilly {

enum class ObjType { kString, kList, kDict};  //暂时先实现List Dict

class object {
 public:
  object(ObjType type_, int ref_count_) :  type_(type_), ref_count_(ref_count_){}

  virtual std::string ToRepr() const = 0;

  virtual ~object() = default;
  //mark bit,next	 // 以后可添加mark bit(GC 用)，指针next等

 protected:
   ObjType type_;     // 表示类型
   int ref_count_;     // 为gc做准备
};
 
class ObjString : public object {
public:
  ObjString(std::string string_value) : object(ObjType::kString, 1), value_(std::move(string_value)) {}

  ObjType Type() const;
  int RefCount() const;

  std::string ToRepr() const override;

private:
  std::string value_;
};


class ObjList : public object { std::vector<Value> element; /*......*/ };   // List

class ObjDic : public object {std::unordered_map<std::string, Value> entries; /*......*/ };  //Dic

}  // namespace cilly

#endif  // CILLY_VM_CPP_OBJECT_H_