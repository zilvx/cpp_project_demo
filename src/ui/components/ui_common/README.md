# UI通用组件库

本目录包含可复用的UI组件，用于简化UI开发并保持一致性。

## 📦 组件列表

### 1. ParamRow (参数行)
**用途**：统一参数输入的UI展示

**头文件**: `param_row.h/cpp`

**使用示例**：
```cpp
#include "param_row.h"

// 创建频率参数行
auto *freqRow = new ParamRow("频率", m_freqSpinBox, "GHz", parent);
m_freqSpinBox->setValue(2.4);

// 设置只读状态
freqRow->setControlStyle(true);

// 设置错误状态
freqRow->setControlStyle(false, true);
```

---

### 2. LedIndicator (LED指示灯)
**用途**：显示设备状态（电源、锁定、过载等）

**头文件**: `led_indicator.h/cpp`

**使用示例**：
```cpp
#include "led_indicator.h"

auto *led = new LedIndicator(parent);

// 设置为常亮（绿色）
led->setStatus(LedIndicator::Status::On, QColor("#27ae60"));

// 设置为闪烁（蓝色）
led->setStatus(LedIndicator::Status::Blinking, QColor("#2a6f9c"));

// 设置为熄灭
led->setStatus(LedIndicator::Status::Off);

// 信号：状态变化时触发
connect(led, &LedIndicator::statusChanged, [](LedIndicator::Status status) {
    // 处理状态变化
});
```

---

### 3. StatRow (统计行)
**用途**：显示状态信息（CPU、内存、频率、功率等）

**头文件**: `stat_row.h/cpp`

**使用示例**：
```cpp
#include "stat_row.h"

// 创建统计行
auto *cpuStat = new StatRow("CPU", "45", "%", parent);
auto *memStat = new StatRow("内存", "2.4", "GB", parent);

// 更新数值
cpuStat->setValue("65");

// 设置样式
cpuStat->setStyle(StatRow::Style::Normal);      // 正常
cpuStat->setStyle(StatRow::Style::Warning);     // 警告
cpuStat->setStyle(StatRow::Style::Error);       // 错误
```

---

### 4. GroupBox (分组框)
**用途**：组织和分类UI元素，支持badge显示

**头文件**: `group_box.h/cpp`

**使用示例**：
```cpp
#include "group_box.h"

// 创建分组框
auto *gb = new GroupBox("载波配置", parent);

// 设置徽标（步骤编号、警告标记等）
gb->setBadge("1");
gb->setBadge("⚠️");
gb->setBadge("3");

// 设置徽标颜色
gb->setBadgeColor(QColor("#2a6f9c"));
gb->setBadgeColor(QColor("#f39c12")); // 黄色
gb->setBadgeColor(QColor("#e74c3c")); // 红色
```

---

### 5. NavButton (导航按钮)
**用途**：显示导航步骤，支持激活/非激活状态

**头文件**: `nav_button.h/cpp`

**使用示例**：
```cpp
#include "nav_button.h"

// 创建导航按钮
auto *btn = new NavButton("载波", parent);

// 设置激活状态
btn->setActive(true);

// 获取当前状态
if (btn->isActive()) {
    // 处理激活状态
}
```

---

### 6. LCDDisplay (LCD显示屏)
**用途**：显示数值（频率、功率、时间等），支持滚动动画

**头文件**: `lcd_display.h/cpp`

**使用示例**：
```cpp
#include "lcd_display.h"

// 创建LCD显示
auto *lcd = new LCDDisplay("2.4", "GHz", parent);

// 更新数值（自动格式化3位小数）
lcd->setValue("2.45");

// 更新数值（禁用动画）
lcd->setValue("2.45", false);

// 设置固定小数位
lcd->setDecimals(2);

// 设置数值格式化回调
lcd->setValueFormatter([](const QString &value) {
    // 自定义格式化
    return QString("¥%1").arg(value);
});

// 获取显示标签
QLabel *display = lcd->displayLabel();
```

---

### 7. KeyboardPad (虚拟键盘)
**用途**：提供97键机械键盘布局

**头文件**: `keyboard_pad.h/cpp`

