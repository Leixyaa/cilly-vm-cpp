# README_GTEST — GoogleTest（gtest）配置与使用指南（Bazel/Bzlmod）

本仓库使用 **GoogleTest（gtest）** 作为**唯一**的自动化测试入口，配合 **Bazelisk + Bazel + Bzlmod** 管理依赖与构建/测试。

你需要知道的核心点：
- ✅ 所有回归/冒烟/单元测试都写在 `tests/` 下的 gtest（不再把手写自测塞进 `main.cc`）
- ✅ 运行测试统一用 `bazelisk test ...`
- ✅ `milestone_smoke_test` 是“里程碑冒烟”入口：覆盖关键全功能链路

---

## 0. 你需要准备什么（Windows）

### 必备软件
- Windows 10/11
- Git for Windows
- Visual Studio 2022（MSVC 编译器工具链）
- **Bazelisk**（用来自动下载并使用 `.bazelversion` 指定的 Bazel 版本）
- Git Bash（用于 Bazel 在 Windows 下的 `BAZEL_SH`，建议装 Git for Windows）

### 本仓库约定
- `.bazelversion`：固定 Bazel 版本（Bazelisk 会自动下载并使用）
- `MODULE.bazel`：Bzlmod 依赖管理（googletest 通过这里接入）
- `tests/BUILD.bazel`：所有 gtest `cc_test(...)` 的定义

---

## 1. 一次性环境配置（Windows）

### Step 1：确认 Bazelisk 可用
在 PowerShell 里执行：
```powershell
bazelisk --version
````

如果能输出版本号，说明 Bazelisk 已安装成功。

> 注意：不要在 PowerShell 里直接输入 `build` / `test`，必须写 `bazelisk build ...` / `bazelisk test ...`

---

### Step 2：配置 BAZEL_SH（Windows 必做）

Bazel 在 Windows 下通常需要一个 bash（来自 Git for Windows）。

在 PowerShell 中执行（按你的实际路径调整）：

````powershell
$env:BAZEL_SH="D:\Program Files\Git\bin\bash.exe"
setx BAZEL_SH "D:\Program Files\Git\bin\bash.exe"
````

验证：

````powershell
echo $env:BAZEL_SH
Test-Path $env:BAZEL_SH
````

> `setx` 设置的是持久环境变量；设置后建议重开一个终端窗口再继续。

---

### Step 3：确认仓库能正常构建

在仓库根目录执行：

````powershell
cd D:\dev\cilly-vm-cpp
bazelisk build //...
````

---

## 2. gtest 是如何接入本仓库的

### 2.1 依赖声明：MODULE.bazel（Bzlmod）

* `MODULE.bazel` 中通过 `bazel_dep(name="googletest", version="...")` 声明依赖
* Bazel 会自动拉取对应版本的 googletest

### 2.2 代码库拆分：src 是库，tests 只依赖库

* `src/BUILD.bazel` & `src/frontend/BUILD.bazel` 把工程代码组织成 `cc_library`
* `tests/BUILD.bazel` 用 `cc_test(...)` 编译测试，并依赖 `//src:xxx` 之类的库 target
* 好处：tests 不需要自己把 src 的一堆源文件再编一次，增量构建快、依赖清晰

### 2.3 测试入口：tests/ 下的多个 cc_test

* 每个测试文件对应一个 Bazel 的 test target
* 你可以跑单个测试，也可以跑 tests:all

---

## 3. 常用命令

> 以下命令都在仓库根目录执行：`D:\dev\cilly-vm-cpp`

### 3.1 构建全部（编译所有 target）

````powershell
bazelisk build //...
````

### 3.2 只构建可执行（如果仓库提供 //src:cilly 之类目标）

````powershell
bazelisk build //src:cilly
````

### 3.3 跑全部单测（推荐带输出）

````powershell
bazelisk test //tests:all --test_output=all
````

### 3.4 只跑里程碑冒烟（最常用的 smoke）

````powershell
bazelisk test //tests:milestone_smoke_test --test_output=all
````

### 3.5 只跑某一个 test target（示例）

````powershell
bazelisk test //tests:value_stack_test --test_output=all
````

---

## 4. tests/ 目录结构与约定

典型结构：

