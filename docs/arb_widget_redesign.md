# ARB Widget 重新设计总结

## 设计参考
基于 [Keysight VSG](file:///Users/shenxing/Downloads/deepseek_html_20260820_ff613b.html) 仪器设计风格重新设计。

## 实现功能

### 1. 四标签页布局
- **载波**：信号参数配置、ARB波形加载
- **波形**：波形库列表、波形属性、波形预览图表
- **硬件**：硬件配置、实时状态监控
- **播放**：播放控制、播放列表、触发设置

### 2. 布局结构

#### 载波页
- **左侧**：参数配置面板
  - 载波频率控制（输入框 + 上下箭头按钮）
  - 输出功率控制（输入框 + 上下箭头按钮）
  - ARB波形加载（文件信息 + 浏览/下载按钮）
  - 采样率控制（输入框 + 上下箭头按钮）
  - 循环模式选择（下拉框）
  - 操作按钮（应用并播放 / 停止）
- **右侧**：前面板（独立组件）
  - 品牌标识 (KEYSIGHT AP5041A G3)
  - LCD屏幕显示（FREQ/POWER/ARB状态/波形名称）
  - 控制旋钮（频率/功率）
  - 硬件按钮（ARB/暂停/停止/RF）
  - LED状态指示灯（MOD/ARB/OVLD/LOCK）

#### 波形页
- **左侧**：
  - 工具栏（新建、导入、编辑、复制、删除、下载）
  - 波形库列表（表格显示所有波形）
  - 波形预览（WaveformChart图表）
- **右侧**：波形属性侧边栏
  - 波形名称、类型、采样率、点数、I/Q格式
  - 快速操作按钮（预览播放、保存波形）

#### 硬件页
- **左侧**：
  - 频率参考卡片（参考源、精度、锁定状态）
  - 射频输出卡片（输出状态、功率范围、阻抗）
  - 温度监控卡片（当前温度、风扇转速、状态）
  - 接口连接卡片（LAN/USB/GPIB）
  - 底部信息栏
- **右侧**：实时状态侧边栏
  - CPU/内存/ARB内存使用
  - 输出功率、VSWR
  - 校准有效期
  - 最近事件日志

#### 播放页
- **左侧**：
  - 播放控制区（播放/暂停/停止/下一段按钮）
  - 播放进度条和时间显示
  - 播放设置（循环模式、触发源、触发延迟、段间间隔）
  - 播放列表
- **右侧**：触发与同步侧边栏
  - 触发模式、触发输入/输出
  - 同步时钟、帧同步
  - Marker设置
  - 同步播放按钮

### 3. 交互功能

#### 参数调整
- 频率：上下箭头按钮 + 文本输入框
- 功率：上下箭头按钮 + 文本输入框
- 采样率：上下箭头按钮 + 文本输入框
- 所有参数支持直接编辑

#### 前面板交互
- ARB按钮：切换ARB调制开关
- 暂停按钮：暂停/恢复播放
- 停止按钮：停止所有输出
- RF按钮：切换射频输出（红色/绿色状态）

#### 波形操作
- 新建波形、导入波形、编辑波形
- 复制波形、删除波形
- 下载到仪器

#### 播放控制
- 播放、暂停、停止、下一段
- 播放列表管理
- 循环模式选择
- 触发源设置
- 触发延迟设置

### 4. 样式设计
- **主题**：蓝色系（#2a6f9c）专业仪器风格
- **字体**：Segoe UI / Helvetica Neue
- **前面板**：深色渐变背景（#1a1a2e → #2a2a3e）
- **LED指示灯**：绿（正常）、黄（警告）、红（错误）
- **LCD屏幕**：等宽字体、绿色/黄色/青色数字显示
- **整体风格**：类似Keysight AP5041A实际仪器

### 5. 文件结构
```
src/ui/widgets/
├── ArbWidget.h           # 头文件（布局类定义）
├── ArbWidget.cpp         # 实现文件（完整功能实现）
└── ../styles/
    └── arb_widget.qss    # QSS样式文件（美化界面）
```

## 使用方法

### 在主UI中集成
```cpp
#include "ui/widgets/ArbWidget.h"

// 创建ARB Widget
ArbWidget *arbWidget = new ArbWidget(parent);

// 加载样式
arbWidget->setStyleSheet(
    "QFile(":/styles/arb_widget.qss").readAll()"
);

// 可以设置为独立窗口或嵌入到某个布局中
```

### 在CMakeLists.txt中添加
```cmake
add_executable(qt_table_app
    main_ui.cpp
    ui/main_forked_demo.cpp
    ui/widgets/ArbWidget.cpp
    ui/widgets/ArbWidget.h
    ui/styles/arb_widget.qss
)
```

## 核心类说明

### ArbWidget 类
- **继承**：QWidget
- **职责**：ARB任意波形发生器界面的完整实现
- **主要方法**：
  - `setupUI()` - 初始化UI布局
  - `setupConnections()` - 设置信号槽连接
  - `createCarrierPanel()` - 创建载波控制面板
  - `createWaveformSidebar()` - 创建波形属性侧边栏
  - `createHardwareSidebar()` - 创建硬件状态侧边栏
  - `createPlaybackSidebar()` - 创建播放同步侧边栏
  - `createFrontPanel()` - 创建前面板
  - `adjustFrequency()` - 调整频率
  - `adjustPower()` - 调整功率
  - `adjustSampleRate()` - 调整采样率
  - `onApplyAndPlay()` - 应用并播放
  - `onStop()` - 停止
  - `updateHardwareStatus()` - 更新前面板状态

### 辅助函数
- `createParamGroup()` - 创建参数组（带标题的QGroupBox）
- `createValueRow()` - 创建值行（输入框 + 单位 + 按钮）
- `createHwCard()` - 创建硬件配置卡片

## 信号
- `rfToggled(bool enabled)` - RF开关状态变化

## 优势
1. **模块化设计**：每个页面和侧边栏独立创建，易于维护
2. **专业风格**：完全参考Keysight VSG设计，仿真真实仪器
3. **功能完整**：覆盖波形加载、参数调整、播放控制等核心功能
4. **可扩展性**：清晰的架构，便于添加新功能（如波形编辑、网络控制等）
5. **样式统一**：QSS文件集中管理样式，易于定制

## 待实现功能
- 波形编辑功能
- 播放列表拖拽排序
- 触发输出控制
- 网络远程控制
- 波形导出/导入增强
- 波形统计信息显示

## 编译运行
```bash
# 编译项目
cmake --build build --target qt_table_app

# 运行应用
./build/qt_table_app.app
```

## 版本信息
- **设计日期**：2026-08-21
- **参考设计**：Keysight VSG / Signal Studio
- **版本**：v2.0
