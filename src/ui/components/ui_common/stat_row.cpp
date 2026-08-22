#include "stat_row.h"
#include <QHBoxLayout>

StatRow::StatRow(const QString &label, const QString &value,
                 const QString &unit, QWidget *parent)
    : QWidget(parent), m_layout(new QHBoxLayout(this)),
      m_label(new QLabel(label)), m_value(new QLabel(value)), m_unit(new QLabel(unit))
{
    // 标签样式
    m_label->setStyleSheet(R"(
        QLabel {
            color: #6b7b8c;
            font-size: 12px;
            min-width: 40px;
        }
    )");

    // 数值样式
    m_value->setStyleSheet(R"(
        QLabel {
            color: #1a2a3a;
            font-weight: 600;
            font-size: 14px;
            min-width: 60px;
        }
    )");

    // 单位样式
    if (!unit.isEmpty()) {
        m_unit->setStyleSheet(R"(
            QLabel {
                color: #6b7b8c;
                font-size: 12px;
                min-width: 30px;
            }
        )");
        m_unit->setText(unit);
        m_layout->addWidget(m_unit);
    }

    m_layout->addWidget(m_label);
    m_layout->addWidget(m_value);
    m_layout->setSpacing(6);
    m_layout->setContentsMargins(0, 0, 0, 0);
}

void StatRow::setValue(const QString &value) {
    m_value->setText(value);
}

void StatRow::setStyle(Style style) {
    QString styleSheet;

    switch (style) {
        case Style::Normal:
            styleSheet = R"(
                QLabel {
                    color: #1a2a3a;
                    font-weight: 600;
                    font-size: 14px;
                }
            )";
            break;
        case Style::Warning:
            styleSheet = R"(
                QLabel {
                    color: #f39c12;
                    font-weight: 600;
                    font-size: 14px;
                }
            )";
            break;
        case Style::Error:
            styleSheet = R"(
                QLabel {
                    color: #e74c3c;
                    font-weight: 600;
                    font-size: 14px;
                }
            )";
            break;
    }

    m_value->setStyleSheet(styleSheet);
}