* `tests/hello_test.cc`：冒烟/示例测试（用于验证 gtest 链路）
* `tests/value_stack_test.cc`：某模块的单元测试
* `tests/milestone_smoke_test.cc`：里程碑级冒烟测试（覆盖“全功能链路”）

### 仓库约束（强约束）

* ✅ 以后新增功能的回归/冒烟都写进 gtest
* ❌ 不再把“大量手写自测函数”塞回 `main.cc`
* ✅ 里程碑 smoke 默认集中在 `milestone_smoke_test.cc`（或后续拆成多个 smoke suite 也行）

---

## 5. 如何新增一个 gtest

下面以新增 `tests/vm_call_test.cc` 为例。

### Step 1：新建测试源文件

在 `tests/` 下新建 `vm_call_test.cc`：

````cpp
#include <gtest/gtest.h>

// 视你仓库的头文件组织情况，按需 include
#include "src/vm.h"
#include "src/value.h"

TEST(VMCallTest, CallNativeOrUserFunction) {
  // Arrange: 构造 VM / 注册 native / 准备脚本或字节码
  // Act:     执行
  // Assert:  检查栈顶值、返回值、错误码等
  SUCCEED();
}
````

> 建议：尽量用“Arrange-Act-Assert”的结构写，失败时更好定位。

---

### Step 2：在 tests/BUILD.bazel 添加一个 cc_test target

打开 `tests/BUILD.bazel`，仿照已有 test 添加：

````bzl
cc_test(
    name = "vm_call_test",
    srcs = ["vm_call_test.cc"],
    deps = [
        "//src:cilly_core",          # 以你仓库实际库 target 名为准
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)
````

如果你仓库里没有 `//src:cilly_core`，就换成你已有的 core library target（例如 `//src:vm`、`//src:frontend` 等）。

---

### Step 3：运行这个新测试

````powershell
bazelisk test //tests:vm_call_test --test_output=all
````

通过后，再跑一次全量回归：

````powershell
bazelisk test //tests:all --test_output=all
````

---

## 6. 如何组织测试

为了后续维护方便，建议按模块分层：

* `value_*_test.cc`：Value/Object/List/Dict 等基础类型
* `chunk_*_test.cc`：字节码、常量表、跳转 patch 等
* `vm_*_test.cc`：执行/调用/栈/错误处理
* `frontend_*_test.cc`：lexer/parser/generator
* `e2e_*_test.cc`：端到端（脚本字符串→编译→运行→断言输出）
* `milestone_smoke_test.cc`：覆盖关键全链路的冒烟用例（少而精）

命名建议：

* 文件：`xxx_test.cc`
* Bazel 目标：与文件同名（去掉 `.cc`）
* TEST 名：`ModuleNameTest, CaseName`

---

## 7. 运行时输出与定位失败

### 7.1 打印更完整的测试输出

已经在用：

````powershell
--test_output=all
````

### 7.2 只看失败的测试日志

Bazel 会在失败时打印日志路径；你也可以重新跑该 target：

````powershell
bazelisk test //tests:milestone_smoke_test --test_output=all
````

---

## 8. 常见问题（Troubleshooting）

### Q1：PowerShell 输入 `build` / `test` 报“无法识别 cmdlet”？

这是 PowerShell 不是 Bazel 命令行。必须：

* `bazelisk build ...`
* `bazelisk test ...`

---

### Q2：`bazelisk build --cxxopt=/std:c++17` 提示 empty set of targets？

因为你没写目标。正确写法例如：

````powershell
bazelisk build //...
bazelisk build //src:cilly --cxxopt=/std:c++17
````

---

### Q3：Bazel server 提示 startup options 不一致要 kill？

这是 Bazel 常见行为。一般直接按提示让它重启即可。
如果状态异常，可尝试：

````powershell
bazelisk shutdown
````

---

### Q4：Windows 下构建失败，像是缺 bash？

确认你已设置 `BAZEL_SH` 指向 Git Bash，例如：
`D:\Program Files\Git\bin\bash.exe`

---

## 9. 本仓库的测试哲学

* 单测要“快”：能单跑、能增量跑
* 冒烟要“少而硬”：`milestone_smoke_test` 覆盖关键路径（Native/CallV/Frontend→VM/E2E）
* 任何 bug 修复都应带一个回归用例（先写 failing test，再修代码）