#include "BasicToolsWidget.h"

#include <QFile>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSplitter>
#include <QStackedWidget>
#include <QStyle>
#include <QVBoxLayout>

#include <spdlog/spdlog.h>

BasicToolsWidget::BasicToolsWidget(QWidget *tablePage, QWidget *wavePage,
                                   QWidget *filePage, QWidget *parent)
    : QWidget(parent) {
    setObjectName(QStringLiteral("BasicToolsWidget"));
    loadStyleSheet();
    setupUI(tablePage, wavePage, filePage);
    spdlog::info("BasicToolsWidget initialized");
}

BasicToolsWidget::~BasicToolsWidget() {
    spdlog::info("BasicToolsWidget destroyed");
}

void BasicToolsWidget::loadStyleSheet() {
    QFile f(QStringLiteral(":/basic_tools.qss"));
    if (f.open(QFile::ReadOnly | QFile::Text)) {
        setStyleSheet(QString::fromUtf8(f.readAll()));
    } else {
        spdlog::warn("BasicToolsWidget: failed to load :/basic_tools.qss");
    }
}

void BasicToolsWidget::setupUI(QWidget *tablePage, QWidget *wavePage,
                               QWidget *filePage) {
    auto *root = new QHBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setObjectName(QStringLiteral("btSplitter"));
    splitter->setChildrenCollapsible(false);
    root->addWidget(splitter);

    auto *nav = createNavPanel();
    nav->setMinimumWidth(200);
    splitter->addWidget(nav);

    m_stack = new QStackedWidget(this);
    m_stack->setObjectName(QStringLiteral("btStack"));
    tablePage->setParent(m_stack);
    wavePage->setParent(m_stack);
    filePage->setParent(m_stack);
    m_stack->addWidget(tablePage);
    m_stack->addWidget(wavePage);
    m_stack->addWidget(filePage);
    splitter->addWidget(m_stack);

    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({200, 850});
}

QWidget *BasicToolsWidget::createNavPanel() {
    auto *nav = new QWidget;
    nav->setObjectName(QStringLiteral("btNavPanel"));
    auto *lay = new QVBoxLayout(nav);
    lay->setContentsMargins(0, 12, 0, 12);
    lay->setSpacing(2);

    auto *secHeader = new QLabel(QStringLiteral("基础工具"));
    secHeader->setObjectName(QStringLiteral("btNavHeader"));
    lay->addWidget(secHeader);

    struct NavEntry {
        const char *icon;
        const char *label;
    };
    const NavEntry navs[] = {
        {"📋", "人员信息表"},
        {"〰", "波形示波器"},
        {"📁", "文件管理器"},
    };
    const QStringList navSteps = {QStringLiteral("1"), QStringLiteral("2"),
                                  QStringLiteral("3")};

    m_navBtns.clear();
    m_navBadges.clear();
    for (int i = 0; i < 3; ++i) {
        auto *btn = new QPushButton(QString::fromUtf8(navs[i].icon) +
                                    QStringLiteral("  ") +
                                    QString::fromUtf8(navs[i].label));
        btn->setObjectName(QStringLiteral("btNavItem"));
        btn->setCheckable(true);
        btn->setChecked(i == 0);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setProperty("pageIndex", i);

        auto *stepBadge = new QLabel(navSteps.at(i));
        stepBadge->setObjectName(QStringLiteral("btNavBadge"));
        stepBadge->setProperty("active", i == 0);
        stepBadge->style()->unpolish(stepBadge);
        stepBadge->style()->polish(stepBadge);

        auto *row = new QHBoxLayout;
        row->setContentsMargins(0, 0, 0, 0);
        row->addWidget(btn, 1);
        row->addWidget(stepBadge);
        auto *wrap = new QWidget;
        wrap->setLayout(row);
        lay->addWidget(wrap);

        m_navBtns.append(btn);
        m_navBadges.append(stepBadge);
        connect(btn, &QPushButton::clicked, this, [this, i]() { setCurrentIndex(i); });
    }

    auto *divider = new QFrame;
    divider->setObjectName(QStringLiteral("btNavDivider"));
    lay->addWidget(divider);

    auto *hint = new QLabel(QStringLiteral("在左侧切换子页面"));
    hint->setObjectName(QStringLiteral("btNavHint"));
    hint->setWordWrap(true);
    lay->addWidget(hint);

    lay->addStretch();
    return nav;
}

void BasicToolsWidget::setCurrentIndex(int index) {
    if (!m_stack || index < 0 || index >= m_stack->count())
        return;
    m_stack->setCurrentIndex(index);
    for (int i = 0; i < m_navBtns.size(); ++i) {
        const bool active = (i == index);
        m_navBtns[i]->setChecked(active);
        if (i < m_navBadges.size()) {
            m_navBadges[i]->setProperty("active", active);
            m_navBadges[i]->style()->unpolish(m_navBadges[i]);
            m_navBadges[i]->style()->polish(m_navBadges[i]);
        }
    }
}
