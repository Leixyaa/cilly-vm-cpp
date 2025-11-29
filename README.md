# cilly-vm-cpp

一个用 C++17 实现的**栈式字节码虚拟机**学习项目，目标是为自制脚本语言 `cilly` 搭建运行时内核，并逐步扩展到「编译器 + 虚拟机」的完整链路。

项目主要特点：

- 使用 **C++17**、遵循 **Google C++ Style Guide**（命名、include 顺序、命名空间注释等）
- 实现了 **动态类型 Value 系统**、**字节码表示 (Chunk)**、**调用帧 (CallFrame)** 和 **虚拟机 (VM)**  
- 支持 **算术运算、比较与逻辑、局部变量、函数调用、条件跳转、互递归** 等
- 支持 **Value / Chunk 的二进制序列化与反序列化**，可以将字节码保存到文件再读回校验

---

## 1. 工程与环境

- 仓库地址：`cilly-vm-cpp`
- 开发环境：
  - Visual Studio 2022（MSVC，C++17）
  - Git + GitHub 做版本管理（commit 信息中文为主，记录清晰）
- 代码风格：
  - 尽量遵循 Google C++ Style Guide：
    - 头文件保护宏：`CILLY_VM_CPP_xxx_H_`
    - `namespace cilly { ... }  // namespace cilly`
    - 类名：大写驼峰 (`StackStats`, `Function`, `VM` 等)
    - 成员变量：下划线结尾 (`type_`, `data_`, `local_count_` 等)
    - 函数与局部变量：小写加下划线（或简洁小写）

---

## 2. 当前支持的能力（语言层面）

从虚拟机视角，目前这个 mini 语言已经支持：

- **值类型（Value）**
  - `null`
  - `bool`（true / false）
  - `number`（内部用 `double`）
  - `string`

- **运算指令**
  - 算术：`OP_ADD`、`OP_SUB`、`OP_MUL`、`OP_DIV`、`OP_NEGATE`
  - 比较：`OP_EQ`、`OP_NOT_EQUAL`、`OP_GREATER`、`OP_LESS`
  - 逻辑：`OP_NOT`

- **控制流与函数**
  - `OP_JUMP`、`OP_JUMP_IF_FALSE`
  - `OP_CALL`、`OP_RETURN`
  - 支持局部变量与参数：`OP_LOAD_VAR`、`OP_STORE_VAR`
  - 已经可以写出：
    - if / else 结构（通过字节码和回填实现）
    - 简单函数调用
    - 带参数的函数调用
    - `odd` / `even` 互递归调用

- **输出与调试**
  - `OP_PRINT`：打印栈顶 Value 的文本表示（不弹栈）
  - 栈统计：记录 push / pop 次数和最大栈深度

---

## 3. 项目结构概览

核心头文件与实现文件：

- `value.h` / `value.cc`  
  动态类型值系统 `Value` 的定义与实现（包括序列化）。

- `stack_stats.h` / `stack_stats.cc`  
  运行时栈 `StackStats`，同时记录 push/pop 次数与最大栈深度。

- `opcodes.h`  
  字节码指令枚举 `OpCode`，统一使用 `int32_t` 作为底层类型。

- `chunk.h` / `chunk.cc`  
  字节码容器 `Chunk`，保存：
  - `code_`：指令 + 操作数序列（`int32_t`）
  - `const_pool_`：常量池（`Value`）
  - `line_info_`：每个 `code_` 元素对应的源代码行号  
  同时提供 `Save` / `Load` 用于序列化。

- `function.h` / `function.cc`  
  函数对象 `Function`，封装：
  - 函数名 `name_`
  - 参数个数 `arity_`
  - 字节码块 `std::unique_ptr<Chunk> chunk_`
  - 局部变量总数 `local_count_`

- `call_frame.h`  
  调用帧 `CallFrame`，包含：
  - 当前函数指针 `fn`
  - 指令指针 `ip`
  - 返回地址 `ret_ip`（预留）
  - 局部变量表 `locals_`

- `vm.h` / `vm.cc`  
  虚拟机 `VM` 本体，包含：
  - 运行时栈 `StackStats stack_`
  - 调用栈 `std::vector<CallFrame> frames_`
  - 函数表 `std::vector<const Function*> functions_`
  - 执行循环 `Run` 与单步执行 `Step_`  
  - 一个简易反汇编函数 `DisassembleChunk`（打印指令与行号）

- `bytecode_stream.h`  
  通用二进制读写工具：
  - `BytecodeWriter`：`Write<T>()`、`WriteString()`
  - `BytecodeReader`：`Read<T>()`、`ReadString()`

- `main.cc`  
  一系列用于自测的测试函数：`ValueTest`、`StackTest`、`ChunkTest`、`FunctionTest`、`VMTest`、`VarTest`、`CallTest`、`CallWithArgTest`、`EqTest`、`IfTest`、`OddEvenTest`、`CompareTest`、`ChunkSerializationTest` 等。

---

## 4. 核心模块详解

### 4.1 Value：动态类型与序列化

`Value` 内部结构：

- `ValueType type_ = ValueType::kNull;`
- `std::variant<std::monostate, bool, double, std::string> data_;`

提供：

- 工厂方法：
  - `Value::Null()`
  - `Value::Bool(bool)`
  - `Value::Num(double)`
  - `Value::Str(std::string)`
- 类型判断：`IsNull()` / `IsBool()` / `IsNum()` / `IsStr()`
- 取值：`AsBool()` / `AsNum()` / `AsStr()`
- 文本表示：`ToRepr()`（`null` / `true` / `false` / 数字 / 字符串）
- 比较：`operator==` / `operator!=`（仅在同类型下比较内容）

