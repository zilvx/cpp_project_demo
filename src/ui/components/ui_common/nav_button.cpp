#include "nav_button.h"

NavButton::NavButton(const QString &text, QWidget *parent)
    : QPushButton(text, parent), m_active(false)
{
    setCheckable(true);
    setStyleSheet(R"(
        QPushButton {
            background-color: #e8ecf1;
            color: #3a4a5a;
            border: none;
            padding: 12px 20px;
            font-size: 13px;
            font-weight: 500;
            min-width: 80px;
            border-radius: 4px;
        }
        QPushButton:checked {
            background-color: #2a6f9c;
            color: white;
            font-weight: 600;
        }
        QPushButton:hover:not(:checked) {
            background-color: #e0e5ec;
        }
        QPushButton:checked:hover {
            background-color: #1e5a80;
        }
    )");
}

void NavButton::setActive(bool active) {
    m_active = active;
    setChecked(active);
    if (active) {
        setStyleSheet(R"(
            QPushButton {
                background-color: #2a6f9c;
                color: white;
                border: none;
                padding: 12px 20px;
                font-size: 13px;
                font-weight: 600;
                min-width: 80px;
                border-radius: 4px;
            }
            QPushButton:hover {
                background-color: #1e5a80;
            }
        )");
    } else {
        setStyleSheet(R"(
            QPushButton {
                background-color: #e8ecf1;
                color: #3a4a5a;
                border: none;
                padding: 12px 20px;
                font-size: 13px;
                font-weight: 500;
                min-width: 80px;
                border-radius: 4px;
            }
            QPushButton:hover:not(:checked) {
                background-color: #e0e5ec;
            }
        )");
    }
}
