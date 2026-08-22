#include "usage_example.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QFrame>

UsageExample::UsageExample(QWidget *parent)
    : QWidget(parent)
{
    auto *mainLayout = new QVBoxLayout(this);

    // ====== 示例1：参数行 ======
    auto *paramGroup = new QGroupBox("参数行示例");
    auto *paramLayout = new QVBoxLayout(paramGroup);

    auto *freqSpinBox = new QDoubleSpinBox;
    freqSpinBox->setRange(0.1, 10.0);
    freqSpinBox->setValue(2.4);

    auto *freqRow = new ParamRow("频率", freqSpinBox, "GHz", paramGroup);
    paramLayout->addWidget(freqRow);

    auto *powerSpinBox = new QDoubleSpinBox;
    powerSpinBox->setRange(-10.0, 10.0);
    powerSpinBox->setValue(-10);

    auto *powerRow = new ParamRow("功率", powerSpinBox, "dBm", paramGroup);
    paramLayout->addWidget(powerRow);

    mainLayout->addWidget(paramGroup);

    // ====== 示例2：LED指示灯 ======
    auto *ledGroup = new QGroupBox("LED指示灯");
    auto *ledLayout = new QHBoxLayout(ledGroup);

    auto *ledOn = new LedIndicator(ledGroup);
    ledOn->setStatus(LedIndicator::Status::On, QColor("#27ae60"));
    ledLayout->addWidget(new QLabel("正常 (绿色)"));

    auto *ledWarn = new LedIndicator(ledGroup);
    ledWarn->setStatus(LedIndicator::Status::On, QColor("#f39c12"));
    ledLayout->addWidget(new QLabel("警告 (黄色)"));

    auto *ledError = new LedIndicator(ledGroup);
    ledError->setStatus(LedIndicator::Status::On, QColor("#e74c3c"));
    ledLayout->addWidget(new QLabel("错误 (红色)"));

    auto *ledOff = new LedIndicator(ledGroup);
    ledOff->setStatus(LedIndicator::Status::Off);
    ledLayout->addWidget(new QLabel("熄灭"));

    auto *ledBlink = new LedIndicator(ledGroup);
    ledBlink->setStatus(LedIndicator::Status::Blinking, QColor("#2a6f9c"));
    ledLayout->addWidget(new QLabel("闪烁"));

    mainLayout->addWidget(ledGroup);

    // ====== 示例3：统计行 ======
    auto *statGroup = new QGroupBox("统计行示例");
    auto *statLayout = new QGridLayout(statGroup);

    auto *cpuStat = new StatRow("CPU", "45", "%", statGroup);
    statLayout->addWidget(cpuStat, 0, 0);

    auto *memStat = new StatRow("内存", "2.4", "GB", statGroup);
    statLayout->addWidget(memStat, 0, 1);

    auto *tempStat = new StatRow("温度", "45", "°C", statGroup);
    statLayout->addWidget(tempStat, 0, 2);

    cpuStat->setStyle(StatRow::Style::Normal);
    memStat->setStyle(StatRow::Style::Warning);
    tempStat->setStyle(StatRow::Style::Error);

    mainLayout->addWidget(statGroup);

    // ====== 示例4：分组框 ======
    auto *gbGroup = new QGroupBox("分组框示例");
    auto *gbLayout = new QVBoxLayout(gbGroup);

    auto *gb1 = new GroupBox("载波配置", gbGroup);
    gb1->setBadge("1");
    gbLayout->addWidget(gb1);

    auto *gb2 = new GroupBox("BWP配置", gbGroup);
    gb2->setBadge("2");
    gbLayout->addWidget(gb2);

    auto *gb3 = new GroupBox("用户配置", gbGroup);
    gb3->setBadge("3");
    gb3->setBadgeColor(QColor("#f39c12"));
    gbLayout->addWidget(gb3);

    mainLayout->addWidget(gbGroup);

    // ====== 示例5：导航按钮 ======
    auto *navGroup = new QGroupBox("导航按钮");
    auto *navLayout = new QHBoxLayout(navGroup);

    auto *nav1 = new NavButton("载波", navGroup);
    nav1->setActive(true);
    navLayout->addWidget(nav1);

    auto *nav2 = new NavButton("BWP", navGroup);
    navLayout->addWidget(nav2);

    auto *nav3 = new NavButton("用户", navGroup);
    navLayout->addWidget(nav3);

    auto *nav4 = new NavButton("路由", navGroup);
    navLayout->addWidget(nav4);

    mainLayout->addWidget(navGroup);

    // ====== 示例6：LCD显示屏 ======
    auto *lcdGroup = new QGroupBox("LCD显示屏");
    auto *lcdLayout = new QHBoxLayout(lcdGroup);

    auto *lcd1 = new LCDDisplay("2.4", "GHz", lcdGroup);
    lcdLayout->addWidget(new QLabel("频率："));
    lcdLayout->addWidget(lcd1);

    auto *lcd2 = new LCDDisplay("-10", "dBm", lcdGroup);
    lcdLayout->addWidget(new QLabel("功率："));
    lcdLayout->addWidget(lcd2);

    auto *lcd3 = new LCDDisplay("80", "MHz", lcdGroup);
    lcdLayout->addWidget(new QLabel("带宽："));
    lcdLayout->addWidget(lcd3);

    mainLayout->addWidget(lcdGroup);

    // ====== 示例7：虚拟键盘 ======
    auto *kbdGroup = new QGroupBox("虚拟键盘");
    auto *kbdLayout = new QVBoxLayout(kbdGroup);

    auto *kbd = new KeyboardPad(kbdGroup);
    kbdLayout->addWidget(kbd);

    auto *btn = new QPushButton("测试按键", kbdGroup);
    connect(btn, &QPushButton::clicked, kbd, [kbd]() {
        kbd->setStyleSheet(R"(
            QPushButton {
                background-color: #2a6f9c;
                color: white;
            }
        )");
        kbd->setStyleSheet(""); // 恢复默认
    });

    kbdLayout->addWidget(btn);

    mainLayout->addWidget(kbdGroup);
    mainLayout->addStretch();
}
