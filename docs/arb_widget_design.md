# ArbWidget 重新设计文档

## 概述

基于 [Keysight VSG](file:///Users/shenxing/Downloads/deepseek_html_20260820_ff613b.html) 设计风格，重新设计了ARB任意波形发生器界面。

## 设计特点

### 1. 四标签页布局
- **载波**：信号参数配置、ARB波形加载
- **波形**：波形库列表、波形属性、预览
- **硬件**：硬件配置、实时状态
- **播放**：播放控制、播放列表、触发设置

### 2. 左右分栏设计
- **左侧**：控制面板（TabWidget + 标签页）
- **右侧**：前面板（LED状态、旋钮、硬件按钮）

### 3. 专业仪器风格
- 类似Keysight AP5041A的实际仪器外观
- LED指示灯、旋钮按钮、硬件开关
- 前面板实时显示参数

## 文件结构

```
src/ui/widgets/
├── ArbWidget.h           # 新设计头文件
├── ArbWidget.cpp         # 新设计实现文件
└── ... (其他文件)

src/ui/styles/
└── arb_widget.qss        # 界面样式文件
```

## 使用说明

### 1. 在主UI中集成

```cpp
// main_ui.cpp 或 main_forked_demo.cpp
#include "ui/widgets/ArbWidget.h"

// 在创建窗口时添加ArbWidget
ArbWidget *arbWidget = new ArbWidget(parent);
// 可以设置为独立窗口或嵌入到某个布局中
```

### 2. 加载QSS样式

```cpp
ArbWidget *arbWidget = new ArbWidget(parent);
arbWidget->setStyleSheet("QFile(":/styles/arb_widget.qss").readAll()");
```

### 3. 在CMakeLists.txt中添加

```cmake
# src/ui/CMakeLists.txt
add_executable(qt_table_app
    main_ui.cpp
    ui/main_forked_demo.cpp
    # ... 其他源文件

    ui/widgets/ArbWidget.cpp
    ui/widgets/ArbWidget.h
    ui/styles/arb_widget.qss
)
```

## 主要功能

### 载波页（Carrier）
- **频率控制**：上下箭头按钮调整载波频率
- **功率控制**：上下箭头按钮调整输出功率
- **ARB波形加载**：浏览和下载波形文件
- **采样率配置**：设置波形采样率
- **循环模式**：连续/单次/触发模式

### 波形页（Waveform）
- **波形库列表**：显示所有可用波形
- **波形属性**：显示波形名称、采样率、长度、状态
- **操作按钮**：新建、导入、编辑、复制、删除、下载

### 硬件页（Hardware）
- **频率参考**：显示参考源、精度、锁定状态
- **射频输出**：显示输出状态、功率范围、阻抗
- **温度监控**：显示温度、风扇转速
- **接口连接**：显示LAN、USB、GPIB状态

### 播放页（Playback）
- **播放控制**：播放/暂停/停止按钮
- **播放列表**：管理多个波形的播放序列
- **触发设置**：触发模式、触发源、触发延迟

### 前面板（Front Panel）
- **屏幕显示**：实时显示频率、功率、ARB状态、波形名称
- **控制旋钮**：频率旋钮、功率旋钮
- **硬件按钮**：ARB、暂停、停止、RF开关
- **LED指示灯**：MOD、ARB、OVLD、LOCK状态

## 接口说明

### 公共信号
```cpp
signals:
    void rfToggled(bool enabled);  // RF开关状态变化
```

### 公共槽函数
```cpp
void onApplyAndPlay();          // 应用并播放
void onStop();                   // 停止
void onUploadWaveform();        // 上传波形
void onRfToggle();              // RF开关切换
void onFreqUp();                // 频率上调
void onFreqDown();              // 频率下调
void onPowerUp();               // 功率上调
void onPowerDown();             // 功率下调
```

## 数据结构

### 当前状态
```cpp
// 载波参数
double m_frequency = 2.4;       // GHz
double m_power = -10.0;         // dBm
double m_sampleRate = 80.0;     // MHz
bool m_rfEnabled = false;       // RF开关状态
bool m_arbEnabled = true;       // ARB开关状态

// 波形数据
WaveformData m_currentData;     // 当前波形数据
QString m_currentWaveformPath;  // 波形文件路径
```

## 样式定制

### 自定义颜色主题
在 [arb_widget.qss](../src/ui/styles/arb_widget.qss) 中修改：
- 主色调：`#2a6f9c`（蓝色）
- 成功色：`#2ecc71`（绿色）
- 警告色：`#f1c40f`（黄色）
- 错误色：`#e74c3c`（红色）

### 调整字体
```qss
QWidget {
    font-family: 'Segoe UI', 'Helvetica Neue', Arial, sans-serif;
    font-size: 13px;
}
```

## 扩展建议

### 1. 添加波形预览图
在波形页添加QChart或QGraphicsView显示波形缩略图。

### 2. 实现播放列表
添加QTableWidget显示播放列表，支持拖拽排序、循环模式设置。

### 3. 增加波形编辑器
添加波形编辑功能，支持手动修改波形点数值。

### 4. 实现触发功能
添加触发源选择、触发延迟设置、触发输出控制。

### 5. 增加网络控制
支持通过网络接口远程控制仪器参数。

## 编译和运行

### 编译
```bash
cd /Users/shenxing/Desktop/Projects/cpp_project_demo
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### 运行
```bash
./build/qt_table_app
```

## 已知问题

1. 需要添加图标资源文件（箭头、复选框等）
2. 部分功能（波形编辑、播放列表）待实现
3. 硬件状态需要与实际仪器通信接口对接

## 联系方式

如有问题或建议，请提交Issue。
