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

5. 通过 Homebrew 安装 Conan（C++ 包管理器）：
```bash
brew install conan
```

6. 通过 Homebrew 安装 Qt6：
```bash
brew install qt
brew install qt-creator  # 可选: Qt IDE
```
 
7. 安装VSCode 并下载插件
 - C/C++ (Microsoft)
 - CMake Tools (Microsoft)
 - Code Runner
 - Clang-Format

# 工程代码
使用CMake作为构建工具，而不是直接调用g++/clang++。
代码如仓库中所示。
直接点击F5进行调试即可（自动触发cmake  --build .）


# 第三方依赖

## spdlog (日志库)

通过 Conan 管理，配置文件 `conanfile.txt`:

```
[requires]
spdlog/1.13.0

[generators]
CMakeDeps
CMakeToolchain

[layout]
cmake_layout
```

### 首次构建步骤

```bash
# 1. 安装 Conan 依赖并生成 CMake 工具链
conan install . --output-folder=. --build=missing -s build_type=Release

# 2. 配置 CMake（会自动 include conan_toolchain.cmake）
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

# 3. 构建
cmake --build build
```

> **注意**: `--output-folder=.` 而非 `--output-folder=build`。因为 `cmake_layout` 会在 output-folder 下自动创建 `build/Release/generators/` 结构，若传入 `build` 会导致路径重复为 `build/build/Release/generators/`，与 CMakeLists.txt 中的路径不匹配。

### Conan toolchain 路径问题

CMakeLists.txt 中通过以下方式引入 Conan 生成的 toolchain：

```cmake
include(${CMAKE_BINARY_DIR}/Release/generators/conan_toolchain.cmake)
```

`CMAKE_BINARY_DIR` = `build`，因此期望文件位于 `build/Release/generators/conan_toolchain.cmake`。Conan 命令必须使用 `--output-folder=.` 才能将文件生成到正确位置。


## Qt6 (GUI 框架)

通过 Homebrew 安装，版本 6.9.1，安装路径 `/opt/homebrew/opt/qt`。

### CMake 集成

```cmake
# 确保 CMake 能找到 Homebrew 安装的 Qt6
list(PREPEND CMAKE_PREFIX_PATH "/opt/homebrew/opt/qt")
find_package(Qt6 REQUIRED COMPONENTS Widgets)

# AUTOMOC/AUTORCC/AUTOUIC 只对 Qt 目标启用
set_target_properties(qt_table_app PROPERTIES
    AUTOMOC ON
    AUTORCC ON
    AUTOUIC ON
)
```

> **注意**: 不要把 `set(CMAKE_AUTOMOC ON)` 设为全局变量，否则非 Qt 目标也会触发 MOC，产生大量 `mocs_compilation.cpp.o has no symbols` 警告。

### QSS 样式管理

样式表独立存放在 `.qss` 文件中，通过 Qt Resource System (`.qrc`) 编译嵌入可执行文件：

```
src/qt_widget/
├── style.qss          # 样式表文件（深色主题）
├── resources.qrc      # 资源索引文件
├── TableWidget.h/cpp  # 组件代码
└── main_qt.cpp        # 应用入口
```

`resources.qrc` 内容：
```xml
<RCC>
    <qresource prefix="/">
        <file alias="style.qss">style.qss</file>
    </qresource>
</RCC>
```

C++ 中加载：
```cpp
QFile styleFile(":/style.qss");
if (styleFile.open(QFile::ReadOnly | QFile::Text)) {
    m_table->setStyleSheet(QString::fromUtf8(styleFile.readAll()));
}
```

修改样式时只需编辑 `style.qss`，无需改动 C++ 代码。


# 问题排查记录

## 1. Code Runner 报错 `'spdlog/spdlog.h' file not found`

**现象**: 在 VSCode 中点击 Run Code ▶ 按钮，终端报错找不到 spdlog 头文件。

**原因**: Code Runner 直接用 `clang++` 编译单个 `.cpp` 文件，不走 CMake，无法获取 Conan 提供的 include 路径。

**解决**: 配置 Code Runner 通过 CMake 构建后运行：

```json
// .vscode/settings.json
"code-runner.executorMap": {
    "cpp": "cd $workspaceRoot && cmake --build build && clear && $workspaceRoot/build/main"
}
```

同时配置 `launch.json` 和 `tasks.json`，使用 F5/Ctrl+F5 进行 CMake 构建 + 调试运行。


## 2. Code Runner 报错 `Error: could not load cache`

**现象**: 点击 Run Code 后报 `Error: could not load cache`。

**原因**: `build/` 目录为空（CMake 缓存丢失），或之前 Conan 使用了错误的 `--output-folder` 导致 toolchain 路径不匹配。

**解决**: 
```bash
# 清理后重新执行
rm -rf build/*
conan install . --output-folder=. --build=missing -s build_type=Release
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```


## 3. SumOfTwoNum 导致程序崩溃，日志丢失

**现象**: `main.cpp` 中调用 `SumOfTwoNum::twoSum()` 后，后续的 `spdlog::info()` 不会输出，日志文件在该处截断。

**原因**: `SumOfTwoNum.cpp` 的 `twoSum()` 函数声明返回 `std::vector<int>`，但函数体结束时没有 `return` 语句，触发**未定义行为**。

编译警告已提示：
```
SumOfTwoNum.cpp:18:1: warning: non-void function does not return a value [-Wreturn-type]
```

