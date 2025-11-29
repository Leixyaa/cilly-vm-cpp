# cilly-vm-cpp

一个用 C++17 手写的「小型字节码虚拟机 + 迷你脚本语言原型」，用于学习编译器 / 虚拟机原理和巩固现代 C++（智能指针、`std::variant`、模板、二进制 IO 等）。

目前项目主要完成 **运行时虚拟机（VM） 和 字节码执行**，并开始实现 **字节码的二进制序列化**，下一步会逐步往“完整编译器 + 解释器前端”方向扩展。

---

## 开发环境与工程规范

- **语言标准**：C++17  
- **IDE / 编译器**：Visual Studio 2022（MSVC）
- **版本管理**：Git + GitHub（commit 信息使用中文，分阶段记录）
- **代码风格**：参考 Google C++ Style Guide
  - 头文件 include 顺序：先 STL / 标准库，再本项目头文件
  - 命名：
    - 类型：`ValueType`, `StackStats`, `CallFrame`（大写驼峰）
    - 函数 / 变量：`push_count_`, `LocalCount()`, `Run()` 等
  - 头文件保护宏：`CILLY_VM_CPP_XXX_H_`
  - 命名空间：`namespace cilly { ... }  // namespace cilly`
- 整体代码量：约 **1k 行 C++**（含自测代码），持续增加中。

---

## 整体架构概览

项目目前分为几层：

1. **Value 层：动态类型运行时值系统**
2. **StackStats 层：操作数栈 + 统计信息**
3. **Chunk 层：字节码容器（指令流 + 常量池 + 行号表）**
4. **Function / CallFrame 层：函数抽象与调用栈**
5. **VM 层：字节码解释执行循环**
6. **Bytecode Stream 层：二进制读写工具（为后续“把字节码写入文件”做准备）**
7. **自测用例：覆盖表达式计算、变量、函数调用、条件跳转、互递归、比较与逻辑等**

下面按模块简单介绍。

---

## 1. Value：动态类型运行时系统（`value.h` / `value.cc`）

代表脚本语言中的运行时值，目前支持四种类型：

```cpp
enum class ValueType { kNull, kBool, kNum, kStr };
