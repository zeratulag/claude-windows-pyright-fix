# claude-windows-pyright-fix

![Platform](https://img.shields.io/badge/platform-Windows-0078D6)
![Language](https://img.shields.io/badge/language-C%2B%2B17-00599C)
![License](https://img.shields.io/badge/license-MIT-2EA043)
![Type](https://img.shields.io/badge/type-shim-8A2BE2)

A tiny Windows shim that fixes Claude Code failing to launch `pyright-langserver`.

一个极小型的 Windows shim，用来修复 Claude Code 在 Windows 上无法正确启动 `pyright-langserver` 的问题。

---

## Chinese

### 简介

这个项目提供了一个同名的 `pyright-langserver.exe` 包装器。

当 Claude Code 在 Windows 上尝试启动 `pyright-langserver` 时，这个包装器会主动搜索真实的 `pyright-langserver.cmd`，再以绝对路径启动它，从而避免直接修改 Claude 自动生成的配置文件。

### 为什么会有这个项目

在社区 issue [anthropics/claude-code#16751](https://github.com/anthropics/claude-code/issues/16751) 中，问题被归因于 Windows 下 `pyright-langserver` 通常以 `.cmd` wrapper 的形式存在，而启动路径没有正确处理这种情况。

基于这个思路，一个常见的传统修复方式是直接修改 Claude 生成的 `marketplace.json`，把：

```json
"command": "pyright-langserver"
```

改成：

```json
"command": "pyright-langserver.cmd"
```

这个办法通常能暂时生效，但不够稳定，因为 `marketplace.json` 可能会被 Claude 后续自动覆盖。

这个项目的目标，是把修复点从“修改 Claude 生成文件”转成“提供一个稳定的 Windows 可执行入口”。

### 特性

- 不修改 Claude 生成的 `marketplace.json`
- 自动搜索真实的 `pyright-langserver.cmd`
- 使用绝对路径启动，避免路径解析绕回自身
- 透传命令行参数、`stdin`、`stdout`、`stderr` 和退出码
- 只有一个源文件，构建和部署都很轻量

### 工作方式

程序会按下面顺序查找真实的 `pyright-langserver.cmd`：

1. `pyright-langserver.exe` 所在目录
2. `PATH` 环境变量中的每个目录

如果找到，就通过 `cmd.exe` 启动真实的 `.cmd`，并把当前调用完整转发过去。

如果找不到，程序会输出错误信息并以非 0 退出。

### 构建前提

请确认：

- 运行环境是 Windows
- 已安装 LLVM，并且 `clang++` 在 `PATH` 中
- 可以使用 Windows SDK 头文件和库
- 当前目录可写，用于生成 `pyright-langserver.exe`

### 构建

直接执行：

```bat
build.bat
```

或手动编译：

```powershell
clang++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -DUNICODE -D_UNICODE -DWIN32_LEAN_AND_MEAN -DNOMINMAX src\main.cpp -o pyright-langserver.exe
```

### 部署前提

请确认：

- 已安装 Claude Code
- 已通过 `npm install -g pyright` 安装 Pyright
- 系统中确实存在真实的 `pyright-langserver.cmd`
- 你放置 `pyright-langserver.exe` 的位置会被 Claude 优先解析

### 部署

先确认真实的 npm shim 路径：

```powershell
where.exe pyright-langserver.cmd
```

然后把构建出的 `pyright-langserver.exe` 放到 Claude 会优先命中的位置。最直接的做法，是把它放到真实 `pyright-langserver.cmd` 的同目录。

部署后可以检查命中情况：

```powershell
where.exe pyright-langserver
where.exe pyright-langserver.cmd
```

理想结果是：

- `where.exe pyright-langserver` 先命中你的 wrapper exe
- `where.exe pyright-langserver.cmd` 仍指向真实的 npm shim

### 快速验证

```powershell
pyright-langserver.exe --stdio
```

如果 shim 工作正常，它应该进入真实的 pyright 进程，而不是报路径解析错误。

### 文件

- `src/main.cpp`: 主实现
- `build.bat`: 构建脚本
- `LICENSE`: MIT 许可证

---

## English

### Overview

This project provides a same-name `pyright-langserver.exe` wrapper.

When Claude Code tries to launch `pyright-langserver` on Windows, the wrapper searches for the real `pyright-langserver.cmd` and launches it by absolute path, avoiding direct edits to Claude-generated configuration files.

### Why This Exists

In community issue [anthropics/claude-code#16751](https://github.com/anthropics/claude-code/issues/16751), the problem is attributed to the fact that on Windows, `pyright-langserver` is commonly exposed through a `.cmd` wrapper, while the launch path does not correctly handle that case.

Based on that direction, a common manual workaround is to edit Claude's generated `marketplace.json` and change:

```json
"command": "pyright-langserver"
```

to:

```json
"command": "pyright-langserver.cmd"
```

That workaround can help temporarily, but it is fragile because `marketplace.json` may later be regenerated or overwritten by Claude.

This project moves the fix away from "patch Claude's generated files" and toward "provide a stable Windows executable entry point".

### Features

- avoids editing Claude-generated `marketplace.json`
- automatically finds the real `pyright-langserver.cmd`
- launches by absolute path to avoid recursive path resolution
- forwards command-line arguments, `stdin`, `stdout`, `stderr`, and exit code
- keeps the project tiny with a single source file

### How It Works

The shim looks for the real `pyright-langserver.cmd` in this order:

1. the directory containing `pyright-langserver.exe`
2. every directory listed in `PATH`

If found, it launches the real `.cmd` through `cmd.exe` and forwards the current invocation to it.

If not found, it prints an error and exits with a non-zero code.

### Build Prerequisites

Make sure:

- you are on Windows
- LLVM is installed and `clang++` is available in `PATH`
- Windows SDK headers and libraries are available
- the project directory is writable so `pyright-langserver.exe` can be created

### Build

Run:

```bat
build.bat
```

Or compile manually:

```powershell
clang++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -DUNICODE -D_UNICODE -DWIN32_LEAN_AND_MEAN -DNOMINMAX src\main.cpp -o pyright-langserver.exe
```

### Deploy Prerequisites

Make sure:

- Claude Code is installed
- Pyright is installed via `npm install -g pyright`
- the real `pyright-langserver.cmd` exists on the system
- the location where you place `pyright-langserver.exe` is resolved by Claude before the original command

### Deploy

First, locate the real npm shim:

```powershell
where.exe pyright-langserver.cmd
```

Then place the built `pyright-langserver.exe` somewhere Claude will resolve first. The simplest option is to put it in the same directory as the real `pyright-langserver.cmd`.

After deployment, verify resolution:

```powershell
where.exe pyright-langserver
where.exe pyright-langserver.cmd
```

Expected result:

- `where.exe pyright-langserver` resolves to your wrapper exe first
- `where.exe pyright-langserver.cmd` still resolves to the real npm shim

### Quick Check

```powershell
pyright-langserver.exe --stdio
```

If the shim is working correctly, it should reach the real pyright process instead of failing with a path-resolution error.

### Files

- `src/main.cpp`: main implementation
- `build.bat`: build script
- `LICENSE`: MIT license