**解决**: 在函数中添加 `return result;`:
```cpp
std::vector<int> SumOfTwoNum::twoSum(const std::vector<int>& nums, int target) {
    std::vector<int> result;
    for (...) {
        for (...) {
            if (...) {
                result.emplace_back(i);
                result.emplace_back(j);
                return result;  // 找到后立即返回
            }
        }
    }
    return result;  // 未找到时返回空 vector
}
```


## 4. Qt6 链接失败: `ld: framework 'AGL' not found`

**现象**: Qt6 应用在链接阶段报错 `ld: framework 'AGL' not found`。

**原因**: macOS 26 (15.x SDK) 中完全移除了 AGL (Apple Graphics Library) 框架。但 Qt6 的 `FindWrapOpenGL.cmake` 仍然硬编码了 `-framework AGL`，即使 Qt6 实际上不调用任何 AGL 符号。

Qt 的查找逻辑 (`/opt/homebrew/opt/qt/lib/cmake/Qt6/FindWrapOpenGL.cmake`):
```cmake
find_library(WrapOpenGL_AGL NAMES AGL)
if(WrapOpenGL_AGL)
    set(__opengl_agl_fw_path "${WrapOpenGL_AGL}")   # 找到了但没有二进制
endif()
if(NOT __opengl_agl_fw_path)
    set(__opengl_agl_fw_path "-framework AGL")        # 兜底也会加 -framework AGL
endif()
target_link_libraries(WrapOpenGL::WrapOpenGL INTERFACE ${__opengl_agl_fw_path})
```

无论 `find_library` 是否找到 AGL，最终都会产生 `-framework AGL` 链接标志。

**解决**: 创建 stub AGL 框架，提供所有 AGL 符号的空实现，满足链接器要求：

```
stub_frameworks/
└── AGL.framework/
    └── Versions/A/AGL    # 桩动态库（导出全部 AGL 符号但无实际操作）
```

CMake 配置：
```cmake
# 链接时使用 stub 框架替代
target_link_options(qt_table_app PRIVATE
    -F${CMAKE_SOURCE_DIR}/stub_frameworks
    -Wl,-rpath,${CMAKE_SOURCE_DIR}/stub_frameworks  # 运行时也能找到
)
```

`-F` 指定框架搜索路径确保链接通过；`-rpath` 确保运行时 dyld 能找到 stub 框架。


## 5. Qt 应用启动崩溃: `Library not loaded: @rpath/AGL.framework/Versions/A/AGL`

**现象**: 双击或 `open` 启动 `.app` 时弹出"无法打开"对话框，终端运行显示：
```
dyld: Library not loaded: @rpath/AGL.framework/Versions/A/AGL
```

**原因**: 链接时 stub AGL 的 `install_name` 设为 `@rpath/AGL.framework/Versions/A/AGL`，但可执行文件的 rpath 中未包含 `stub_frameworks` 目录。

**解决**: 在 CMake 中追加 rpath：
```cmake
target_link_options(qt_table_app PRIVATE
    -F${CMAKE_SOURCE_DIR}/stub_frameworks
    -Wl,-rpath,${CMAKE_SOURCE_DIR}/stub_frameworks  # ← 关键
)
```

验证据 `otool -l`:
```
cmd LC_RPATH
path /Users/shenxing/Desktop/Projects/cpp_project_demo/stub_frameworks
```


## 6. QTableWidget 行号错乱 + 数据丢失

**现象**: 表格显示时行号顺序变为 第4→第3→第2→第1→第5 行，部分数据丢失。

**原因**: `setSortingEnabled(true)` 在 `setupUI()` 中（数据填充**之前**）就被调用。每调用一次 `setItem()`，排序机制就触发一次重排，导致行号与数据行的映射关系错乱。

```cpp
// ❌ 错误做法
m_table->setSortingEnabled(true);   // 先启用排序
populateSampleData();               // 每次 setItem 都触发重排

// ✅ 正确做法
populateSampleData();               // 先安静地填充数据
m_table->setSortingEnabled(true);   // 数据填充完毕后再启用排序
```

**结论**: Qt 中任何可排序的 Item View，都应在**所有数据填充完毕后**再启用排序。


## 7. QTableWidget 左上角（行列表头交叉处）显示白色

**现象**: 深色主题表格的左上角（行号与列名交叉处）出现白色背景，与深色表头不协调。

**原因**: QTableWidget 左上角是一个 `QTableCornerButton`，默认使用系统原生样式（亮色），不会自动继承 HeaderView 的 QSS。

**解决**: 在 `style.qss` 中单独设置 CornerButton 样式：

```css
QTableCornerButton::section {
    background-color: #181c34;   /* 与表头颜色一致 */
    border: none;
    border-bottom: 1px solid rgba(255, 255, 255, 0.05);
    border-right:  1px solid rgba(255, 255, 255, 0.05);
}
```


## 8. 深色主题滚动条封装 — ScrollBar 组件解耦

**需求**: 为表格添加深色滚动条，且滚动条作为独立组件与 TableWidget 解耦。

**实现**: 创建 `ScrollBarStyler` 工具类，通过 QSS 控制 `QScrollBar` 的每个子控件：

