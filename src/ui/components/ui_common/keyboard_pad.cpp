#include "keyboard_pad.h"
#include <QGridLayout>
#include <QPushButton>
#include <QLabel>

KeyboardPad::KeyboardPad(QWidget *parent)
    : QWidget(parent), m_layout(new QGridLayout(this)),
      m_currentRow(0), m_currentCol(0)
{
    setStyleSheet(R"(
        QWidget {
            background-color: #f8f9fb;
            border-radius: 8px;
            padding: 8px;
        }
        QPushButton {
            background-color: #ffffff;
            color: #1a2a3a;
            border: 1px solid #cdd3db;
            border-radius: 4px;
            padding: 8px 12px;
            font-size: 13px;
            font-weight: 500;
            min-height: 36px;
            min-width: 48px;
        }
        QPushButton:hover {
            background-color: #e0e5ec;
            border-color: #bdc3c7;
        }
        QPushButton:pressed {
            background-color: #2a6f9c;
            color: white;
            border-color: #2a6f9c;
        }
    )");

    // 创建96个键位（4行4列的扩展键盘）
    const QString rows[] = {
        "1234567890",
        "QWERTYUIOP",
        "ASDFGHJKL",
        "ZXCVBNM"
    };

    const QString topRow = "QWERTYUIOP[]\\0123456789-=";

    // 第一行：QWERTYUIOP[]\0123456789-=
    m_currentRow = 0;
    m_currentCol = 0;
    for (int i = 0; i < 10; ++i) {
        m_keys[i] = new QPushButton(QString(rows[0][i]), this);
        m_layout->addWidget(m_keys[i], 0, i);
        m_currentCol++;
    }
    m_keys[10] = new QPushButton("[", this);
    m_layout->addWidget(m_keys[10], 0, 10);
    m_keys[11] = new QPushButton("]", this);
    m_layout->addWidget(m_keys[11], 0, 11);
    m_keys[12] = new QPushButton("\\", this);
    m_layout->addWidget(m_keys[12], 0, 12);
    m_keys[13] = new QPushButton("`", this);
    m_layout->addWidget(m_keys[13], 0, 13);
    m_currentCol += 4;

    // 第二行：ASDFGHJKL;'
    m_currentRow = 1;
    m_currentCol = 0;
    for (int i = 0; i < 9; ++i) {
        m_keys[14 + i] = new QPushButton(QString(rows[2][i]), this);
        m_layout->addWidget(m_keys[14 + i], 1, i);
        m_currentCol++;
    }
    m_keys[23] = new QPushButton(";", this);
    m_layout->addWidget(m_keys[23], 1, 9);
    m_keys[24] = new QPushButton("'", this);
    m_layout->addWidget(m_keys[24], 1, 10);
    m_currentCol += 2;

    // 第三行：ZXCVBNM,./
    m_currentRow = 2;
    m_currentCol = 0;
    for (int i = 0; i < 9; ++i) {
        m_keys[25 + i] = new QPushButton(QString(rows[3][i]), this);
        m_layout->addWidget(m_keys[25 + i], 2, i);
        m_currentCol++;
    }
    m_keys[34] = new QPushButton(",", this);
    m_layout->addWidget(m_keys[34], 2, 9);
    m_keys[35] = new QPushButton(".", this);
    m_layout->addWidget(m_keys[35], 2, 10);
    m_keys[36] = new QPushButton("/", this);
    m_layout->addWidget(m_keys[36], 2, 11);
    m_currentCol += 3;

    // 第四行：数字行
    m_currentRow = 3;
    m_currentCol = 0;
    for (int i = 0; i < 10; ++i) {
        m_keys[37 + i] = new QPushButton(QString(rows[1][i]), this);
        m_layout->addWidget(m_keys[37 + i], 3, i);
        m_currentCol++;
    }
    m_keys[47] = new QPushButton("-", this);
    m_layout->addWidget(m_keys[47], 3, 10);
    m_keys[48] = new QPushButton("=", this);
    m_layout->addWidget(m_keys[48], 3, 11);
    m_currentCol += 2;

    m_layout->setSpacing(4);
    m_layout->setContentsMargins(4, 4, 4, 4);
    m_layout->setColumnStretch(0, 1);
    m_layout->setColumnStretch(1, 1);
    m_layout->setColumnStretch(2, 1);
    m_layout->setColumnStretch(3, 1);
}

void KeyboardPad::setShowControlKeys(bool show) {
    // 未来扩展：显示Shift、Ctrl、Alt、Enter等控制键
    Q_UNUSED(show);
}
