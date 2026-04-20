# claude-windows-pyright-fix

![Platform](https://img.shields.io/badge/platform-Windows-0078D6)
![Language](https://img.shields.io/badge/language-C%2B%2B17-00599C)
![License](https://img.shields.io/badge/license-MIT-2EA043)
![Purpose](https://img.shields.io/badge/purpose-Claude%20Code%20Pyright%20fix-8A2BE2)

[简体中文](#简体中文) | [English](#english)

---

## 简体中文

### 项目简介

这是一个极小型的 Windows shim，用来修复 Claude Code 在 Windows 上启动 `pyright-langserver` 失败的问题。

它不依赖修改 Claude 自动生成的配置文件，而是提供一个 `pyright-langserver.exe` 包装器，由这个包装器主动搜索真实的 `pyright-langserver.cmd`，再以绝对路径启动它。

### 为什么会有这个项目

在社区 issue [anthropics/claude-code#16751](https://github.com/anthropics/claude-code/issues/16751) 中，问题被归因于 Windows 下 `pyright-langserver` 通常实际以 `.cmd` wrapper 的形式存在，而启动路径没有正确处理这种情况。

基于这个思路，一个传统的手工修复方式是直接修改 Claude 生成的 `marketplace.json`，把：

```json
"command": "pyright-langserver"
```

改成：

```json
"command": "pyright-langserver.cmd"
```

这个办法通常能暂时生效，但缺点也很明显：`marketplace.json` 可能会被 Claude 后续自动覆盖，所以修复不稳定。

这个项目的目标，就是把修复点从“改 Claude 的生成物”改成“补一个稳定的 Windows 可执行入口”。

### 工作方式

程序会按下面顺序查找真实的 `pyright-langserver.cmd`：

1. `pyright-langserver.exe` 所在目录
2. `PATH` 环境变量中的每个目录

找到后会透传：

- 命令行参数
- 标准输入
- 标准输出
- 标准错误
- 退出码

如果找不到 `pyright-langserver.cmd`，程序会输出错误信息并以非 0 退出。

### 构建前提

构建前请确认：

- 运行环境是 Windows
- 已安装 LLVM，并且 `clang++` 在 `PATH` 中
- 可以使用 Windows SDK 头文件和库
- 当前目录可写，用于输出 `pyright-langserver.exe`

### 构建

直接执行：

```bat
build.bat
```

如果你想手动编译，也可以执行：

```powershell
clang++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -DUNICODE -D_UNICODE -DWIN32_LEAN_AND_MEAN -DNOMINMAX src\main.cpp -o pyright-langserver.exe
```

### 部署前提

部署前请确认：

- 已安装 Claude Code
- 已通过 `npm install -g pyright` 安装 Pyright
- 系统里确实存在真实的 `pyright-langserver.cmd`
- 你准备放置 `pyright-langserver.exe` 的位置会被 Claude 优先解析

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

- `src/main.cpp`：主实现
- `build.bat`：构建脚本
- `LICENSE`：MIT 许可证

---

## English

### Overview

This is a tiny Windows shim that fixes Claude Code failing to launch `pyright-langserver` on Windows.

Instead of editing Claude-generated configuration, this project provides a `pyright-langserver.exe` wrapper. The wrapper searches for the real `pyright-langserver.cmd` and launches it by absolute path.

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

That workaround can help temporarily, but it is fragile because `marketplace.json` may be regenerated or overwritten by Claude later.

This project exists to move the fix away from "patch Claude's generated files" and toward "provide a stable Windows executable entry point".

### How It Works

The shim looks for the real `pyright-langserver.cmd` in this order:

1. The directory containing `pyright-langserver.exe`
2. Every directory listed in `PATH`

When found, the shim forwards:

- command-line arguments
- stdin
- stdout
- stderr
- exit code

If `pyright-langserver.cmd` cannot be found, the shim prints an error and exits with a non-zero code.

### Build Prerequisites

Before building, make sure:

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

Before deploying, make sure:

- Claude Code is installed
- Pyright is installed via `npm install -g pyright`
- the real `pyright-langserver.cmd` actually exists on the system
- the location where you place `pyright-langserver.exe` will be resolved by Claude before the original command

### Deploy

First, locate the real npm shim:

```powershell
where.exe pyright-langserver.cmd
```

Then place the built `pyright-langserver.exe` somewhere Claude will resolve first. The simplest option is to put it in the same directory as the real `pyright-langserver.cmd`.

After deployment, verify command resolution:

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