**使用示例**：
```cpp
#include "keyboard_pad.h"

// 创建虚拟键盘
auto *kbd = new KeyboardPad(parent);

// 显示/隐藏控制键（Shift、Ctrl等）
kbd->setShowControlKeys(true);

// 监听按键事件
connect(kbd, &KeyboardPad::keyPressed, [](const QString &key) {
    qDebug() << "按键:" << key;
});
```

---

## 🎨 设计系统

### Design System (design_system.qss)
统一的设计规范，包含颜色、间距、圆角、字体等视觉令牌。

**使用方法**：
```cpp
// 在主窗口初始化时应用设计系统样式
void MainWindow::setupUI() {
    QFile styleFile(":/styles/design_system.qss");
    if (styleFile.open(QIODevice::ReadOnly)) {
        QString style = QString::fromUtf8(styleFile.readAll());
        // 替换颜色变量（可选）
        style = style.arg(/* primaryColor */).arg(/* secondaryColor */);
        this->setStyleSheet(style);
    }
}
```

**CSS 变量**：
- `--primary`: 主色 `#2a6f9c`
- `--secondary`: 次色 `#f39c12`
- `--success`: 成功色 `#27ae60`
- `--warning`: 警告色 `#f39c12`
- `--danger`: 错误色 `#e74c3c`
- `--bg-primary`: 主背景 `#ffffff`
- `--bg-secondary`: 次背景 `#f8f9fb`

---

## 📝 使用示例

### 完整示例：载波配置面板

```cpp
#include "param_row.h"
#include "led_indicator.h"
#include "stat_row.h"
#include "group_box.h"
#include "lcd_display.h"

class CarrierConfigWidget : public QWidget {
    Q_OBJECT
public:
    CarrierConfigWidget(QWidget *parent = nullptr) : QWidget(parent) {
        auto *mainLayout = new QVBoxLayout(this);

        // 1. 使用 GroupBox 组织界面
        auto *configGroupBox = new GroupBox("载波配置", this);
        auto *configLayout = new QVBoxLayout(configGroupBox);

        // 2. 使用 ParamRow 创建参数行
        auto *freqRow = new ParamRow("频率", m_freqSpinBox, "GHz", configGroupBox);
        auto *powerRow = new ParamRow("功率", m_powerSpinBox, "dBm", configGroupBox);
        auto *bwRow = new ParamRow("带宽", m_bwSpinBox, "MHz", configGroupBox);

        configLayout->addWidget(freqRow);
        configLayout->addWidget(powerRow);
        configLayout->addWidget(bwRow);

        mainLayout->addWidget(configGroupBox);

        // 3. 使用 LCDDisplay 显示当前值
        auto *lcdGroup = new QGroupBox("当前状态");
        auto *lcdLayout = new QHBoxLayout(lcdGroup);

        auto *lcdFreq = new LCDDisplay("2.4", "GHz", lcdGroup);
        auto *lcdPower = new LCDDisplay("-10", "dBm", lcdGroup);
        auto *lcdBw = new LCDDisplay("80", "MHz", lcdGroup);

        lcdLayout->addWidget(lcdFreq);
        lcdLayout->addWidget(lcdPower);
        lcdLayout->addWidget(lcdBw);

        mainLayout->addWidget(lcdGroup);

        // 4. 使用 StatRow 显示统计信息
        auto *statsGroupBox = new QGroupBox("统计信息");
        auto *statsLayout = new QGridLayout(statsGroupBox);

        auto *cpuStat = new StatRow("CPU", "45", "%", statsGroupBox);
        auto *memStat = new StatRow("内存", "2.4", "GB", statsGroupBox);
        auto *tempStat = new StatRow("温度", "45", "°C", statsGroupBox);

        statsLayout->addWidget(cpuStat, 0, 0);
        statsLayout->addWidget(memStat, 0, 1);
        statsLayout->addWidget(tempStat, 0, 2);

        mainLayout->addWidget(statsGroupBox);

        // 5. 使用 LedIndicator 显示状态
        auto *ledGroup = new QHBoxLayout;
        auto *ledLock = new LedIndicator(this);
        ledLock->setStatus(LedIndicator::Status::On, QColor("#27ae60"));
        ledGroup->addWidget(new QLabel("锁定"));

        auto *ledArb = new LedIndicator(this);
        ledArb->setStatus(LedIndicator::Status::On, QColor("#2a6f9c"));
        ledGroup->addWidget(new QLabel("ARB"));

        mainLayout->addWidget(new QWidget);  // 占位
        mainLayout->addLayout(ledGroup);

        // 连接信号
        connect(m_freqSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                [this](double value) {
                    // 更新LCD
                    m_lcdFreq->setValue(QString::number(value, 'f', 2));
                });
    }

private:
    QDoubleSpinBox *m_freqSpinBox = new QDoubleSpinBox;
    QDoubleSpinBox *m_powerSpinBox = new QDoubleSpinBox;
    QDoubleSpinBox *m_bwSpinBox = new QDoubleSpinBox;
    LCDDisplay *m_lcdFreq = nullptr;
};
```

