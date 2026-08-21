# Qt Demo Project

基于 Qt6 + CMake + Conan 的 C++ 桌面应用，包含表格数据管理、波形图表、HTTP 前后端通信等功能。

## 快速开始

```bash
# 1. 安装依赖
conan install . --output-folder=. --build=missing -s build_type=Release

# 2. 构建
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# 3. 运行
open build/qt_table_app.app               # Qt 桌面应用
./build/table_data_server                 # HTTP 后端服务（端口 8080）
./build/main                              # 控制台测试程序
```

> 环境准备（编译器、CMake、Conan、Qt）详见 [环境搭建](#环境搭建)。

---

## 项目结构

```
.
├── CMakeLists.txt
├── conanfile.txt                    # Conan 依赖声明
├── main.cpp                         # 控制台测试入口
│
├── src/
│   ├── bs/                          # 业务后端
│   │   ├── main_bs.cpp              #   服务入口
│   │   ├── TableDataServer.h        #   HTTP 服务器 (cpp-httplib)
│   │   └── TableDataServer.cpp
│   │
│   └── ui/                          # Qt 前端
│       ├── main_ui.cpp              #   应用入口
│       ├── resources.qrc
│       ├── providers/               #   数据提供者
│       │   ├── IDataProvider.h/.cpp         # 抽象接口
│       │   ├── SampleDataProvider.h/.cpp    # 本地样本
│       │   ├── HttpDataProvider.h/.cpp      # HTTP JSON
│       │   └── HttpProtoDataProvider.h/.cpp # HTTP Protobuf
│       ├── core/                    #   逻辑组件
│       │   ├── TableData.h          #     表格数据结构
│       │   ├── EditController.h/.cpp
│       │   ├── ScrollBar.h/.cpp
│       │   ├── PenIconDelegate.h/.cpp
│       │   └── WaveformData.h/.cpp
│       ├── widgets/                 #   视图组件
│       │   ├── TableWidget.h/.cpp
│       │   ├── VirtualKeyboard.h/.cpp
│       │   └── WaveformChart.h/.cpp
│       └── styles/                  #   QSS 样式
│           ├── table.qss
│           ├── keyboard_style.qss
│           └── scrollbar_style.qss
│
├── src/utils/
│   ├── logging/                     # 日志封装 (spdlog)
│   └── proto/                       # Protobuf schema
│
├── tests/                           # 单元测试 (GoogleTest)
│   ├── test_nice_step.cpp
│   ├── test_waveform_data.cpp
│   ├── test_table_data.cpp
│   └── test_edit_controller.cpp
│
├── examples/                        # 算法练习
│   ├── random/
│   └── leetcode/
│
├── data/arbitrary_wave.csv
└── stub_frameworks/                 # macOS AGL 兼容
```

---

## 构建目标

| 目标 | 说明 | 运行方式 |
|------|------|----------|
| `qt_table_app` | Qt 桌面应用（表格 + 键盘 + 波形） | `open build/qt_table_app.app` |
| `table_data_server` | HTTP 后端服务 | `./build/table_data_server [端口]` |
| `main` | 控制台测试程序 | `./build/main` |
| `table_tests` | 单元测试（28 用例） | `./build/tests/table_tests` |

---

## 功能架构

### 数据源切换（条件编译）

通过 CMake option 在编译期选择 TableWidget 的数据来源，互斥三选一：

```bash
cmake -S . -B build                                    # 默认：本地样本数据
cmake -S . -B build -DUSE_HTTP_DATA=ON                 # HTTP JSON 模式
cmake -S . -B build -DUSE_PROTO_DATA=ON                # HTTP Protobuf 模式
```

```
IDataProvider (抽象接口)
    ├── SampleDataProvider       ← 本地硬编码（默认）
    ├── HttpDataProvider         ← GET /api/table → JSON
    └── HttpProtoDataProvider    ← GET /api/table/proto → Protobuf
```

所有 Provider 通过 `fetchData()` 触发获取，完成后发射 `dataReady(TableData)` 信号 → `TableWidget::loadData()` 渲染。

### HTTP 后端服务

基于 [cpp-httplib](https://github.com/yhirose/cpp-httplib)，自动处理 HTTP 协议细节。

| 端点 | 格式 | 说明 |
|------|------|------|
| `GET /api/table` | JSON | 表格数据 |
| `GET /api/table/proto` | Protobuf | 表格数据（需 `USE_PROTO_DATA=ON`） |
| `GET /health` | JSON | 健康检查 |

### 其他组件

| 组件 | 说明 |
|------|------|
| **WaveformChart** | 示波器风格波形图，支持正弦/方波/锯齿波/CSV，Catmull-Rom 平滑曲线，可拖动标记 |
| **VirtualKeyboard** | 98 键机械键盘风格虚拟键盘，接管表格编辑 |
| **EditController** | 表格编辑状态机，支持字符追加/退格/确认/取消/Tab 跳转 |
| **ScrollBarStyler** | 深色主题滚动条，QSS 驱动，通用接口 |
| **PenIconDelegate** | 可编辑单元格右下角钢笔图标 ✎ |

---

## 单元测试

28 个测试用例，5 个套件，GoogleTest 框架，可通过 `ctest` 运行：

```bash
cmake --build build --target table_tests
./build/tests/table_tests              # 或: cd build && ctest
```

| 套件 | 覆盖 |
|------|------|
| `NiceStepTest` (4) | 刻度间距算法 |
| `WaveformDataTest` (6) | 波形生成 |
| `WaveformChartTest` (4) | 坐标转换、插值 |
| `TableDataTest` (6) | 数据结构、Protobuf 往返 |
| `EditControllerTest` (8) | 编辑状态机全分支 |

---

## 依赖管理

```
Conan (conanfile.txt)
├── spdlog/1.13.0           # 日志
├── protobuf/3.21.12        # 序列化（含 protoc + abseil）
├── gtest/1.14.0            # 测试框架
└── cpp-httplib/0.47.0      # HTTP 库

Homebrew
└── qt                      # Qt6 (Widgets + Network)
```

源码中的问题排查记录和架构优化记录已移至 [`docs/`](docs/) 目录。

---

## 环境搭建

<details>
<summary>展开查看完整环境搭建步骤</summary>

### 1. 安装 Xcode 命令行工具
```bash
xcode-select --install
```

### 2. 安装 Homebrew
```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

### 3. 安装 CMake
```bash
brew install cmake
```

### 4. 安装 Conan
```bash
brew install conan
```

### 5. 安装 Qt6
```bash
brew install qt
```

### 6. 安装 VSCode 插件
- C/C++ (Microsoft)
- CMake Tools (Microsoft)

</details>

## VSCode 配置

- **F5 调试**: 自动触发 CMake 构建
- **IntelliSense**: `build/compile_commands.json` 自动生成（需 `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON`）
- **Code Runner**: 通过 CMake 构建后运行，配置见 `.vscode/settings.json`

## 打包发布

使用 `packaging/build_package.py` 一键构建并打包当前平台的发布产物（自动执行
`conan install` → CMake 构建 → 依赖部署 → 生成分发文件）：

```bash
python3 packaging/build_package.py                      # 全量打包
python3 packaging/build_package.py -t qt                # 仅 Qt 桌面应用
python3 packaging/build_package.py -t server            # 仅后端服务
python3 packaging/build_package.py --data-source http   # HTTP JSON 数据源构建
python3 packaging/build_package.py --skip-build         # 只打包已有构建产物
```

各平台产物（输出到 `build/dist/<平台>/`，已被 gitignore 忽略）：

| 平台 | 产物 | 说明 |
|------|------|------|
| macOS | `QtDemo-<版本>-macos.dmg` | 两个 `.app` 已内置 Qt 框架并 ad-hoc 签名，可独立运行 |
| Windows | `QtDemo-<版本>-windows-x86_64.zip` | `windeployqt` 收集 Qt DLL/插件 |
| Linux | `QtDemo-<版本>-linux-x86_64.tar.gz` | 手动收集动态库 + `patchelf` 改写 rpath |

> 注意：Windows 需在 MSVC 开发者环境中运行，并用 `--qt-dir` 指定 Qt 安装目录；
> Linux 依赖 `patchelf`；脚本只能在当前操作系统上打包对应平台产物。

### 打包范围

| 目标 | 内容 |
|------|------|
| `all`（默认） | 桌面应用 + 后端服务 + 控制台程序 |
| `qt` | `qt_table_app`、`qt_table_app_fork`（含内嵌子进程 `fetch_table_child`） |
| `server` | `table_data_server`、`table_data_server_prefork` |
| `console` | `main` |

## 更多文档

- [问题排查记录](docs/troubleshooting.md) — 14 个历史问题及解决方案
- [架构优化记录](docs/optimization.md) — 20 项代码质量优化详情
