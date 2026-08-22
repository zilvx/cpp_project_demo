#include "lcd_display.h"
#include <QTimer>
#include <QHBoxLayout>
#include <QLabel>
#include <QFont>

LCDDisplay::LCDDisplay(const QString &defaultValue, const QString &unit,
                       QWidget *parent)
    : QWidget(parent), m_layout(new QHBoxLayout(this)),
      m_display(new QLabel(defaultValue)), m_unit(new QLabel(unit)),
      m_animationTimer(new QTimer(this)),
      m_decimalPlaces(3), m_formatter(nullptr)
{
    // 显示样式 - 模拟LCD
    QFont displayFont;
    displayFont.setFamily("Courier New");
    displayFont.setBold(true);
    displayFont.setPointSize(16);
    m_display->setFont(displayFont);

    m_display->setStyleSheet(R"(
        QLabel {
            background-color: #1a2a3a;
            color: #2a6f9c;
            border-radius: 4px;
            padding: 8px 12px;
            min-height: 36px;
            min-width: 100px;
            font-weight: bold;
            font-size: 18px;
        }
    )");

    if (!unit.isEmpty()) {
        m_unit->setStyleSheet(R"(
            QLabel {
                color: #6b7b8c;
                font-size: 12px;
                font-weight: 500;
            }
        )");
        m_unit->setText(unit);
        m_layout->addWidget(m_unit);
    }

    m_layout->addWidget(m_display);
    m_layout->setSpacing(8);
    m_layout->setContentsMargins(0, 0, 0, 0);

    connect(m_animationTimer, &QTimer::timeout, this, &LCDDisplay::onTimerTick);
}

void LCDDisplay::setValue(const QString &value, bool animate) {
    if (m_currentValue == value) return;
    m_currentValue = value;

    // 格式化数值
    QString displayValue = value;
    if (m_formatter) {
        displayValue = m_formatter(value);
    } else if (m_decimalPlaces > 0) {
        // 固定小数位
        bool ok;
        double num = value.toDouble(&ok);
        if (ok) {
            displayValue = QString::number(num, 'f', m_decimalPlaces);
        }
    }

    m_display->setText(displayValue);
    emit valueChanged(value);

    if (animate) {
        m_animationTimer->start(100);  // 100ms刷新一次
    } else {
        m_animationTimer->stop();
    }
}

void LCDDisplay::setUnit(const QString &unit) {
    if (m_unit) {
        m_unit->setText(unit);
    }
}

void LCDDisplay::setDecimals(int decimals) {
    if (decimals >= 0 && decimals <= 10) {
        m_decimalPlaces = decimals;
    }
}

void LCDDisplay::setValueFormatter(ValueFormatter formatter) {
    m_formatter = formatter;
}

void LCDDisplay::onTimerTick() {
    static int frame = 0;
    frame = (frame + 1) % 5;  // 循环5帧
    QString animValue = m_currentValue;
    if (!animValue.isEmpty() && animValue.length() > 0) {
        // 简单的闪烁效果
        if (frame == 0) {
            m_display->setText("    ");  // 空格占位
        } else if (frame == 2) {
            m_display->setText(m_currentValue);
        }
    }
    if (frame == 4) {
        m_animationTimer->stop();
    }
}
