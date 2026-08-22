#include "group_box.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>

GroupBox::GroupBox(const QString &title, QWidget *parent)
    : QGroupBox(title, parent), m_badge(new QLabel(this))
{
    // 设置容器布局
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 12, 10, 12);
    layout->setSpacing(0);

    // 徽标样式
    m_badge->setFixedSize(20, 20);
    m_badge->setStyleSheet(R"(
        QLabel {
            background-color: #2a6f9c;
            color: white;
            border-radius: 10px;
            font-weight: 600;
            font-size: 12px;
            text-align: center;
        }
    )");
    m_badge->setText("1");
    m_badge->hide(); // 默认隐藏

    // 将徽标放在标题旁边
    QFrame *titleFrame = new QFrame(this);
    QHBoxLayout *titleLayout = new QHBoxLayout(titleFrame);
    titleLayout->setContentsMargins(0, 0, 0, 0);
    titleLayout->setSpacing(8);

    QLabel *titleLabel = new QLabel(title, this);
    titleLabel->setStyleSheet(R"(
        QLabel {
            color: #2a6f9c;
            font-weight: 600;
            font-size: 14px;
        }
    )");
    titleLayout->addWidget(m_badge);
    titleLayout->addWidget(titleLabel);
    titleLayout->addStretch();

    layout->addWidget(titleFrame);
}

void GroupBox::setBadge(const QString &text) {
    m_badge->setText(text);
    m_badge->show();
}

void GroupBox::setBadgeColor(const QColor &color) {
    QString styleSheet = QString(R"(
        QLabel {
            background-color: %1;
            color: white;
            border-radius: 10px;
            font-weight: 600;
            font-size: 12px;
            text-align: center;
        }
    )").arg(color.name());
    m_badge->setStyleSheet(styleSheet);
}
