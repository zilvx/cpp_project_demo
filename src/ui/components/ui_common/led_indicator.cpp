#include "led_indicator.h"
#include <QTimer>
#include <QLabel>
#include <QHBoxLayout>
#include <QColor>
#include <QPainter>
#include <QResizeEvent>

LedIndicator::LedIndicator(QWidget *parent)
    : QWidget(parent), m_blinkTimer(new QTimer(this)),
      m_color(Qt::green), m_status(Status::Off), m_isBlinking(false)
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_led = new QLabel(this);
    m_led->setFixedSize(16, 16);
    m_led->setStyleSheet(R"(
        QLabel {
            background-color: #e0e5ec;
            border-radius: 8px;
        }
    )");

    layout->addWidget(m_led);

    // 闪烁定时器
    connect(m_blinkTimer, &QTimer::timeout, this, &LedIndicator::onTimerTick);
}

void LedIndicator::setStatus(Status status, QColor color) {
    if (m_status == status) return;

    m_status = status;
    m_color = color;

    if (m_status == Status::On || m_status == Status::Blinking) {
        QString bgStyle = QString(R"(
            QLabel {
                background-color: %1;
                border-radius: 8px;
            }
        )").arg(color.name());

        if (m_status == Status::Blinking) {
            bgStyle += R"(
                QLabel {
                    animation: blink 0.5s infinite;
                }
            )";
            startBlinking();
        } else {
            stopBlinking();
        }

        m_led->setStyleSheet(bgStyle);
    } else {
        m_led->setStyleSheet(R"(
            QLabel {
                background-color: #e0e5ec;
                border-radius: 8px;
            }
        )");
        stopBlinking();
    }

    emit statusChanged(status);
}

void LedIndicator::setColor(QColor color) {
    m_color = color;
    if (m_status == Status::On || m_status == Status::Blinking) {
        QString bgStyle = QString(R"(
            QLabel {
                background-color: %1;
                border-radius: 8px;
            }
        )").arg(color.name());
        m_led->setStyleSheet(bgStyle);
    }
}

void LedIndicator::startBlinking(int interval) {
    if (m_isBlinking) return;
    m_isBlinking = true;
    m_blinkTimer->start(interval);
}

void LedIndicator::stopBlinking() {
    if (!m_isBlinking) return;
    m_isBlinking = false;
    m_blinkTimer->stop();
    // 重置为常亮状态
    m_led->setStyleSheet(QString(R"(
        QLabel {
            background-color: %1;
            border-radius: 8px;
        }
    )").arg(m_color.name()));
}

void LedIndicator::onTimerTick() {
    static bool visible = true;
    visible = !visible;
    if (visible) {
        m_led->setStyleSheet(QString(R"(
            QLabel {
                background-color: %1;
                border-radius: 8px;
            }
        )").arg(m_color.name()));
    } else {
        m_led->setStyleSheet(R"(
            QLabel {
                background-color: #e0e5ec;
                border-radius: 8px;
            }
        )");
    }
}