---

## 🔧 CMake 配置

这些组件已经添加到 CMakeLists.txt，自动编译到 `qt_table_app` 和 `qt_table_app_fork` 中。

**源文件已添加到**：
- `qt_table_app`: 包含所有组件源文件
- `qt_table_app_fork`: 包含所有组件源文件（除了 ToastService.h）

**自动包含路径**：
- `src/ui/components/ui_common/` 已添加到 include 路径

---

## 📊 组件对比表

| 组件 | 用途 | 典型场景 | 状态支持 |
|------|------|----------|----------|
| ParamRow | 参数输入 | 波形参数、载波配置 | 只读/错误 |
| LedIndicator | 状态指示 | 电源、锁定、过载 | 熄灭/常亮/闪烁 |
| StatRow | 数值显示 | CPU、内存监控 | 正常/警告/错误 |
| GroupBox | 界面分组 | 参数组、配置步骤 | 徽标显示 |
| NavButton | 导航切换 | 步骤导航、选项卡 | 激活/非激活 |
| LCDDisplay | 数值展示 | 频率、功率显示 | 无 |
| KeyboardPad | 虚拟键盘 | 表格编辑 | 无 |

---

## 🎯 最佳实践

### 1. 组件复用
优先使用通用组件，避免重复编写相同的UI代码：

```cpp
// ❌ 不推荐：重复创建参数行
auto *row1 = new QFrame;
auto *lbl1 = new QLabel("频率");
auto *spin1 = new QDoubleSpinBox;
// ... 重复代码

// ✅ 推荐：使用 ParamRow
auto *freqRow = new ParamRow("频率", spinBox, "GHz", parent);
```

### 2. 样式一致性
所有组件都使用设计系统的颜色变量，保持视觉一致性。

### 3. 信号连接
正确连接组件信号，避免内存泄漏：

```cpp
// ❌ 不推荐：lambda没有捕获列表
connect(led, &LedIndicator::statusChanged, [this](Status status) {
    updateUI(status);  // 编译错误
});

// ✅ 推荐：正确捕获 this
connect(led, &LedIndicator::statusChanged, [this](Status status) {
    updateUI(status);
});
```

### 4. 父对象设置
所有控件都应该设置父对象，保证正确的内存管理和事件传递：

```cpp
// ❌ 不推荐：没有父对象
auto *led = new LedIndicator;

// ✅ 推荐：设置父对象
auto *led = new LedIndicator(this);
```

---

## 🚀 未来扩展

### 待实现组件
- [ ] TabButton（可切换标签）
- [ ] Slider（滑动条）
- [ ] Switch（开关按钮）
- [ ] Popover（弹出框）
- [ ] Tooltip（工具提示）
- [ ] ProgressBar（进度条）
- [ ] CircularProgressBar（环形进度条）

### 优化方向
- [ ] 支持主题切换（深色/浅色）
- [ ] 支持动画过渡效果
- [ ] 支持国际化（多语言）
- [ ] 支持可访问性（键盘导航、屏幕阅读器）
- [ ] 单元测试覆盖

---

## 📚 相关资源

- **项目文档**: `CLAUDE.md`
- **设计规范**: `src/ui/styles/design_system.qss`
- **示例代码**: `usage_example.h/cpp`
- **样式文件**: `src/ui/styles/`

---

## 🤝 贡献

如果您有改进建议或有其他需要的UI组件，欢迎提交 Pull Request！

**贡献指南**：
1. 创建新组件文件（.h 和 .cpp）
2. 添加详细的注释和文档
3. 更新本 README.md
4. 添加使用示例
5. 确保组件符合设计系统规范