```
src/qt_widget/
├── ScrollBar.h      ← 接口：ScrollBarStyler::applyTo(QAbstractScrollArea *)
├── ScrollBar.cpp    ← 垂直/水平滚动条全套 QSS
└── TableWidget.cpp  ← 一行调用：ScrollBarStyler::applyTo(m_table)
```

**设计要点**:

| 特性 | 说明 |
|---|---|
| 通用接口 | `applyTo(QAbstractScrollArea*)` 适用于 QTableWidget、QTreeWidget 等任何滚动控件 |
| 样式封装 | 滚动条 QSS 由 ScrollBar 内部管理，外部不可见 |
| 垂直/水平 | 分别处理 `QScrollBar:vertical` 与 `QScrollBar:horizontal` |
| 滑块反馈 | normal / hover / pressed 三种半透明度状态 |
| 箭头隐藏 | `QScrollBar::up-arrow / down-arrow` 尺寸设 0，呈现无箭头简约风格 |

**关键 QSS 结构**:

```css
QScrollBar:vertical     { width: 8px;  background: #323649; }   /* 滑道 */
QScrollBar::handle:vertical { background: rgba(255,255,255,0.15); border-radius: 4px; } /* 滑块 */
QScrollBar::handle:vertical:hover  { background: rgba(255,255,255,0.25); }
QScrollBar::handle:vertical:pressed { background: rgba(255,255,255,0.35); }
QScrollBar::add-line:vertical, .sub-line:vertical { height: 0; }  /* 隐藏箭头区域 */
```


## 9. 钢笔图标位置优化 — QStyledItemDelegate

**需求**: 可编辑单元格右下角显示钢笔图标 ✎ 提示可编辑；部门列不可编辑，无图标。

**第一版问题**: 使用 `QTableWidgetItem::setIcon()` (DecorationRole) 只能将图标放在文字**左侧**，不满足"右下角"的需求。

**解决方案**: 创建 `PenIconDelegate`，继承 `QStyledItemDelegate`，重写 `paint()` 在右下角绘制：

```
src/qt_widget/
├── PenIconDelegate.h    ← QStyledItemDelegate 子类声明
├── PenIconDelegate.cpp  ← 绘制逻辑
└── TableWidget.cpp      ← m_table->setItemDelegate(new PenIconDelegate(m_table))
```

**实现原理**:

```cpp
void PenIconDelegate::paint(QPainter *painter, ...) {
    // 1. 先绘制默认样式（文字、背景、选中高亮）
    QStyledItemDelegate::paint(painter, option, index);

    // 2. 仅对有 Qt::ItemIsEditable 标志的单元格绘制钢笔图标
    if (!(index.flags() & Qt::ItemIsEditable)) return;

    // 3. 定位到右下角（4px 边距）
    const QPoint pt(cellRect.right() - iconW - 4,
                    cellRect.bottom() - iconH - 4);
    painter->drawPixmap(QRect(pt, m_penPixmap.size()), m_penPixmap);
}
```

**部门列只读**: 在 `populateSampleData()` 中对 `col == 2` 的 item 移除 `Qt::ItemIsEditable` 标志，delegate 检测不到该标志时跳过图标绘制，且双击不触发编辑。

```cpp
if (col == kDeptCol) {
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);   // 只读
} else {
    item->setFlags(item->flags() | Qt::ItemIsEditable);    // 可编辑
}
```


## 10. 98键虚拟键盘组件

**需求**: 封装一个 98 键（1800 紧凑布局）机械键盘风格的虚拟键盘组件。

**实现**:

```
src/qt_widget/
├── VirtualKeyboard.h      ← QWidget 子类，信号 keyPressed(QString)
├── VirtualKeyboard.cpp    ← 数据驱动 6 行键位布局
└── keyboard_style.qss     ← 键帽样式（深色立体感）
```

**设计要点**:

| 特性 | 说明 |
|---|---|
| 数据驱动 | `kLayout` 6 行键位定义：`{标签, 宽度单位, 键值}` |
| 键宽比例 | 1u=48px, Tab 1.5u, Caps 1.75u, Shift 2.25u, Space 6.25u |
| 信号设计 | `keyPressed(QString)` — 字母键发单字符，功能键发名称 (`"Backspace"`, `"Enter"` 等) |
| 样式分层 | `keyClass="alpha"` 普通键 / `"special"` 功能键 / `"space"` 空格键 |
| 解耦 | 独立组件，无外部依赖，可嵌入任何 QWidget |

**键位布局定义**（数据驱动，便于修改）:

```cpp
static const QVector<QVector<KeyDef>> kLayout = {
    // Row 0: F-row
    {{"Esc",1}, {"F1",1}, {"F2",1}, ..., {"Del",1,"Delete"}, {"PrtSc",1,"Print"}},
    // Row 1: Number row
    {{"`",1}, {"1",1}, ..., {"0",1}, {"-",1}, {"=",1}, {"←",2,"Backspace"}},
    // ... 6 rows total
};
```

**键帽立体感 QSS**:

```css
QPushButton {
    background-color: #2c3042;
    border-bottom: 3px solid #1f2233;   /* 底部阴影 → 立体感 */
    border-radius: 6px;
}
QPushButton:pressed {
    border-bottom: 1px solid #1a1d2e;   /* 按下时阴影缩回 */
    padding-top: 4px;
    padding-bottom: 0px;
}
```


## 11. 虚拟键盘接管表格编辑

**需求**: 点击表格可编辑单元格时弹出虚拟键盘进行编辑，代替系统默认的行内编辑器。

**问题**: QTableWidget 默认使用 `QLineEdit` 作为行内编辑控件（`DoubleClicked` 触发），无法替换为自定义键盘。

**解决方案**: 禁用默认编辑器 + 信号/槽联动虚拟键盘：

```
   TableWidget                          VirtualKeyboard
     ┌──────────────────┐               ┌──────────────┐
     │ cellClicked ──────→ onCellClicked │              │
     │                   │   ↓          │              │
     │ cellEditingStarted → setVisible(true) ───→ 显示  │
     │                   │              │              │
     │ handleKeyInput ←── keyPressed ←─── 按键点击    │
     │   ↓               │              │              │
     │ item->setText()   │              │              │
     │                   │              │              │
     │ commitEdit()      │              │              │
     │ cellEditingFinished → setVisible(false) ─→ 隐藏 │
     └──────────────────┘               └──────────────┘
