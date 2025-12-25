# README_FORMAT — Clang-Format（Google 风格）配置与使用指南

本仓库使用 **clang-format** 统一 C/C++ 代码风格，规则由仓库根目录的 **`.clang-format`** 文件定义（以 **Google C++ Style** 为基础）。

这份文档目标是告知：
- ✅ 在 Windows 上正确安装并启用 clang-format
- ✅ 知道仓库里有哪些格式化脚本、各自用途是什么
- ✅ 学会日常最快的“检查→一键修复→提交”流程
- ✅ 启用 pre-commit 钩子：未格式化禁止提交

---

## 0. 你需要准备什么

- Windows 10/11
- Git for Windows（含 Git Bash）
- PowerShell（Windows 自带即可）
- LLVM（提供 clang-format）

> 说明：  
> - 你可以在 **Git Bash / PowerShell / CMD** 任意终端里用 `git` 提交。  
> - 本仓库的格式化工具脚本是 **PowerShell (.ps1)**，但在 **Git Bash 里也能通过 `powershell -File ...` 调用**。

---

## 1. 安装 clang-format（Windows）

### Step 1：安装 LLVM（包含 clang-format）
打开 PowerShell，执行：

```powershell
winget install LLVM.LLVM
````

### Step 2：验证 clang-format 可用 & PATH 正确

````
where.exe clang-format
clang-format --version
````

期望结果：

* `where.exe clang-format` 输出类似：
  `C:\Program Files\LLVM\bin\clang-format.exe`
* `clang-format --version` 输出版本号

> 常见问题：
> 你的机器上可能同时存在多份 clang-format（例如 VS 自带、VSCode 扩展自带）。
> 只要 `where.exe clang-format` 第一条指向 `C:\Program Files\LLVM\bin\clang-format.exe` 就行。
> 这样全员格式输出更稳定。

---

## 2. 仓库里与格式化相关的文件是什么

仓库根目录：

* `.clang-format`
  clang-format 规则文件（Google 风格为基础）。所有格式化/检查都以它为准。

`scripts/` 目录（仓库自带脚本）：

* `scripts/format.ps1`
  ✅ **全仓库格式化**：扫描并格式化整个仓库的 C/C++ 文件（首次清洗/大重构用）
* `scripts/format_check.ps1`
  ✅ **全仓库检查**：只检查，不修改文件（CI 或全量验证用）
* `scripts/format_changed.ps1`
  ✅ **只格式化 Git 变更文件**（推荐日常使用，速度快）
* `scripts/format_check_changed.ps1`
  ✅ **只检查 Git 变更文件**（推荐提交前使用）

> 日常推荐：优先用 `*_changed` 版本。
> 全仓库版本适合：首次统一格式、或你做了大规模重构。

---

## 3. 第一次使用本仓库：最短上手路径（建议照做）

### Step 1：进入仓库根目录

Git Bash：

````
cd /d/dev/cilly-vm-cpp
````

PowerShell：

````powershell
cd D:\dev\cilly-vm-cpp
````

### Step 2：确认 clang-format 能运行
PowerShell:
````
clang-format --version
````

### Step 3：跑一次“全仓库检查”（不改文件）
PowerShell:
````
powershell -ExecutionPolicy Bypass -File scripts\format_check.ps1
````

如果输出 `All formatted.`，说明当前仓库格式已统一。
如果列出 `NOT FORMATTED`，说明需要格式化（见下一步）。

### Step 4：第一次统一格式（全仓库格式化）
PowerShell:
````
powershell -ExecutionPolicy Bypass -File scripts\format.ps1
````

然后再检查一次确认通过：
PowerShell:
````
powershell -ExecutionPolicy Bypass -File scripts\format_check.ps1
````

---

## 4. 日常开发推荐流程

你平时写代码时，推荐用这一套固定节奏：

### Step A：写代码

正常开发、保存文件即可。

### Step B：提交前检查（只检查你改过的文件，最快）

Git Bash / PowerShell 都可以：
PowerShell:
````
powershell -ExecutionPolicy Bypass -File scripts\format_check_changed.ps1
````

如果输出：

* `No changed files.`：你没改动任何文件
* `All changed files formatted.`：可以放心提交
* 列出 `NOT FORMATTED: ...`：说明这些文件需要格式化，进入 Step C

### Step C：一键修复（只格式化你改过的文件）
PowerShell:
````
powershell -ExecutionPolicy Bypass -File scripts\format_changed.ps1
````

修复后建议再检查一次：
PowerShell:
````
powershell -ExecutionPolicy Bypass -File scripts\format_check_changed.ps1
````

### Step D：正常 git 提交

（在 Git Bash 或 PowerShell 里提交都一样）
Bash:
````
git add -A
git commit -m "your message"
git push
````

---

## 5. 启用 pre-commit（未格式化就禁止提交）

如果你希望“自己忘了格式化也没事，git 会自动拦住”，请启用 pre-commit 钩子。

### Step 1：创建/编辑 `.git/hooks/pre-commit`

在仓库目录找到 `.git/hooks/`，新建文件 **`pre-commit`**（无后缀），内容如下：

````
#!/bin/sh
cd "$(git rev-parse --show-toplevel)" || exit 1
powershell -ExecutionPolicy Bypass -File scripts/format_check_changed.ps1
````

### Step 2：效果

之后你执行 `git commit` 时会自动运行检查：

* 如果有未格式化文件：提交失败，并提示你运行 `scripts/format_changed.ps1`
* 如果都已格式化：提交通过

> 说明：
> 你仍然可以在 Git Bash 里提交。钩子只是“自动调用 PowerShell 来检查”。

---

## 6. 常见问题（Troubleshooting）

### Q1：PowerShell 提示找不到脚本（-File 路径不存在）

原因：你可能不在仓库根目录运行相对路径脚本。
解决方式：

* 先 `cd` 到仓库根目录再执行
* 或使用绝对路径执行脚本

Git Bash 绝对路径示例：
Bash:
````
powershell -ExecutionPolicy Bypass -File /d/dev/cilly-vm-cpp/scripts/format_check_changed.ps1
````

### Q2：为什么检查脚本用 `-output-replacements-xml`？

Windows 上常见 CRLF/LF 换行差异会导致“字符串比较”误判。
本仓库采用 `clang-format -output-replacements-xml` 判断“是否会产生替换”，更稳定。

### Q3：我只想格式化一个文件怎么办？

你可以直接运行：
PowerShell:
````
clang-format -i --style=file path\to\file.cc
````

---

## 7. 推荐给贡献者的提交习惯

* 每次提交前跑一次：

  * `scripts/format_check_changed.ps1`
* 若未通过，立刻跑：

  * `scripts/format_changed.ps1`
* 如果你做了“大重构 / 全仓库范围的排版调整”，请先单独提交一次“纯格式化提交”，避免混淆功能改动。