**序列化接口：**

- `void Save(BytecodeWriter& writer) const;`
- `static Value Load(BytecodeReader& reader);`

协议约定：

1. 先写入 1 字节的类型标记（`ValueType` → `uint8_t`）。
2. 再写入具体数据：
   - `kNull`：无数据
   - `kBool`：写入 `bool`
   - `kNum`：写入 `double`
   - `kStr`：调用 `WriteString` 写入长度 + 内容

`Load` 按同样顺序读取，并根据类型构造出新的 `Value`。  
遇到未知类型标记时，会输出错误信息并安全返回 `Null`。

---

### 4.2 StackStats：带统计的运行时栈

- 基于 `std::vector<Value>` 实现。
- 额外统计：
  - `push_count_`
  - `pop_count_`
  - `max_depth_`
- 接口：
  - `Push(const Value&)`
  - `Pop()`
  - `Top() const`
  - `Depth() const`
  - `MaxDepth() const`
  - `PushCount() const`
  - `PopCount() const`
  - `ResetStats()`

用于观察不同程序片段的栈使用情况和最大深度，方便后续优化。

---

### 4.3 Chunk：字节码表示与序列化

`Chunk` 负责表示一段可执行字节码：

- `code_`：`std::vector<int32_t>`，存 OpCode 与操作数
- `const_pool_`：`std::vector<Value>`
- `line_info_`：`std::vector<int32_t>`，记录每个 `code_` 元素对应的源行号

主要接口：

- 写入：
  - `Emit(OpCode op, int src_line)`
  - `EmitI32(int32_t v, int src_line)`
  - `AddConst(const Value& v)` → 返回常量索引
  - `PatchI32(int index, int32_t value)` → 回填跳转地址

- 读取：
  - `CodeSize()`, `ConstSize()`
  - `CodeAt(int index)`
  - `ConstAt(int index)`
  - `LineAt(int index)`

**序列化接口：**

- `void Save(BytecodeWriter& writer) const;`
- `static Chunk Load(BytecodeReader& reader);`

文件格式约定：

1. `int32_t const_pool_size`
2. 依次写入 `const_pool_`：每个常量调用 `Value::Save`
3. `int32_t code_size`
4. 写入 `code_size` 个 `int32_t`（字节码）
5. `int32_t line_info_size`
6. 写入 `line_info_size` 个 `int32_t`（行号）

`Load` 按此格式依次读回，构建 `Chunk` 对象。

---

### 4.4 Function 与 CallFrame

`Function` 封装一段可调用的字节码：

- `name_`：函数名（如 `"main"`、`"odd"`、`"even"`）
- `arity_`：参数个数
- `chunk_`：`std::unique_ptr<Chunk>`
- `local_count_`：局部变量总数（包括参数）

提供快捷转发接口：

- `Emit` / `EmitI32` / `AddConst` / `PatchI32`
- `CodeSize()` / `chunk()` / `LocalCount()` 等

`CallFrame` 是虚拟机调用栈中的一帧：

- `const Function* fn`
- `int ip`
- `int ret_ip`
- `std::vector<Value> locals_`

---

### 4.5 VM：虚拟机执行循环

`VM` 内部：

- `StackStats stack_`
- `std::vector<CallFrame> frames_`
- `std::vector<const Function*> functions_`

执行流程：

- `Run(const Function& fn)`：
  - 清空调用栈
  - 为入口函数创建 `CallFrame`，初始化 `locals_`
  - 重置栈统计
  - 循环调用 `Step_()` 知道返回 `false` 或指令耗尽

- `Step_()`：
  - 读取一条指令（`OpCode`）
  - 使用 `switch` 分派到对应实现
  - 处理常量、算术、比较、逻辑、跳转、变量读写、函数调用和返回等

- `RegisterFunction(const Function* fn)`：
  - 注册函数到 `functions_`，返回函数 ID（索引），供 `OP_CALL` 使用。

此外实现了简易反汇编 `DisassembleChunk(const Chunk&)`，便于调试字节码与行号。

---

## 5. 自测用例（main.cc）

目前在 `main.cc` 中实现了多个自测函数，用来验证各模块功能：

- `ValueTest()`：测试 `Value` 的构造、比较与 `ToRepr()`。
- `StackTest()`：测试 `StackStats` 的压栈/弹栈和统计信息。
- `ChunkTest()`：测试 `Chunk` 的常量池、指令流与行号。
- `FunctionTest()`：测试 `Function` 对 `Chunk` 的封装。
- `VMTest()`：构造简单算式 `10 - 2 * 3 / 2` 并执行。
- `VarTest()`：测试局部变量存储与读取。
- `CallTest()`：测试无参函数调用与返回值。
- `CallWithArgTest()`：测试带一个参数的函数调用。
- `EqTest()`：测试 `OP_EQ` 相等比较。
- `IfTest()`：测试条件跳转（`OP_JUMP_IF_FALSE` + 回填）。
- `OddEvenTest()`：测试 `odd` / `even` 互递归函数。
- `CompareTest()`：测试 `>`、`<`、`!=`、`!` 相关指令。
- `ChunkSerializationTest()`：测试 `Chunk` 的序列化/反序列化是否保持内容一致。

`main()` 中可以按需选择调用这些测试来观察行为。

---

## 6. 编译与运行

以 Visual Studio 2022 为例：

1. 克隆仓库：
   ```bash
   git clone https://github.com/Leixyaa/cilly-vm-cpp.git
