# 📦 Project State & Handoff Document

**Project Name:** `cilly-vm-cpp`
**Type:** Handwritten Compiler + Virtual Machine (C++)
**Status:** Active Development
**Last Updated:** Frontend assignment support completed

---

## 1. 项目总览（What this project is）

这是一个从零开始、完全手写的 **C++ 编译器 + 字节码虚拟机项目**，目标是：

* 理解并实现完整的编译链路：
  **Source → Lexer → Parser → AST → Bytecode → VM**
* 不依赖 LLVM / ANTLR 等重型框架
* 逐步从“教学级”演进到“工程级”结构
* 为后续加入：

  * List / Dict
  * for / while
  * GC
  * 模块系统
    打下可扩展基础

项目当前**重点不在语言复杂度，而在结构正确性和可演进性**。

---

## 2. 后端（VM / Runtime）当前状态

### 2.1 核心组件

* `Value`：支持 number / bool / null / object
* `Function` / `Chunk`：存放常量池 + 字节码
* `VM`：基于栈的字节码解释执行器

### 2.2 已支持的字节码指令

* 常量与算术：

  * `OP_CONSTANT`
  * `OP_ADD / OP_SUB / OP_MUL / OP_DIV`
* 栈操作：

  * `OP_POP`
* IO：

  * `OP_PRINT`
* 变量访问：

  * `OP_LOAD_VAR`
  * `OP_STORE_VAR`
* 控制流（已在后端验证）：

  * `OP_JUMP`
  * `OP_JUMP_IF_FALSE`

### 2.3 运行时数据结构

* 已实现：

  * `ObjList`
  * `ObjDict`
* List / Dict 在 VM 层面可正常构造、访问、修改
* for 循环在 **字节码层面** 已可手写测试跑通

---

## 3. 前端（Frontend）整体结构

前端被明确拆分为三个模块：

```
frontend/
 ├── lexer.{h,cc}
 ├── parser.{h,cc}
 ├── generator.{h,cc}
```

设计目标是 **与后端完全解耦，仅通过字节码交互**。

---

## 4. Lexer（词法分析器）

### 4.1 职责

* 将源代码字符串切分为 token 流

### 4.2 当前支持的 token

* 标识符（identifier）
* 数字字面量（整数）
* 关键字：

  * `var`
  * `print`
* 运算符：

  * `+ - * / =`
* 分隔符：

  * `(` `)` `{` `}` `;` `,`
* 自动补 `EOF`

### 4.3 核心接口

```cpp
class Lexer {
 public:
  explicit Lexer(const std::string& source);
  std::vector<Token> ScanAll();
};
```

---

## 5. Parser（递归下降语法分析器）

### 5.1 职责

* 将 token 流转换为 AST（抽象语法树）

### 5.2 AST 结构

#### 语句（Stmt）

* `PrintStmt`
* `VarStmt`
* `AssignStmt`
* `ExprStmt`
* `BlockStmt`（结构已预留）

#### 表达式（Expr）

* `LiteralExpr`
* `VariableExpr`
* `BinaryExpr`

### 5.3 已支持的语法

```c
var x = 10;
print x;
x = x + 1;
print (x + 2) * 3;
```

### 5.4 表达式优先级

```
Expression
 └── Term        (+ -)
     └── Factor  (* /)
         └── Unary
             └── Primary
```

### 5.5 错误处理

* 使用 `Consume()` 校验 token 类型
* 已优化错误输出：

  * 打印错误信息
  * 显示出错 token 与行号
* 当前阶段仍以 `assert` 为主（未引入异常系统）

---

## 6. Generator（AST → Bytecode）

### 6.1 职责

* 将 AST 转换为 VM 可执行的字节码 `Function`

### 6.2 变量管理策略

* 使用：

  ```cpp
  std::unordered_map<std::string, int> locals_;
  int next_local_index_;
  ```
* 所有变量当前视为“函数级 local”（无作用域嵌套）

### 6.3 已支持的生成能力

* `print expr;`
* `var x = expr;`
* `x = expr;`
* 表达式求值（含变量读取）

### 6.4 Generator → VM 全链路验证

以下代码已可正确执行：

```c
var x = 1;
print x;
x = x + 1;
print x;
x = x * 10;
print x;
```

输出：

```
1
2
20
```

---

## 7. 当前里程碑（Milestone）

### ✅ 已完成

* 前端 Lexer / Parser / Generator 完整打通
* 支持变量声明、读取、赋值
* 前端 → 字节码 → VM 全链路可运行
* 赋值语句版本已提交（Git）

### 🟡 当前状态

* 前端功能 **落后于后端一小步**
* 后端已有 List / Dict / for（字节码层）

---

## 8. 下一阶段目标（Next Steps）

### 优先级 1（前端同步后端）

* 前端支持 List / Dict：

  * 构造（字面量或内建函数）
  * 访问（索引 / key）
* Generator 生成对应容器字节码

### 优先级 2（控制流）

* 前端支持 `for` 语句：

  * 数值 for
  * for-in（List / Dict）
* 语法层展开为 jump + 条件

### 优先级 3（长期）

* 作用域系统
* GC（替换 shared_ptr）
* 模块 / import
* 错误恢复（异常系统）

---

## 9. 协作与推进约定（重要）

* ❌ 不一次性给完整代码
* ✅ 给“代码框架 + 实现要点”
* ✅ 用户实现 → 助手检查
* ✅ 每个阶段提醒 Git commit（中文，一句话）
* ✅ 严格 Google C++ Style
* ✅ 小步推进 + 原理解释

---

## 10. 使用方式（切换对话时）

在新对话中：

1. 贴出本文件内容（或精简版）
2. 说明：

   > “这是一个上下文接续项目，请按该状态继续”

即可无缝衔接。
