# 代码架构优化记录

## P0 — 构建系统硬编码

### 1. 硬编码 `CMAKE_BUILD_TYPE`
`set(CMAKE_BUILD_TYPE Release)` 覆盖用户传入值 → 改为条件默认值仅在未指定时设置。

### 2. 硬编码 Conan Toolchain 路径
`include(.../Release/generators/...)` 导致 Debug 配置找不到 toolchain → `${CMAKE_BUILD_TYPE}` 动态路径。

### 3. 删除冗余全局 `include_directories`
三行全局 include 对 `main.cpp` 无作用，各子库已通过 `target_include_directories(... PUBLIC)` 配置。

---

## P1 — 重要问题

### 4. ScrollBar QSS 嵌入 C++
80+ 行 QSS 作为字符串嵌入代码 → 提取到 `scrollbar_style.qss`，运行时加载。

### 5. `commitEdit()` / `cancelEdit()` 重复
90% 相同逻辑 → 合并为 `finishEdit(bool accept)`。

### 6. 移除 `#ifdef UNUSED` 死代码
`UNUSED` 宏从未定义，永久死代码 → 删除。

---

## P2 — 架构优化

### 7. 提取 EditController
TableWidget 5 种职责拆分 → 编辑状态机独立为 EditController（130 行缩减，可单测）。

### 8. Stub AGL 嵌入 App Bundle
绝对路径 rpath 使 .app 不可移植 → `@executable_path/../Frameworks` 相对路径 + `POST_BUILD` 复制。

---

## P3 — 代码质量

### 9. QSS 加载失败 → qWarning
静默返回 → 所有加载点添加 `qWarning()`。

### 10. 删除未使用的 `m_allKeys`
`VirtualKeyboard::m_allKeys` 只写不读 → 删除。

### 11. `sender()` → Lambda
类型不安全 → `createKey()` 中 lambda 捕获键值。

### 12. float → int 截断
`createKey(int widthUnits)` → `createKey(float widthUnits)`。

### 13. `random_lib` PUBLIC → PRIVATE 链接 spdlog
头文件未引用 spdlog 类型 → 改为 PRIVATE，避免依赖泄漏。

---

## 第二轮 — WaveformChart 审查

### 14. 标记隐藏 `m_markerX < 0` 不可靠
负值数据误判 → `bool m_markerVisible` 显式标志位。

### 15. 坐标标签右边界溢出
固定右侧偏移 → 检测溢出后翻转到左侧。

### 16. `niceStep()` 重复定义
`drawGrid()` 和 `drawAxisLabels()` 各一份 → 提取为静态方法。

### 17. `chartRect` 3 处重复
`paintEvent`/`mousePress`/`mouseMove` 各一份 → 提取为 `chartRect()`。

### 18. QFont 每帧重复创建
`paintEvent` 中 `QFont("Monospace", 9)` → 构造函数初始化，成员缓存。

### 19. 颜色魔数散布
16 个魔数 5 个方法 → 20 个 `static constexpr` 命名常量。

### 20. `CMakeUserPresets.json` → .gitignore
Conan 自动生成，不同开发者路径不同 → 添加 `.gitignore`。

---

### 效果

| 指标 | 优化前 | 优化后 |
|------|--------|--------|
| WaveformChart.cpp 行数 | 413 | 380 |
| TableWidget 行数 | 300 | 210 |
| 重复代码 | 3 处 | 0 处 |
| 颜色魔数 | 16 个散落 | 20 个常量集中 |
