# 问题排查记录

## 1. Code Runner 报错 `spdlog/spdlog.h file not found`

Code Runner 直接调用 `clang++`，不走 CMake，无法获取 Conan 的 include 路径。

**解决**: 配置 Code Runner 通过 CMake 构建后运行：
```json
"code-runner.executorMap": {
    "cpp": "cd $workspaceRoot && cmake --build build && clear && $workspaceRoot/build/main"
}
```

## 2. Code Runner 报错 `could not load cache`

`build/` 目录为空或 Conan toolchain 路径不匹配。

**解决**: 清理重建
```bash
rm -rf build/*
conan install . --output-folder=. --build=missing -s build_type=Release
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## 3. SumOfTwoNum 程序崩溃，日志截断

`SumOfTwoNum::twoSum()` 声明返回 `std::vector<int>`，但函数末尾缺少 `return` 语句，触发未定义行为。

**解决**: 添加 `return result;`

## 4. Qt6 链接 `ld: framework 'AGL' not found`

macOS 26 (15.x SDK) 移除了 AGL 框架，但 Qt6 的 FindWrapOpenGL.cmake 仍硬编码 `-framework AGL`。

**解决**: 创建 stub AGL framework（见 `stub_frameworks/`），CMake 通过 `-F` 和 `-rpath` 引用。

## 5. Qt 应用启动崩溃: `Library not loaded: @rpath/AGL.framework`

stub AGL 的 rpath 未包含在可执行文件中。

**解决**: `target_link_options` 中同时设置 `-F` 和 `-Wl,-rpath,...`

## 6. QTableWidget 行号错乱 + 数据丢失

`setSortingEnabled(true)` 在数据填充前调用，每次 `setItem()` 触发排序导致行号映射混乱。

**解决**: 数据填充完毕后再启用排序。

## 7. QTableWidget 左上角白色背景

QTableCornerButton 默认使用系统原生样式，不继承 HeaderView QSS。

**解决**: 在 `table.qss` 中单独设置 `QTableCornerButton::section` 样式。

## 8. 深色滚动条封装 — ScrollBar 解耦

80 行 QSS 嵌入 C++ 代码，违反"样式放 .qss"规范。

**解决**: 创建独立的 `scrollbar_style.qss`，通过 `:/scrollbar_style.qss` 加载。

## 9. 钢笔图标位置 — QStyledItemDelegate

`QTableWidgetItem::setIcon()` 只能将图标放文字左侧，不满足右下角需求。

**解决**: 创建 `PenIconDelegate` 继承 `QStyledItemDelegate`，重写 `paint()` 在右下角绘制。

## 10. 虚拟键盘组件

98 键紧凑布局机械键盘风格，数据驱动 6 行键位定义，lambda 绑定键值消除 `sender()` 反模式。

## 11. 虚拟键盘接管表格编辑

禁用默认 `QLineEdit` 编辑器，信号/槽联动虚拟键盘完成输入。

## 12. 波形图表组件

Catmull-Rom 样条 → Cubic Bezier 平滑曲线，Nice Numbers 刻度算法，正弦/方波/锯齿波 + CSV 导入。

## 13. 波形标记器

鼠标拖动三角形标记沿曲线滑动，二分查找 + 线性插值计算 Y 坐标。

## 14. 目录结构重组

`widgets/` / `core/` / `styles/` 三层分离，CMakeLists.txt 同步更新。
