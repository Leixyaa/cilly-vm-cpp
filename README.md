# cilly-vm-cpp

`cilly-vm-cpp` 是一个使用 C++ 手写的小型语言编译器与虚拟机项目。
项目覆盖了从源码解析到字节码执行的完整流程，主要用于系统性理解编译器前端、字节码设计以及栈式虚拟机的执行模型。

这是一个持续演进的学习型项目，重点不在于功能的完整性，而在于每一步实现都可解释、可扩展、可验证。

---

## 当前已支持的语言特性

### 基础语法

* 变量声明与赋值
* 表达式语句
* `print` 输出语句

示例：

```c
var x = 1;
x = x + 2;
print(x);
```

---

### 数据类型

* Number（double）
* Bool（true / false）
* String
* Null
* List
* Dict（key 为 string）

示例：

```c
var a = [1, 2, 3];
var d = {"k": 10};
d["k"] = a[1];
```

---

### 运算符

#### 算术运算

* `+`
* `-`
* `*`
* `/`

#### 比较与逻辑运算

* `<`
* `>`
* `==`
* `!=`
* `!`（一元逻辑非）

示例：

```c
print(3 > 2);
print(1 != 1);
print(!(3 > 2));
```

条件表达式在运行期必须为 bool 类型，否则会触发断言失败。这是一个刻意的语义约束，用于保持控制流的明确性。

---

### 控制流

* `if / else`
* `while`
* `for`
* `break`
* `continue`

示例：

```c
for (var i = 0; i < 10; i = i + 1) {
  if (i == 3) break;
  print(i);
}
```

实现说明：

* `break` / `continue` 使用 patch jump 的方式生成字节码
* `for` 与 `while` 的 `continue` 行为被区分处理：

  * `while` 跳转回条件判断
  * `for` 跳转到 step 语句

---

## 编译与执行流程

```
Source Code
  → Lexer        (string → tokens)
  → Parser       (tokens → AST)
  → Generator    (AST → bytecode / Function)
  → VM           (bytecode → execution)
```

---

## 项目结构（简化）

```
src/
├── frontend/
│   ├── lexer.*        词法分析
│   ├── parser.*       语法分析
│   ├── ast.h          AST 定义
│   └── generator.*    AST 到字节码的生成
├── vm/
│   ├── vm.*           虚拟机执行逻辑
│   ├── value.*        运行期 Value 系统
│   ├── chunk.*        字节码容器
│   └── function.*     函数与常量池
└── main.cc            测试入口
```

---

## 运行方式

当前通过在 `main.cc` 中直接提供源码字符串进行测试，例如：

```cpp
std::string source =
    "var x = 1;"
    "print(x + 2);";
```

同时支持从文件中读取源码，用于更完整的测试用例。

---

## 测试方式

项目采用逐步 smoke test 的方式推进：

* 每新增一个语言特性
* 增加对应的最小可验证测试
* 确保前端、字节码生成与 VM 行为一致

---

## 计划中的功能

* Block scope 与变量生命周期管理
* 逻辑运算符 `&&` / `||`（短路求值）
* 函数定义与调用
* 更完善的错误处理机制（替代 assert）
* 更独立的脚本运行入口或 REPL

---

## 项目目的

本项目的目的在于：

* 理解表达式优先级与 AST 结构
* 理解控制流如何被编译为跳转指令
* 理解栈式虚拟机的执行模型
* 为后续学习编译原理、虚拟机或语言实现打基础

---

## 备注

代码遵循 Google C++ Style，项目会随着学习进度持续重构与扩展。