```

**关键步骤**:

1. **禁用默认行内编辑器**:
```cpp
m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
```

2. **单元格点击 → 开始编辑**:
```cpp
connect(m_table, &QTableWidget::cellClicked, this, &TableWidget::onCellClicked);

void TableWidget::onCellClicked(int row, int col) {
    // 检查 Qt::ItemIsEditable 标志，忽略只读列
    if (!(item->flags() & Qt::ItemIsEditable)) return;
    // 保存编辑位置和原始文本
    m_editingRow = row; m_editingCol = col;
    m_originalText = item->text();
    emit cellEditingStarted(row, col);
}
```

3. **按键处理逻辑**:

| 按键 | 行为 |
|---|---|
| 单字符 (`A-Z`, `0-9`, 符号) | 追加到单元格文本末尾 |
| `Backspace` | 删除最后一个字符 |
| `Space` | 追加空格 |
| `Enter` / `Return` | 确认编辑，隐藏键盘 |
| `Escape` | 取消编辑，恢复原始文本 |
| `Tab` | 确认当前编辑，跳转到下一可编辑列 |
| `Shift` / `Ctrl` / `Alt` / `Fn` | 静默忽略 |

4. **输入保护**: 如果用户清空了单元格内容，`commitEdit()` 自动恢复原始值：
```cpp
if (item && item->text().isEmpty()) {
    item->setText(m_originalText);
}
```


## 12. 示波器风格波形图表组件

**需求**: 设计一个直方图组件，具备精细刻度网格，用于呈现正弦波、方波、锯齿波以及 CSV 导入的任意波形。

**实现**:

```
src/qt_widget/
├── core/
│   └── WaveformData.h/cpp       ← 波形数据模型（CSV 加载 + 数学波形生成）
└── widgets/
    └── WaveformChart.h/cpp      ← 示波器风格绘制（网格 + 平滑曲线 + 坐标轴）
```

**WaveformData — 数据层**:

| 数据源 | 方法 | 说明 |
|---|---|---|
| CSV 文件 | `loadCSV(path)` | 两列格式 `时间,幅值`，支持 `#` 注释行 |
| 正弦波 | `generateSine(freq, amp, sr, cycles)` | `y = A·sin(2πft)` |
| 方波 | `generateSquare(freq, amp, sr, cycles, riseRatio)` | 含渐进上升/下降沿，`riseRatio=0.02` |
| 锯齿波 | `generateSawtooth(freq, amp, sr, cycles)` | 线性上升，周期复位 |

**WaveformChart — 绘制层**:

