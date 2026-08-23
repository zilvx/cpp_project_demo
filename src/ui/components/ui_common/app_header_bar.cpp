#include "app_header_bar.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

AppHeaderBar::AppHeaderBar(QWidget *parent) : QWidget(parent) {
    setObjectName(QStringLiteral("appHeaderBar"));
    setupUI();
}

bool AppHeaderBar::isRfEnabled() const {
    return m_rfToggleBtn && m_rfToggleBtn->isChecked();
}

void AppHeaderBar::setRfEnabled(bool enabled) {
    if (!m_rfToggleBtn)
        return;
    m_rfToggleBtn->setChecked(enabled);
    m_rfToggleBtn->setText(enabled ? QStringLiteral("开启") : QStringLiteral("关闭"));
}

void AppHeaderBar::setupUI() {
    auto *lay = new QHBoxLayout(this);
    lay->setContentsMargins(24, 12, 24, 12);
    lay->setSpacing(12);

    auto *logoIcon = new QLabel(QStringLiteral("📶"));
    logoIcon->setObjectName(QStringLiteral("appHeaderLogoIcon"));
    lay->addWidget(logoIcon);

    auto *logoText = new QLabel(QStringLiteral("Signal Studio"));
    logoText->setObjectName(QStringLiteral("appHeaderLogoText"));
    lay->addWidget(logoText);

    auto *subText = new QLabel(QStringLiteral("for VSG"));
    subText->setObjectName(QStringLiteral("appHeaderLogoSub"));
    lay->addWidget(subText);

    auto *badge = new QLabel(QStringLiteral("AP5041A"));
    badge->setObjectName(QStringLiteral("appHeaderBadge"));
    lay->addWidget(badge);

    lay->addStretch();

    auto *dot = new QLabel(QStringLiteral("●"));
    dot->setObjectName(QStringLiteral("appHeaderConnDot"));
    lay->addWidget(dot);

    auto *connStatus = new QLabel(QStringLiteral("已连接 · 192.168.1.100"));
    connStatus->setObjectName(QStringLiteral("appHeaderConnText"));
    lay->addWidget(connStatus);

    auto *rfLabel = new QLabel(QStringLiteral("RF 输出"));
    rfLabel->setObjectName(QStringLiteral("appHeaderRfLabel"));
    lay->addWidget(rfLabel);

    m_rfToggleBtn = new QPushButton(QStringLiteral("开启"));
    m_rfToggleBtn->setObjectName(QStringLiteral("appHeaderRfToggleBtn"));
    m_rfToggleBtn->setCheckable(true);
    m_rfToggleBtn->setChecked(true);
    m_rfToggleBtn->setCursor(Qt::PointingHandCursor);
    lay->addWidget(m_rfToggleBtn);

    connect(m_rfToggleBtn, &QPushButton::clicked, this, [this]() {
        const bool on = m_rfToggleBtn->isChecked();
        m_rfToggleBtn->setText(on ? QStringLiteral("开启") : QStringLiteral("关闭"));
        emit rfToggled(on);
    });
}
