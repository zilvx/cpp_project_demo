#ifndef USAGE_EXAMPLE_H
#define USAGE_EXAMPLE_H

#include <QWidget>

/**
 * @brief UI组件使用示例
 *
 * 本文件展示如何使用新创建的通用UI组件
 *
 * 示例1：使用 ParamRow 创建参数行
 *   auto *freqRow = new ParamRow("频率", m_freqSpinBox, "MHz", this);
 *
 * 示例2：使用 LedIndicator 显示状态
 *   auto *led = new LedIndicator(this);
 *   led->setStatus(LedIndicator::Status::On, QColor("#27ae60"));
 *
 * 示例3：使用 StatRow 显示统计信息
 *   auto *stat = new StatRow("CPU", "45", "%", this);
 *
 * 示例4：使用 GroupBox 创建分组
 *   auto *gb = new GroupBox("载波配置", this);
 *   gb->setBadge("1");
 *
 * 示例5：使用 NavButton 创建导航按钮
 *   auto *btn = new NavButton("载波", this);
 *   btn->setActive(true);
 *
 * 示例6：使用 LCDDisplay 显示数值
 *   auto *lcd = new LCDDisplay("2.4", "GHz", this);
 *   lcd->setValue("2.45");
 *
 * 示例7：使用 KeyboardPad 创建虚拟键盘
 *   auto *kbd = new KeyboardPad(this);
 *   connect(kbd, &KeyboardPad::keyPressed, this, &MyWidget::handleKeyPress);
 */
class UsageExample : public QWidget {
    Q_OBJECT
public:
    explicit UsageExample(QWidget *parent = nullptr);

private:
    void setupParamRows();
    void setupLedIndicators();
    void setupStatRows();
    void setupGroupBoxes();
    void setupNavButtons();
    void setupLCDDisplay();
    void setupKeyboardPad();
};

#endif // USAGE_EXAMPLE_H
