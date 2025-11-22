# cilly-vm-cpp

一个用 **C++（遵循 Google C++ Style）重写的教学虚拟机项目**。  
最初源于编译原理课程的 `cilly_vm` 作业，我在此基础上：

- 用面向对象方式重构了运行时数据结构（Value / Chunk / Function / VM 等）；
- 实现了栈统计、变量系统、函数调用、条件跳转等功能；
- 支持 odd/even 互递归，用字节码真实地跑起来。

这个项目会写进我的简历，用来展示我在 C++ 和编译器/虚拟机方向的理解与实践。

---

## 功能概览

当前 VM 支持的主要能力：

- **运行时值系统（Value）**
  - 支持 `null / bool / number / string` 四种类型。
  - 使用 `std::variant` 封装，提供类型判断与取值接口。
  - 提供 `ToRepr()` 做调试打印。

- **字节码容器（Chunk）**
  - 维护指令序列 `code_`、常量池 `const_pool_` 与行号 `line_info_`。
  - 提供 `Emit / EmitI32 / AddConst / PatchI32` 等接口。
  - 支持 **跳转指令操作数的“占位 + 回填”（Patch）**。

- **函数对象（Function）**
  - 封装函数名、形参数量 `arity` 和内部的 `Chunk`。
  - 支持设置局部变量总数 `SetLocalCount`（参数 + 局部变量）。
  - 作为 VM 可调用单元，通过 `VM::RegisterFunction` 注册。

- **运行时栈（StackStats）**
  - 封装了一个 `std::vector<Value>` 作为栈。
  - 额外统计：
    - `PushCount / PopCount`
    - `Depth / MaxDepth`
  - 方便分析不同程序在 VM 上的栈使用情况。

- **虚拟机（VM）**
  - 支持的指令包括：
    - 常量与栈操作：`OP_CONSTANT / OP_POP / OP_DUP`
    - 算术：`OP_ADD / OP_SUB / OP_MUL / OP_DIV / OP_NEGATE`
    - 比较与逻辑：`OP_EQ`（预留了 `OP_NOT_EQUAL / OP_GREATER / OP_LESS / OP_NOT`）
    - 变量：`OP_LOAD_VAR / OP_STORE_VAR`
    - 调用：`OP_CALL / OP_RETURN`
    - 条件跳转：`OP_JUMP / OP_JUMP_IF_FALSE`
    - 输出：`OP_PRINT`
  - 使用 **调用帧（CallFrame）** 存储：
    - 当前函数指针；
    - 指令指针 `ip`；
    - 当前帧的局部变量表 `locals_`。
  - 通过 `frames_` 向量维护调用栈，支持递归调用。

---

## 自测用例（tests）

在 `main.cc` 中包含一系列自测函数，每个函数对应一个小特性：

- `ValueTest`：测试 `Value` 封装与比较。
- `StackTest`：测试 `StackStats` 的 push/pop 统计。
- `ChunkTest`：测试 `Chunk` 的字节码与常量池布局。
- `FunctionTest`：测试 `Function` 封装与内部 `Chunk`。
- `VMTest`：测试最小执行循环，例如计算表达式 `10 - 2 * 3 / 2`。
- `VarTest`：测试局部变量系统（`LOAD_VAR / STORE_VAR`）。
- `CallTest`：测试无参数函数调用与返回。
- `CallWithArgTest`：测试带一个参数的函数调用。
- `EqTest`：测试比较指令 `OP_EQ`。
- `IfTest`：测试条件跳转 `OP_JUMP_IF_FALSE + OP_JUMP`。
- `OddEvenTest`：测试 **odd/even 互递归**，是真正的“多帧递归调用”样例。

运行程序时会依次调用上述自测函数，在控制台输出结果。

---

## odd/even 互递归设计说明

目标是用字节码实现如下逻辑：

```javascript
fun even(n) {
  if (n == 0) return true;
  else return odd(n - 1);
}

fun odd(n) {
  if (n == 0) return false;
  else return even(n - 1);
}

// main:
print(even(3));
print(odd(3));