| 特性 | 实现 |
|---|---|
| 背景 | 深色示波器风格 (#1a1d2e) |
| 网格 | Nice Numbers 算法自动计算最佳主/次刻度间距，多级透明度 |
| 曲线 | Catmull-Rom 样条 → Cubic Bezier 转换，张力 0.5，精确通过所有数据点 |
| 坐标轴 | 自动刻度标签 + 轴标题 |
| 颜色 | 青色发光波形线 (#00D2D2)，半透明白色网格 |

**平滑曲线算法** — Catmull-Rom → Bezier:

给定 4 个连续锚点 P0, P1, P2, P3，P1→P2 段的 Bezier 控制点为:

```
CP1 = P1 + (P2 - P0) × tension / 3
CP2 = P2 - (P3 - P1) × tension / 3
```

端点使用反射虚拟控制点保证首尾段也有平滑曲率。

**波形生成示例**:

```cpp
WaveformData data;
data.generateSine(1.0, 2.5, 500.0, 3);     // 1Hz, 2.5V, 500采样点/秒, 3个周期
data.generateSquare(2.0, 1.5, 500.0, 2);    // 2Hz 方波
data.loadCSV("data/arbitrary_wave.csv");     // Excel 导出的自定义波形
```


## 13. 波形标记器 — 滑动三角形 + 实时坐标

**需求**: 在波形曲线上增加一个可鼠标拖动的三角形标记，滑动过程中右侧始终显示当前坐标值。

**实现**:

| 特性 | 说明 |
|---|---|
| 交互 | 点击图表区域 → 三角形出现 → 拖动鼠标沿曲线滑动 → 释放后标记保持 |
| X 定位 | 鼠标 X 像素 → `pixelToData()` → 数据空间 X 值 |
| Y 插值 | 给定 X，二分查找最近两个数据点 → 线性插值得到精确 Y |
| 三角形 | 金色 (#FFB41E)，发光阴影，顶点精确指向曲线上的数据位置 |
| 坐标标签 | 深色圆角背景 + 金色边框 + 等宽字体，格式 `(2.514, 1.237)` |
| 约束 | X 限制在数据范围内，Y 跟随曲线 |

**关键方法**:

```cpp
// 像素 → 数据坐标（与 dataToPixel 互逆）
QPointF pixelToData(const QPointF &pixel, const QRect &chartRect) const;

// 二分查找 + 线性插值
double interpolateY(double dataX) const;

// 绘制三角形 + 坐标标签
void drawMarker(QPainter &p, const QRect &chartRect);
```


## 14. 目录结构重组 — widgets / core / styles

**需求**: `src/qt_widget/` 下 14 个文件平铺，缺乏层次感。按文件类型分为三层。

**最终结构**:

```
src/qt_widget/
├── main_qt.cpp                # 应用入口（QTabWidget: 表格 + 波形）
├── resources.qrc              # Qt 资源索引
│
├── widgets/                   # 视觉组件 (QWidget 子类)
│   ├── TableWidget.h/cpp      #   人员信息表
│   ├── VirtualKeyboard.h/cpp  #   98 键虚拟键盘
│   └── WaveformChart.h/cpp    #   波形示波器
│
├── core/                      # 工具/逻辑组件 (无界面)
│   ├── EditController.h/cpp   #   编辑状态机
│   ├── ScrollBar.h/cpp        #   滚动条样式
│   ├── PenIconDelegate.h/cpp  #   钢笔图标委托
│   └── WaveformData.h/cpp     #   波形数据模型
│
└── styles/                    # QSS 样式表
    ├── table.qss              #   表格深色主题
    ├── keyboard_style.qss     #   键帽立体感样式
    └── scrollbar_style.qss    #   滚动条深色风格
```

**分层规则**:
- `widgets/` — 有视觉界面的 QWidget 子类，可直接实例化使用
- `core/` — 无界面的工具/逻辑类，可被多个 widget 复用
- `styles/` — 纯 QSS 文件，UI 调整时只需改此目录

**同步修改**: CMakeLists.txt 源文件路径、resources.qrc 引用路径、所有 `#include` 路径。


# 代码架构优化

对项目进行系统性代码审查后，按优先级分四类进行了优化。以下记录每项优化的**动机（为什么）**与**方案（怎么做）**。


## P0 — 构建系统硬编码（阻塞性问题）

### 1. 硬编码 `CMAKE_BUILD_TYPE`

**为什么优化**: `set(CMAKE_BUILD_TYPE Release)` 强制覆盖用户传入的值，`cmake -DCMAKE_BUILD_TYPE=Debug` 完全无效，永远无法调试。

**如何优化**: 改为条件默认值，仅在用户未指定时设为 Release：

```cmake
# ❌ 之前
set(CMAKE_BUILD_TYPE Release)

# ✅ 之后
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()
```

### 2. 硬编码 Conan Toolchain 路径

**为什么优化**: `include(${CMAKE_BINARY_DIR}/Release/generators/conan_toolchain.cmake)` 中硬编码了 `Release`，导致 Debug 配置时找不到 toolchain 文件。

Conan 的 `cmake_layout` 将 generators 输出到 `<output>/<build_type>/generators/`，路径中的构建类型必须动态跟随 `CMAKE_BUILD_TYPE`。

**如何优化**: 路径中的 `Release` 替换为 `${CMAKE_BUILD_TYPE}`：

```cmake
# ❌ 之前
include(${CMAKE_BINARY_DIR}/Release/generators/conan_toolchain.cmake)

# ✅ 之后
include(${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/generators/conan_toolchain.cmake)
```

### 3. 删除冗余的全局 `include_directories`

**为什么优化**: 根 CMakeLists.txt 中存在三行全局 `include_directories`：

```cmake
include_directories(${PROJECT_SOURCE_DIR}/src/spdlog)
include_directories(${PROJECT_SOURCE_DIR}/src/random)
include_directories(${PROJECT_SOURCE_DIR}/src/leetcode)
```

分析发现这些路径对 `main.cpp` 的 `#include "random/TestRandom.h"` 等语句**完全没有作用**——实际解析靠的是 `main.cpp` 所在目录 (`src/`) 作为 `#include "..."` 的起始搜索路径。且各子库已在 `target_include_directories(... PUBLIC ...)` 中正确配置。全局 `include_directories` 仅污染所有目标的 include 路径，破坏模块隔离。

**如何优化**: 直接删除这三行。


## P1 — 重要问题

### 4. ScrollBar QSS 嵌入 C++ 违反项目规范

**为什么优化**: `ScrollBarStyler::styleSheet()` 将 80+ 行 QSS 作为 C++ 字符串字面量嵌入代码，与项目"样式放 .qss 文件"的规范矛盾。修改样式需要重新编译，不利于 UI 迭代。

**如何优化**:

1. 创建 `src/qt_widget/scrollbar_style.qss`，移入全部滚动条 QSS
2. `resources.qrc` 注册 `scrollbar_style.qss`
3. `ScrollBar.cpp` 改为运行时从 `:/scrollbar_style.qss` 加载：

```cpp
// ❌ 之前：80 行 QSS 字符串嵌在 C++ 中
static const QString &styleSheet() {
    if (s_styleSheet.isEmpty()) {
        s_styleSheet = QStringLiteral(
            "QScrollBar:vertical { background: #323649; ... }"
            "QScrollBar::handle:vertical { ... }"
            // ... 80+ 行
        );
    }
    return s_styleSheet;
}

// ✅ 之后：从 Qt 资源文件加载
static const QString &scrollbarStyleSheet() {
    QFile f(":/scrollbar_style.qss");
    if (f.open(QFile::ReadOnly | QFile::Text)) {
        s_styleSheet = QString::fromUtf8(f.readAll());
    }
    return s_styleSheet;
}
```

### 5. `commitEdit()` / `cancelEdit()` 重复代码

**为什么优化**: 两个方法 90% 逻辑相同（清理状态、恢复选中、发射信号），差异仅在"是否恢复原始文本"。这种重复意味着修复一个 Bug 时容易漏改另一处。

**如何优化**: 合并为 `finishEdit(bool accept)`：

```cpp
// ❌ 之前：两个几乎一样的方法，共约 50 行
void TableWidget::commitEdit() { ... }   // 26 行
void TableWidget::cancelEdit()  { ... }  // 24 行

// ✅ 之后：一个方法
void TableWidget::finishEdit(bool accept) {
    if (!isEditing()) return;
    auto *item = m_table->item(m_row, m_col);
    if (item) {
        if (accept) {
            if (item->text().isEmpty()) item->setText(m_originalText);
        } else {
            item->setText(m_originalText);
        }
    }
    // 公共清理逻辑 ...
    emit cellEditingFinished();
}
```

调用处语义更清晰：`finishEdit(true)` 确认，`finishEdit(false)` 取消。

### 6. 移除 `#ifdef UNUSED` 死代码

**为什么优化**: `main.cpp` 中存在 `#ifdef UNUSED ... #else ... #endif` 预处理器块，由于 `UNUSED` 从未定义，`#ifdef` 分支是永久死代码，增加阅读负担。

**如何优化**: 删除预处理器开关，保留实际执行的代码路径。


## P2 — 架构优化

### 7. 提取 EditController（TableWidget 职责分离）

**为什么优化**: TableWidget 在重构前承担了 **5 种职责**：UI 布局、数据填充、QSS 加载、编辑状态机、列标志管理。其中编辑状态机（`onCellClicked` / `handleKeyInput` / `finishEdit`）占约 130 行代码，是一个完整的状态管理子系统，应独立封装。

分离后的好处：
- TableWidget 从 300 行缩减到 210 行
- EditController 可独立单元测试
- 编辑逻辑修改不影响 TableWidget 的 UI 代码

**如何优化**:

```
重构前:                          重构后:
TableWidget                     TableWidget
  ├── setupUI()                   ├── setupUI()
  ├── populateSampleData()        ├── populateSampleData()
  ├── onCellClicked()     ──→     ├── EditController
  ├── handleKeyInput()    ──→     │     ├── onCellClicked()
  ├── finishEdit()        ──→     │     ├── handleKeyInput()
  ├── isEditing()                 │     └── finishEdit()
  ├── setCellText()               ├── setCellText()
  └── cellText()                  └── cellText()
```

TableWidget 通过信号转发保持对外 API 不变：

```cpp
// TableWidget 内部
m_editCtrl = new EditController(m_table, this);
connect(m_editCtrl, &EditController::editingStarted,  this, &TableWidget::cellEditingStarted);
connect(m_editCtrl, &EditController::editingFinished, this, &TableWidget::cellEditingFinished);

// 外部调用 handleKeyInput → 透明委托
void TableWidget::handleKeyInput(const QString &key) {
    m_editCtrl->handleKeyInput(key);
}
```

### 8. Stub AGL 嵌入 App Bundle

**为什么优化**: 重构前 AGL stub framework 通过**绝对路径** rpath 引用：

```cmake
-Wl,-rpath,${CMAKE_SOURCE_DIR}/stub_frameworks
```

这导致 `.app` 拿到任何其他机器或目录下都会因路径不存在而崩溃。

**如何优化**: 利用 CMake 的 `POST_BUILD` 步骤将 AGL 复制进 `.app` bundle 内部，并用 `@executable_path` 引用：

```cmake
# 链接时：用项目路径找到 stub AGL
target_link_options(qt_table_app PRIVATE
    -F${CMAKE_SOURCE_DIR}/stub_frameworks
    -Wl,-rpath,@executable_path/../Frameworks    # ← 相对路径
)

# 构建后：复制到 bundle
add_custom_command(TARGET qt_table_app POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/stub_frameworks"
        "$<TARGET_BUNDLE_CONTENT_DIR:qt_table_app>/Frameworks"
)
```

验证：
```
$ ls qt_table_app.app/Contents/Frameworks/AGL.framework/Versions/A/AGL
✓ AGL embedded in bundle
```


## P3 — 代码质量

### 9. QSS 加载失败静默忽略 → 加 qWarning

**为什么优化**: 所有 `loadStyleSheet()` 方法在 QSS 文件打开失败时静默返回，开发者完全不知道样式未加载。

**如何优化**: 在所有样式加载点添加 `qWarning`：

```cpp
// VirtualKeyboard::loadStyleSheet()
if (!f.open(...)) {
    qWarning("VirtualKeyboard: failed to load :/keyboard_style.qss");
}

// TableWidget::setupUI()
if (!styleFile.open(...)) {
    qWarning("TableWidget: failed to load :/style.qss");
}

// ScrollBarStyler
if (!f.open(...)) {
    qWarning("ScrollBarStyler: failed to load :/scrollbar_style.qss");
}
```

### 10. 删除未使用的 `m_allKeys`

**为什么优化**: `VirtualKeyboard::m_allKeys` 只有 `append()` 写入，从未读取。死代码占用内存且误导读者。

**如何优化**: 从 `VirtualKeyboard.h` 删除 `QVector<QPushButton *> m_allKeys` 成员及所有 `m_allKeys.append()` 调用。

### 11. `sender()` 反模式 → Lambda

**为什么优化**: `VirtualKeyboard::onKeyClicked()` 依赖 `sender()` 获取信号源：

```cpp
auto *btn = qobject_cast<QPushButton *>(sender());  // fragile
```

`sender()` 的问题：(1) 类型不安全，编译期无法检查；(2) 对信号来源的变更敏感；(3) 多线程下行为不确定。

**如何优化**: 在 `createKey()` 中直接用 lambda 捕获键值：

```cpp
// ❌ 之前
connect(btn, &QPushButton::clicked, this, &VirtualKeyboard::onKeyClicked);

// ✅ 之后
const QString emitValue = keyValue.isEmpty() ? text : keyValue;
connect(btn, &QPushButton::clicked, this,
        [this, emitValue]() { emit keyPressed(emitValue); });
```

### 12. float 宽度参数被 int 截断

**为什么优化**: 键位布局定义使用 `float` 宽度（如 Tab=1.5, Caps=1.75），但 `createKey()` 参数声明为 `int`，导致 1.5f 被隐式截断为 1，键帽宽度不准确。

**如何优化**: 参数改为 `float`：

```cpp
// ❌ 之前
QPushButton *createKey(const QString &text, int widthUnits = 1, ...);

// ✅ 之后
QPushButton *createKey(const QString &text, float widthUnits = 1.0f, ...);
```

### 13. `random_lib` 不恰当地 PUBLIC 链接 spdlog

**为什么优化**: `target_link_libraries(random_lib PUBLIC spdlog::spdlog)` 将 spdlog 的 include 路径传播给所有链接 `random_lib` 的目标。但 `TestRandom.h` 头文件完全没有引用 spdlog 类型，这是不必要的依赖泄漏。

**如何优化**: 改为 `PRIVATE`：

```cmake
# ❌ 之前
target_link_libraries(random_lib PUBLIC spdlog::spdlog)

# ✅ 之后
target_link_libraries(random_lib PRIVATE spdlog::spdlog)
```


## 第二轮 — WaveformChart 代码审查优化

对新增的波形图表组件进行代码审查后，发现 6 个可优化点，按 P0-P2 优先级处理。

### 14. 标记隐藏判断 `m_markerX < 0` 不可靠

**为什么优化**: 标记的"隐藏"状态通过 `m_markerX < 0` 判定。如果波形数据的 X 轴从负值开始（如 `-5.0 ~ 5.0`），标记即使在有效范围内也会被误判隐藏。

```cpp
// ❌ 问题：数据从 -1.0 开始时，m_markerX 合法值为负，但被误判隐藏
if (m_markerX < 0 || m_data.isEmpty()) return;
```

**如何优化**: 用显式的 `bool m_markerVisible` 替代 sentinel 值：

```cpp
// ✅ 之后
bool m_markerVisible = false;

void drawMarker(...) {
    if (!m_markerVisible || m_data.isEmpty()) return;  // 显式判断
}
```

### 15. 坐标标签右边界溢出

**为什么优化**: 当标记拖到图表右边缘时，坐标标签按固定偏移放在右侧，会绘制到图表区域外，部分内容不可见。

**如何优化**: 检测右边界溢出后，将标签翻转到标记**左侧**显示：

```cpp
const bool overflowRight = (px.x() + kTriHalfW + 6 + textW > r.right());
const int labelX = overflowRight
    ? static_cast<int>(px.x()) - kTriHalfW - 6 - textW   // 翻转到左侧
    : static_cast<int>(px.x()) + kTriHalfW + 6;           // 默认右侧
```

### 16. `niceStep()` 算法重复定义

**为什么优化**: Nice Numbers（最佳刻度间距）算法在 `drawGrid()` 和 `drawAxisLabels()` 中各有一份完全相同的 lambda 实现。重复代码 = 双重维护风险。

**如何优化**: 提取为类的静态方法，两处调用共享同一实现：

```cpp
// ✅ 头文件声明
static double niceStep(double span, int targetDivs);

// ✅ .cpp 中单一定义，drawGrid 和 drawAxisLabels 共用
const double xMajorStep = niceStep(tRange.span(), 10);
const double yMajorStep = niceStep(aRange.span(), 8);
```

### 17. `chartRect` 计算在 3 处重复

**为什么优化**: `paintEvent`、`mousePressEvent`、`mouseMoveEvent` 中各有一份相同的 chartRect 计算（6 行），违反 DRY 原则。

**如何优化**: 提取为内联方法：

```cpp
QRect WaveformChart::chartRect() const {
    return QRect(kMarginLeft, kMarginTop,
                 width()  - kMarginLeft - kMarginRight,
                 height() - kMarginTop  - kMarginBottom);
}
```

所有调用处简化为 `const QRect cr = chartRect();`。

### 18. QFont 每帧重复创建

**为什么优化**: `drawAxisLabels()` 和 `drawMarker()` 在每次 `paintEvent` 中都用 `QFont("Monospace", 9)` 创建新字体对象。高频渲染场景下（鼠标拖动标记每秒触发数十次 repaint），不必要的内存分配累积。

**如何优化**: 在构造函数中初始化，成员中缓存复用：

```cpp
// 构造函数中
m_axisFont  = QFont(QStringLiteral("Monospace"), 9);
m_titleFont = QFont(QStringLiteral("Monospace"), 12, QFont::Bold);
m_labelFont = QFont(QStringLiteral("Monospace"), 10, QFont::Bold);

// paintEvent 中直接使用
p.setFont(m_axisFont);  // 无需每帧创建
```

### 19. 颜色值散布代码各处

**为什么优化**: WaveformChart 中有 16 个颜色魔数分散在 5 个方法中：

```cpp
// ❌ 散布各处的魔数
QColor(255, 255, 255, 25)   // 次网格线 (drawGrid)
QColor(255, 255, 255, 50)   // 主网格线 (drawGrid)
QColor(0x1a, 0x1d, 0x2e)      // 背景     (drawBackground)
QColor(0, 210, 210, 180)      // 波形线   (drawCurve)
QColor(255, 180, 30, 240)    // 标记三角形 (drawMarker)
// ... 共 16 个
```

调整主题色时需要逐个定位修改，容易漏改。

**如何优化**: 提取为 `static constexpr` 命名常量，集中定义在头文件中：

```cpp
// 主题颜色常量集中管理
static constexpr int kBgR = 0x1a, kBgG = 0x1d, kBgB = 0x2e;
static constexpr int kWaveR = 0, kWaveG = 210, kWaveB = 210, kWaveAlpha = 180;
static constexpr int kGridMinorAlpha = 25, kGridMajorAlpha = 50;
static constexpr int kMarkerR = 255, kMarkerG = 180, kMarkerB = 30;
// ... 共 20 个常量，换主题只需改这一块
```

### 20. `CMakeUserPresets.json` 纳入 .gitignore

**为什么优化**: 该文件由 Conan 的 `cmake_layout` 自动生成，包含本地构建类型路径（`build/Release/generators/CMakePresets.json`）。已提交到版本控制后，每次 `conan install` 可能产生差异，且其他开发者路径不同会导致不必要的 diff。

**如何优化**: 添加到 `.gitignore` 并从 git 跟踪中移除：

```gitignore
CMakeUserPresets.json     # Conan 自动生成
```

### 本轮效果

| 指标 | 优化前 | 优化后 |
|---|---|---|
| WaveformChart.cpp 行数 | 413 | 380 (-33) |
| 重复函数定义 | 2 处 niceStep | 1 处 |
| 重复 chartRect | 3 处 | 1 处 `chartRect()` |
| 颜色魔数 | 16 个散落 | 20 个常量集中 |
| 标记隐藏判断 | m_markerX < 0 | m_markerVisible |
| 标签溢出处理 | 无 | 左右自适应翻转 |

---

# 构建目标

| 目标 | 说明 | 运行方式 |
|---|---|---|
| `main` | 控制台程序 (spdlog + leetcode) | `./build/main` |
| `qt_table_app` | Qt 应用（表格 + 键盘 + 波形示波器 + 标记器） | `open build/qt_table_app.app` |

# 项目结构

```
.
├── CMakeLists.txt              # 根 CMake 配置
├── conanfile.txt               # Conan 依赖声明 (spdlog)
├── src/
│   ├── main.cpp                # 控制台程序入口
│   ├── spdlog/logutil.*        # 日志工具封装
│   ├── random/                 # 随机数示例
│   ├── leetcode/
│   │   ├── LongestPalindrome/  # 最长回文子串
│   │   └── SumOfTwoNum/        # 两数之和
│   └── qt_widget/
│       ├── main_qt.cpp              # Qt 应用入口（QTabWidget 双标签页）
│       ├── resources.qrc            # Qt 资源索引
│       ├── widgets/                 # 视觉组件
│       │   ├── TableWidget.*        #   人员信息表
│       │   ├── VirtualKeyboard.*    #   98 键虚拟键盘
│       │   └── WaveformChart.*      #   波形示波器
│       ├── core/                    # 工具/逻辑组件
│       │   ├── EditController.*     #   编辑状态机
│       │   ├── ScrollBar.*          #   滚动条样式管理
│       │   ├── PenIconDelegate.*    #   钢笔图标委托
│       │   └── WaveformData.*       #   波形数据模型
│       └── styles/                  # QSS 样式表
│           ├── table.qss            #   表格深色主题
│           ├── keyboard_style.qss   #   键帽立体感样式
│           └── scrollbar_style.qss  #   滚动条深色风格
├── data/
│   └── arbitrary_wave.csv           # 自定义波形坐标点示例
├── stub_frameworks/
│   └── AGL.framework/          # AGL 桩框架 (macOS 26 兼容)
├── logs/                       # 日志输出目录
└── .vscode/
    ├── c_cpp_properties.json   # IntelliSense 配置
    ├── settings.json           # Code Runner 等设置
    ├── launch.json             # F5 调试配置
    └── tasks.json              # CMake 构建任务
```
