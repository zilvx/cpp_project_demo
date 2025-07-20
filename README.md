在 macOS 上基于 VSCode 搭建 C++ 开发环境
# 环境准备
1. 安装 Xcode 命令行工具（提供编译器）：
```bash
xcode-select --install
```

2. 安装 Homebrew（包管理器）：
```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

3. 通过 Homebrew 安装 GCC/Clang：
```bash
brew install gcc
```
 
4. 通过 Homebrew 安装 cmake：
```bash
brew install cmake
```
 
5. 安装VSCode 并下载插件
 - C/C++ (Microsoft)
 - CMake Tools (Microsoft)
 - Code Runner
 - Clang-Format

# 工程代码
使用CMake作为构建工具，而不是直接调用g++/clang++。
代码如仓库中所示。
直接点击F5进行调试即可（自动出发cmake  --build .）


