# README_GTEST.md — Bazel + Bazelisk + GoogleTest 接入说明（Windows）

本项目已在 Windows 上完成 Bazel/Bazelisk + GoogleTest（gtest）接入，并能通过 Bazel 统一构建与执行单元测试与冒烟测试。

目标：
- 不再把测试写进 `src/main.cc`（不再手写 `XXXSmokeTest()`）
- 所有冒烟测试/回归测试统一放到 `tests/*.cc` 的 gtest 中
- 通过 `bazelisk test //tests:all` 一键运行所有测试

---

## 1. 总体结构与依赖关系

### 1.1 Bazel 目标（Targets）之间如何连接

- `//src:cilly_core`  
  后端核心库：VM/Value/Object/Chunk/Function/StackStats 等（cc_library）

- `//src/frontend:frontend`  
  前端库：Lexer/Parser/Generator（cc_library），依赖 `//src:cilly_core`

- `//src:cilly`  
  可执行文件（cc_binary），依赖：
  - `//src:cilly_core`
  - `//src/frontend:frontend`

- `//tests:*`  
  单元测试（cc_test），每个测试依赖：
  - `@googletest//:gtest` + `@googletest//:gtest_main`
  - 以及它要测的工程库（例如 `//src:cilly_core`、或再加 `//src/frontend:frontend`）

- `//tests:all`  
  测试套件（test_suite），用于一次性运行全部 tests

### 1.2 文件组织建议

```

repo_root/
.bazelversion
MODULE.bazel
.bazelrc                
src/
BUILD.bazel            (cilly_core + cilly binary)
frontend/
BUILD.bazel          (frontend lib)
tests/
BUILD.bazel            (hello_test / value_stack_test / milestone_smoke_test / all)
hello_test.cc
value_stack_test.cc
milestone_smoke_test.cc

````

---

## 2. 接入步骤回顾

### 2.1 Bazelisk（代替手动安装 Bazel）

结论：无需单独安装 Bazel，只需要 Bazelisk。
Bazelisk 会自动下载并管理 Bazel 版本。

由于直接使用 `latest` 可能因访问 Google GCS 超时失败，所以固定 Bazel 版本。

### 2.2 固定 Bazel 版本：`.bazelversion`

在仓库根目录创建：

```txt
8.5.0
````

这样执行 `bazelisk` 时会自动使用（并下载） Bazel 8.5.0。

### 2.3 引入 GoogleTest：`MODULE.bazel`

使用 Bzlmod（而非 WORKSPACE）引入 googletest：

```py
bazel_dep(name = "googletest", version = "1.17.0.bcr.2")
```

此后可以在 BUILD 里用：

* `@googletest//:gtest`
* `@googletest//:gtest_main`

### 2.4 Windows 必要配置：`BAZEL_SH`

Windows 下 Bazel 需要一个 bash（shell toolchain），通常使用 Git Bash：

PowerShell 临时设置：

```powershell
$env:BAZEL_SH="D:\Program Files\Git\bin\bash.exe"
```

永久设置：

```powershell
setx BAZEL_SH "D:\Program Files\Git\bin\bash.exe"
```

---

## 3. 工程 BUILD 文件（关键：把源码变成可依赖的库）

### 3.1 `src/BUILD.bazel`

定义核心库 `cilly_core`（供测试/前端/可执行依赖）：

```bazel
cc_library(
    name = "cilly_core",
    srcs = [
        "chunk.cc",
        "function.cc",
        "object.cc",
        "stack_stats.cc",
        "value.cc",
        "vm.cc",
    ],
    hdrs = [
        "bytecode_stream.h",
        "call_frame.h",
        "chunk.h",
        "function.h",
        "object.h",
        "opcodes.h",
        "stack_stats.h",
        "value.h",
        "vm.h",
    ],
    includes = ["."],
    visibility = ["//visibility:public"],
)

cc_binary(
    name = "cilly",
    srcs = ["main.cc"],
    deps = [
        ":cilly_core",
        "//src/frontend:frontend",
    ],
)
```

### 3.2 `src/frontend/BUILD.bazel`

把前端作为独立库 `frontend`：

```bazel
cc_library(
    name = "frontend",
    srcs = glob(["*.cc"]),
    hdrs = glob(["*.h"]),
    includes = ["."],
    deps = [
        "//src:cilly_core",
    ],
    visibility = ["//visibility:public"],
)
```

---

## 4. tests/：如何写 gtest + 如何让 Bazel 发现并运行

### 4.1 `tests/BUILD.bazel` 示例

```bazel
cc_test(
    name = "hello_test",
    srcs = ["hello_test.cc"],
    deps = [
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "value_stack_test",
    srcs = ["value_stack_test.cc"],
    deps = [
        "//src:cilly_core",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "milestone_smoke_test",
    srcs = ["milestone_smoke_test.cc"],
    deps = [
        "//src:cilly_core",
        "//src/frontend:frontend",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

test_suite(
    name = "all",
    tests = [
        ":hello_test",
        ":value_stack_test",
        ":milestone_smoke_test",
    ],
)
```

> 注意：如果 test 需要走 “source → lexer → parser → generator → vm.run” 的集成路径，
> 必须依赖 `//src/frontend:frontend` + `//src:cilly_core`。

---

## 5. 常用命令

### 5.1 构建

构建全部：

```powershell
bazelisk build //...
```

只构建可执行：

```powershell
bazelisk build //src:cilly
```

只构建 tests：

```powershell
bazelisk build //tests:all
```

> 重要：Bazel 命令必须带目标（targets），否则会提示 empty set of targets。

### 5.2 运行测试

跑单个测试：

```powershell
bazelisk test //tests:milestone_smoke_test --test_output=all
```

跑全部测试：

```powershell
bazelisk test //tests:all
```

只跑某个 gtest case（过滤）：

```powershell
bazelisk test //tests:milestone_smoke_test --test_arg=--gtest_filter=Milestone.*
```

---

## 6. 推荐：固化 C++17 与测试输出（.bazelrc）

为了避免每次都要写 `--cxxopt=/std:c++17`，并减少 “options changed 丢缓存” 提示，
建议在仓库根目录创建 `.bazelrc`：

```txt
build --cxxopt=/std:c++17
test  --cxxopt=/std:c++17
test  --test_output=errors
```

之后即可直接：

```powershell
bazelisk test //tests:all
```

---

## 7. 常见问题

### 7.1 PowerShell 里直接输入 `build`/`test` 会报错

错误原因：`build` 不是 PowerShell 命令，而是 Bazel 子命令。

正确用法：

```powershell
bazelisk build //...
bazelisk test //tests:all
```

### 7.2 `bazelisk build --cxxopt=/std:c++17` 提示 empty set of targets

原因：没写目标。应写：

```powershell
bazelisk build --cxxopt=/std:c++17 //...
```

### 7.3 “test size too big / timeout warnings”

这是 Bazel 根据运行耗时给的 metadata 建议，不影响 PASS。
如果想消除提示，可以在 `cc_test` 里增加：

```bazel
size = "small",
timeout = "short",
```

