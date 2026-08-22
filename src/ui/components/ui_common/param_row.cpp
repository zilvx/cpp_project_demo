#include "param_row.h"
#include <QLineEdit>
#include <QAbstractSpinBox>

ParamRow::ParamRow(const QString &label, QWidget *control,
                   const QString &unit, QWidget *parent)
    : QWidget(parent), m_layout(new QHBoxLayout(this)),
      m_label(new QLabel(label)), m_control(control), m_unit(new QLabel(unit))
{
    // 设置样式
    m_label->setStyleSheet(R"(
        QLabel {
            color: #2a6f9c;
            font-weight: 600;
            font-size: 13px;
            min-width: 60px;
        }
    )");

    m_control->setMaximumWidth(120);

    if (!unit.isEmpty()) {
        m_unit->setStyleSheet(R"(
            QLabel {
                color: #6b7b8c;
                font-size: 12px;
                min-width: 40px;
            }
        )");
        m_unit->setText(unit);
        m_layout->addWidget(m_unit);
    }

    m_layout->addWidget(m_label);
    m_layout->addWidget(m_control);
    m_layout->setSpacing(8);
    m_layout->setContentsMargins(0, 0, 0, 0);
}

void ParamRow::setControlStyle(bool readonly, bool error) {
    QPalette pal = m_control->palette();

    if (readonly) {
        if (auto *le = qobject_cast<QLineEdit *>(m_control)) {
            le->setReadOnly(true);
        } else if (auto *sb = qobject_cast<QAbstractSpinBox *>(m_control)) {
            sb->setReadOnly(true);
        }
        m_control->setStyleSheet(R"(
            QLineEdit, QSpinBox, QDoubleSpinBox {
                background-color: #f0f3f7;
                color: #a0b0c0;
                border: 1px solid #d9dce1;
                border-radius: 4px;
                padding: 6px;
            }
        )");
    } else if (error) {
        m_control->setStyleSheet(R"(
            QLineEdit, QSpinBox, QDoubleSpinBox {
                background-color: #fff0f0;
                color: #e74c3c;
                border: 1px solid #ffcccc;
                border-radius: 4px;
                padding: 6px;
            }
        )");
    } else {
        m_control->setStyleSheet(R"(
            QLineEdit, QSpinBox, QDoubleSpinBox {
                background-color: #ffffff;
                color: #1a2a3a;
                border: 1px solid #cdd3db;
                border-radius: 4px;
                padding: 6px;
            }
        )");
    }
}
