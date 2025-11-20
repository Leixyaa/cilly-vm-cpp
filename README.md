# cilly-vm-cpp

一个用 C++17 从零实现的**栈式字节码虚拟机（VM）**小项目，参考了 Python / Lua 等虚拟机的整体思路，并遵守 Google C++ Style Guide 编写。


---

## 已完成功能（截至目前）

### 1. Value：运行时动态类型系统
- 支持 `Null / Bool / Number / String` 四种类型；
- 使用 `std::variant` 存储数据；
- 提供 `ToRepr()` 用于打印调试；
- 支持同类型相等比较运算。

### 2. StackStats：带统计信息的运行时栈
- 内部使用 `std::vector<Value>` 实现栈；
- 额外记录：
  - `PushCount`：压栈次数；
  - `PopCount`：出栈次数；
  - `Depth`：当前栈深度；
  - `MaxDepth`：执行过程中的最大栈深度；
- 在 VM 运行结束后，可打印这些统计信息，用于理解程序复杂度和调试。

### 3. Chunk：字节码与常量池
- 持有一段可执行字节码序列 `code_`；
- 持有常量池 `const_pool_`（例如数值常量）；
- 每个字节码单元附带源代码行号信息，便于出错时定位。

### 4. Function：函数对象
- 封装：
  - 函数名（`name_`）；
  - 形参数量（`arity_`）；
  - 局部变量数量（`local_count_`）；
  - 对应的字节码块 `Chunk`；
- 提供便捷方法向内部 `Chunk` 写入指令和常量；
- 使用 `std::unique_ptr<Chunk>` 表示唯一拥有权。

### 5. VM：栈式虚拟机（Stack-based VM）
- 支持的指令（目前）：
  - `OP_CONSTANT`：压入常量池中的值；
  - `OP_LOAD_VAR` / `OP_STORE_VAR`：按索引读写局部变量；
  - `OP_ADD / OP_SUB / OP_MUL / OP_DIV`：四则运算；
  - `OP_NEGATE`：一元负号；
  - `OP_PRINT`：打印栈顶（peek，不弹栈）；
  - `OP_RETURN`：函数返回（当前版本为简单版本）；
- 使用 `StackStats` 作为运行时操作数栈，自动记录压栈/出栈行为；
- 使用 `locals_` 向量作为当前函数的局部变量表。

### 6. 反汇编器（Disassembler，简易版）
- 可以把 `Chunk` 中的字节码打印成人类可读形式：
  - 指令名；
  - 操作数（如常量索引）；
  - 对应的源代码行号；
- 便于调试编译器输出的字节码是否正确。

---

## 示例：简单算术 + 变量

在 `main.cc` 中，目前已经可以构造如下程序的字节码并在 VM 中执行：

```text
// 伪代码示意：
// var a = 10;
// var b = 20;
// print(a + b);  // 输出 30
