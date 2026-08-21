#include "FiveGNrWidget.h"

#include <QComboBox>
#include <QDateTime>
#include <QFile>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QSplitter>
#include <QStackedWidget>
#include <QTimer>
#include <QVBoxLayout>

#include <spdlog/spdlog.h>

#include "../core/WaveformData.h"

namespace {
// 设计稿配色常量（供内联样式使用，主体样式见 five_g_nr.qss）
constexpr const char *kBlue   = "#2a6f9c";   // 主色
constexpr const char *kBlueBg = "rgba(42, 111, 156, 0.08)"; // 激活项背景
constexpr const char *kGreen  = "#2ecc71";   // 开启 / 就绪
constexpr const char *kRed    = "#e74c3c";   // 关闭
constexpr const char *kPurple = "#8e44ad";   // BWP 层级
constexpr const char *kText   = "#1a2a3a";
constexpr const char *kSub    = "#5a6a7a";

// 载波频率显示（默认 FR2 28.000 GHz）
constexpr const char *kDefaultFreq = "28.000";
} // namespace

FiveGNrWidget::FiveGNrWidget(QWidget *parent) : QWidget(parent) {
    // 使 QSS 顶层选择器 #FiveGNrWidget 生效，确保根背景/字体与设计稿一致
    setObjectName(QStringLiteral("FiveGNrWidget"));
    loadStyleSheet();
    setupUI();
    setupConnections();

    updateSignalStatus();
    m_footerTime->setText(
        QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm")));
    spdlog::info("FiveGNrWidget initialized");
}

FiveGNrWidget::~FiveGNrWidget() {
    spdlog::info("FiveGNrWidget destroyed");
}

// ====== 样式加载 ======

void FiveGNrWidget::loadStyleSheet() {
    QFile f(QStringLiteral(":/five_g_nr.qss"));
    if (f.open(QFile::ReadOnly | QFile::Text)) {
        setStyleSheet(QString::fromUtf8(f.readAll()));
    } else {
        spdlog::warn("FiveGNrWidget: failed to load :/five_g_nr.qss");
    }
}

// ====== 主布局：标题栏 + Apps 栏 + 三栏主体 + 底部状态栏 ======

void FiveGNrWidget::setupUI() {
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // 顶部标题栏
    m_header = createHeader();
    root->addWidget(m_header);

    // Apps 菜单栏
    m_appsBar = createAppsBar();
    root->addWidget(m_appsBar);

    // 主体：左导航 / 中参数配置 / 右辅助面板
    m_mainSplitter = new QSplitter(Qt::Horizontal, this);
    m_mainSplitter->setObjectName(QStringLiteral("fgSplitter"));
    m_mainSplitter->setChildrenCollapsible(false);
    root->addWidget(m_mainSplitter, 1);

    m_mainSplitter->addWidget(createNavPanel());
    m_mainSplitter->addWidget(createConfigPanel());
    m_mainSplitter->addWidget(createSidePanel());
    m_mainSplitter->setStretchFactor(0, 0);
    m_mainSplitter->setStretchFactor(1, 1);
    m_mainSplitter->setStretchFactor(2, 0);
    m_mainSplitter->setSizes({200, 700, 260});

    // 底部状态栏
    m_footer = createFooter();
    root->addWidget(m_footer);
}

// ====== 顶部标题栏 ======

QWidget *FiveGNrWidget::createHeader() {
    auto *header = new QWidget;
    header->setObjectName(QStringLiteral("fgHeader"));
    auto *lay = new QHBoxLayout(header);
    lay->setContentsMargins(24, 12, 24, 12);
    lay->setSpacing(12);

    // Logo 区
    auto *logoIcon = new QLabel(QStringLiteral("📶"));
    logoIcon->setObjectName(QStringLiteral("fgLogoIcon"));
    lay->addWidget(logoIcon);

    auto *logoText = new QLabel(QStringLiteral("Signal Studio"));
    logoText->setObjectName(QStringLiteral("fgLogoText"));
    lay->addWidget(logoText);

    auto *subText = new QLabel(QStringLiteral("for 5G NR"));
    subText->setObjectName(QStringLiteral("fgLogoSub"));
    lay->addWidget(subText);

    auto *badge = new QLabel(QStringLiteral("M9484C VXG"));
    badge->setObjectName(QStringLiteral("fgModelBadge"));
    lay->addWidget(badge);

    auto *appIcon = new QLabel(QStringLiteral("5G NR"));
    appIcon->setObjectName(QStringLiteral("fgAppIcon"));
    lay->addWidget(appIcon);

    lay->addStretch();

    // 连接状态
    auto *dot = new QLabel(QStringLiteral("●"));
    dot->setObjectName(QStringLiteral("fgConnDot"));
    lay->addWidget(dot);

    auto *connStatus = new QLabel(QStringLiteral("已连接 · 192.168.1.100"));
    connStatus->setObjectName(QStringLiteral("fgConnText"));
    lay->addWidget(connStatus);

    auto *fwInfo = new QLabel(QStringLiteral("| 固件 v4.0.1"));
    fwInfo->setObjectName(QStringLiteral("fgFwText"));
    lay->addWidget(fwInfo);

    return header;
}

// ====== Apps 菜单栏 ======

QWidget *FiveGNrWidget::createAppsBar() {
    auto *bar = new QWidget;
    bar->setObjectName(QStringLiteral("fgAppsBar"));
    auto *lay = new QHBoxLayout(bar);
    lay->setContentsMargins(20, 8, 20, 8);
    lay->setSpacing(8);

    auto *appsLabel = new QLabel(QStringLiteral("🟦  Apps"));
    appsLabel->setObjectName(QStringLiteral("fgAppsLabel"));
    lay->addWidget(appsLabel);

    // Apps 按钮（5G NR 默认激活），图标 + 名称，5G NR 项带红色 5G 徽标
    struct AppEntry { const char *icon; const char *name; };
    const AppEntry apps[] = {
        {"📶", "5G NR"},
        {"📡", "WLAN"},
        {"📻", "LTE"},
        {"📱", "Bluetooth"},
        {"〰", "ARB"}
    };
    m_appBtns.clear();
    for (int i = 0; i < 5; ++i) {
        auto *wrap = new QWidget;
        auto *row = new QHBoxLayout(wrap);
        row->setContentsMargins(0, 0, 0, 0);
        row->setSpacing(6);

        auto *btn = new QPushButton(QString::fromUtf8(apps[i].icon) + QStringLiteral("  ") +
                                    QString::fromUtf8(apps[i].name));
        btn->setObjectName(QStringLiteral("fgAppItem"));
        btn->setCheckable(true);
        btn->setChecked(i == 0);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setProperty("appIndex", i);
        row->addWidget(btn);

        if (i == 0) {
            auto *badge = new QLabel(QStringLiteral("5G"));
            badge->setObjectName(QStringLiteral("fgAppBadge5g"));
            row->addWidget(badge);
        }

        m_appBtns.append(btn);
        lay->addWidget(wrap);
    }

    lay->addStretch();

    // RF 开关
    auto *rfLabel = new QLabel(QStringLiteral("⏻  RF"));
    rfLabel->setObjectName(QStringLiteral("fgRfLabel"));
    lay->addWidget(rfLabel);

    m_rfToggleBtn = new QPushButton(QStringLiteral("●  开启"));
    m_rfToggleBtn->setObjectName(QStringLiteral("fgRfToggleBtn"));
    m_rfToggleBtn->setCheckable(true);
    m_rfToggleBtn->setChecked(true);
    m_rfToggleBtn->setCursor(Qt::PointingHandCursor);
    lay->addWidget(m_rfToggleBtn);

    return bar;
}

// ====== 左侧导航面板 ======

QWidget *FiveGNrWidget::createNavPanel() {
    auto *nav = new QWidget;
    nav->setObjectName(QStringLiteral("fgNavPanel"));
    auto *lay = new QVBoxLayout(nav);
    lay->setContentsMargins(0, 12, 0, 12);
    lay->setSpacing(2);

    // 信号配置小节
    auto *secHeader = new QLabel(QStringLiteral("信号配置"));
    secHeader->setObjectName(QStringLiteral("fgNavHeader"));
    lay->addWidget(secHeader);

    struct NavEntry { const char *icon; const char *label; };
    const NavEntry navs[] = {
        {"📡", "载波 (Carrier)"},
        {"🗂", "BWP (带宽部分)"},
        {"👤", "用户 (UE)"},
        {"🧭", "路由 (Routing)"}
    };
    const QStringList navSteps = {QStringLiteral("1"), QStringLiteral("2"),
                                  QStringLiteral("3"), QStringLiteral("4")};
    m_navBtns.clear();
    for (int i = 0; i < 4; ++i) {
        auto *btn = new QPushButton(QString::fromUtf8(navs[i].icon) + QStringLiteral("  ") +
                                    QString::fromUtf8(navs[i].label));
        btn->setObjectName(QStringLiteral("fgNavItem"));
        btn->setCheckable(true);
        btn->setChecked(i == 0);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setProperty("navIndex", i);

        // 步骤徽标放在最右
        auto *stepBadge = new QLabel(navSteps.at(i));
        stepBadge->setObjectName(QStringLiteral("fgNavBadge"));
        stepBadge->setProperty("active", i == 0);
        auto *row = new QHBoxLayout;
        row->setContentsMargins(0, 0, 0, 0);
        row->addWidget(btn, 1);
        row->addWidget(stepBadge);
        auto *wrap = new QWidget;
        wrap->setLayout(row);
        lay->addWidget(wrap);

        m_navBtns.append(btn);
        m_navBadges.append(stepBadge);
    }

    // 分隔线
    auto *divider = new QFrame;
    divider->setObjectName(QStringLiteral("fgNavDivider"));
    lay->addWidget(divider);

    // 工具小节
    auto *toolHeader = new QLabel(QStringLiteral("工具"));
    toolHeader->setObjectName(QStringLiteral("fgNavHeader"));
    lay->addWidget(toolHeader);

    const QStringList tools = {
        QStringLiteral("📥  导入波形"),
        QStringLiteral("💾  保存预设"),
        QStringLiteral("📤  导出设置")
    };
    for (const QString &t : tools) {
        auto *btn = new QPushButton(t);
        btn->setObjectName(QStringLiteral("fgNavToolItem"));
        btn->setCursor(Qt::PointingHandCursor);
        lay->addWidget(btn);
    }

    lay->addStretch();
    return nav;
}

// ====== 中央参数配置面板 ======

QWidget *FiveGNrWidget::createConfigPanel() {
    auto *panel = new QWidget;
    panel->setObjectName(QStringLiteral("fgConfigPanel"));

    // 外层滚动区域，避免内容过多溢出
    auto *scroll = new QScrollArea(panel);
    scroll->setObjectName(QStringLiteral("fgConfigScroll"));
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->viewport()->setObjectName(QStringLiteral("fgConfigViewport"));

    auto *content = new QWidget;
    content->setObjectName(QStringLiteral("fgConfigContent"));
    auto *lay = new QVBoxLayout(content);
    lay->setContentsMargins(24, 20, 24, 20);
    lay->setSpacing(16);

    // 面板标题（由导航切换更新）+ 副标题 + 层级徽标
    auto *titleRow = new QHBoxLayout;
    titleRow->setSpacing(10);
    m_panelTitle = new QLabel(QStringLiteral("载波配置 (Cell)"));
    m_panelTitle->setObjectName(QStringLiteral("fgPanelTitle"));
    titleRow->addWidget(m_panelTitle);

    m_panelSub = new QLabel(
        QStringLiteral("— 设置 5G 小区的基础帧结构和频率参数"));
    m_panelSub->setObjectName(QStringLiteral("fgPanelSub"));
    titleRow->addWidget(m_panelSub);

    m_panelBadge = new QLabel(QStringLiteral("🧊  Cell"));
    m_panelBadge->setObjectName(QStringLiteral("fgPanelBadge"));
    titleRow->addWidget(m_panelBadge);
    titleRow->addStretch();
    lay->addLayout(titleRow);

    auto *descRow = new QHBoxLayout;
    descRow->setSpacing(8);
    descRow->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    auto *descIcon = new QLabel(QStringLiteral("ℹ"));
    descIcon->setObjectName(QStringLiteral("fgDescIcon"));
    descRow->addWidget(descIcon);
    m_panelDesc = new QLabel(
        QStringLiteral("配置频率范围 (FR1/FR2)、载波频率、带宽、子载波间隔和双工模式。"
                       "这些参数定义了 5G NR 信号的“骨架”。"));
    m_panelDesc->setObjectName(QStringLiteral("fgPanelDesc"));
    m_panelDesc->setWordWrap(true);
    descRow->addWidget(m_panelDesc, 1);
    lay->addLayout(descRow);

    // 快捷配置
    auto *quick = new QWidget;
    quick->setObjectName(QStringLiteral("fgQuickConfig"));
    auto *quickLay = new QHBoxLayout(quick);
    quickLay->setContentsMargins(14, 12, 14, 12);
    quickLay->setSpacing(10);

    auto *quickLabel = new QLabel(QStringLiteral("⚡  快速配置 (Full-filled Config)"));
    quickLabel->setObjectName(QStringLiteral("fgQuickLabel"));
    quickLay->addWidget(quickLabel);
    quickLay->addStretch();

    const QStringList presets = {
        QStringLiteral("FR1 100MHz"), QStringLiteral("FR2 400MHz"),
        QStringLiteral("FR2 800MHz"), QStringLiteral("FR2 1600MHz"),
        QStringLiteral("自定义")
    };
    for (int i = 0; i < presets.size(); ++i) {
        auto *btn = new QPushButton(presets.at(i));
        btn->setObjectName(i == 2 ? QStringLiteral("fgQuickBtnPrimary")
                                  : QStringLiteral("fgQuickBtn"));
        btn->setCheckable(true);
        btn->setChecked(i == 2);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setProperty("preset", presets.at(i));
        connect(btn, &QPushButton::clicked, this, &FiveGNrWidget::onPresetClicked);
        quickLay->addWidget(btn);
    }
    lay->addWidget(quick);

    // 参数页堆栈
    m_configStack = new QStackedWidget;
    m_configStack->setObjectName(QStringLiteral("fgConfigStack"));
    m_configStack->addWidget(createCarrierPage());
    m_configStack->addWidget(createBwpPage());
    m_configStack->addWidget(createUserPage());
    m_configStack->addWidget(createRoutingPage());
    lay->addWidget(m_configStack);

    // 生成按钮区
    auto *genArea = new QHBoxLayout;
    genArea->setSpacing(10);

    auto *genBtn = new QPushButton(QStringLiteral("⚙  生成波形 (Generate)"));
    genBtn->setObjectName(QStringLiteral("fgGenPrimary"));
    genBtn->setCursor(Qt::PointingHandCursor);
    connect(genBtn, &QPushButton::clicked, this, &FiveGNrWidget::onGenerateClicked);
    genArea->addWidget(genBtn, 2);

    auto *previewBtn = new QPushButton(QStringLiteral("▶  预览"));
    previewBtn->setObjectName(QStringLiteral("fgGenSecondary"));
    previewBtn->setCursor(Qt::PointingHandCursor);
    connect(previewBtn, &QPushButton::clicked, this, &FiveGNrWidget::onPreviewClicked);
    genArea->addWidget(previewBtn, 1);

    auto *saveBtn = new QPushButton(QStringLiteral("💾  保存预设"));
    saveBtn->setObjectName(QStringLiteral("fgGenSecondary"));
    saveBtn->setCursor(Qt::PointingHandCursor);
    connect(saveBtn, &QPushButton::clicked, this, &FiveGNrWidget::onSavePresetClicked);
    genArea->addWidget(saveBtn, 1);
    lay->addLayout(genArea);

    lay->addStretch();

    scroll->setWidget(content);
    auto *panelLay = new QVBoxLayout(panel);
    panelLay->setContentsMargins(0, 0, 0, 0);
    panelLay->addWidget(scroll);
    return panel;
}

// ====== 右侧辅助面板 ======

QWidget *FiveGNrWidget::createSidePanel() {
    auto *side = new QWidget;
    side->setObjectName(QStringLiteral("fgSidePanel"));
    auto *lay = new QVBoxLayout(side);
    lay->setContentsMargins(16, 16, 16, 16);
    lay->setSpacing(14);

    // ---- 配置层级卡片 ----
    auto *hierCard = new QWidget;
    hierCard->setObjectName(QStringLiteral("fgInfoCard"));
    auto *hierLay = new QVBoxLayout(hierCard);
    hierLay->setContentsMargins(14, 12, 14, 12);
    hierLay->setSpacing(6);

    auto *hierTitle = new QLabel(QStringLiteral("🗂  配置层级"));
    hierTitle->setObjectName(QStringLiteral("fgCardTitle"));
    hierLay->addWidget(hierTitle);

    auto makeHierLevel = [&hierLay](const QString &indent, const QString &lbl,
                                    QLabel *&val) {
        auto *row = new QHBoxLayout;
        auto *ind = new QLabel(indent);
        ind->setObjectName(QStringLiteral("fgHierIndent"));
        row->addWidget(ind);
        auto *l = new QLabel(lbl);
        l->setObjectName(QStringLiteral("fgHierLbl"));
        row->addWidget(l);
        val = new QLabel;
        val->setObjectName(QStringLiteral("fgHierVal"));
        row->addWidget(val);
        row->addStretch();
        hierLay->addLayout(row);
    };

    makeHierLevel(QStringLiteral("📡"), QStringLiteral("Cell"), m_hierCellVal);
    makeHierLevel(QStringLiteral("└─"), QStringLiteral("BWP"), m_hierBwpVal);
    makeHierLevel(QStringLiteral("  └─"), QStringLiteral("CORESET"), m_hierCoresetVal);
    makeHierLevel(QStringLiteral("  └─"), QStringLiteral("User"), m_hierUserVal);
    lay->addWidget(hierCard);

    // ---- 信号状态卡片 ----
    auto *statCard = new QWidget;
    statCard->setObjectName(QStringLiteral("fgInfoCard"));
    auto *statLay = new QVBoxLayout(statCard);
    statLay->setContentsMargins(14, 12, 14, 12);
    statLay->setSpacing(6);

    auto *statTitle = new QLabel(QStringLiteral("ℹ  信号状态"));
    statTitle->setObjectName(QStringLiteral("fgCardTitle"));
    statLay->addWidget(statTitle);

    auto makeStat = [&statLay](const QString &label, QLabel *&val) {
        auto *frame = new QFrame;
        frame->setObjectName(QStringLiteral("fgStatRow"));
        auto *row = new QHBoxLayout(frame);
        row->setContentsMargins(0, 0, 0, 0);
        row->setSpacing(0);
        auto *l = new QLabel(label);
        l->setObjectName(QStringLiteral("fgStatLabel"));
        row->addWidget(l);
        row->addStretch();
        val = new QLabel;
        val->setObjectName(QStringLiteral("fgStatValue"));
        row->addWidget(val);
        statLay->addWidget(frame);
    };

    makeStat(QStringLiteral("频率范围"), m_statFrVal);
    makeStat(QStringLiteral("载波频率"), m_statFreqVal);
    makeStat(QStringLiteral("带宽"), m_statBwVal);
    makeStat(QStringLiteral("子载波间隔"), m_statScsVal);
    makeStat(QStringLiteral("信号状态"), m_statStatusVal);
    lay->addWidget(statCard);

    // ---- 波形预览卡片 ----
    auto *waveCard = new QWidget;
    waveCard->setObjectName(QStringLiteral("fgInfoCard"));
    auto *waveLay = new QVBoxLayout(waveCard);
    waveLay->setContentsMargins(14, 12, 14, 12);
    waveLay->setSpacing(8);

    auto *waveTitle = new QLabel(QStringLiteral("〰  波形预览"));
    waveTitle->setObjectName(QStringLiteral("fgCardTitle"));
    waveLay->addWidget(waveTitle);

    m_previewChart = new WaveformChart(this);
    m_previewChart->setObjectName(QStringLiteral("fgPreviewChart"));
    m_previewChart->setTitle(QStringLiteral(""));
    m_previewChart->setXLabel(QStringLiteral("时间"));
    m_previewChart->setYLabel(QStringLiteral("幅值"));
    m_previewChart->setMinimumHeight(60);
    m_previewChart->setMaximumHeight(60);

    WaveformData previewData;
    previewData.generateSine(2.0, 1.0, 100.0, 4, 0.0, 0.0);
    m_previewChart->setData(previewData);
    waveLay->addWidget(m_previewChart);

    auto *waveFoot = new QLabel(QStringLiteral("I/Q 时域  ·  采样率: 245.76 MHz"));
    waveFoot->setObjectName(QStringLiteral("fgWaveFoot"));
    waveLay->addWidget(waveFoot);
    lay->addWidget(waveCard);

    lay->addStretch();
    return side;
}

// ====== 底部状态栏 ======

QWidget *FiveGNrWidget::createFooter() {
    auto *footer = new QWidget;
    footer->setObjectName(QStringLiteral("fgFooter"));
    auto *lay = new QHBoxLayout(footer);
    lay->setContentsMargins(24, 6, 24, 6);
    lay->setSpacing(12);

    m_footerStatus = new QLabel(
        QStringLiteral("<span style='color:%1;'>●</span>  就绪").arg(kGreen));
    m_footerStatus->setTextFormat(Qt::RichText);
    m_footerStatus->setObjectName(QStringLiteral("fgFooterItem"));
    lay->addWidget(m_footerStatus);

    lay->addWidget(new QLabel(QStringLiteral("|")));
    m_footerWave = new QLabel(QStringLiteral("波形: 5G_NR_FR2_800MHz_256QAM"));
    m_footerWave->setObjectName(QStringLiteral("fgFooterItem"));
    lay->addWidget(m_footerWave);

    lay->addWidget(new QLabel(QStringLiteral("|")));
    m_footerMem = new QLabel(QStringLiteral("内存: 8% 已用"));
    m_footerMem->setObjectName(QStringLiteral("fgFooterItem"));
    lay->addWidget(m_footerMem);

    lay->addStretch();

    m_footerModel = new QLabel(QStringLiteral("M9484C VXG"));
    m_footerModel->setObjectName(QStringLiteral("fgFooterItem"));
    lay->addWidget(m_footerModel);

    lay->addWidget(new QLabel(QStringLiteral("|")));
    m_footerFw = new QLabel(QStringLiteral("固件 v4.0.1"));
    m_footerFw->setObjectName(QStringLiteral("fgFooterItem"));
    lay->addWidget(m_footerFw);

    lay->addWidget(new QLabel(QStringLiteral("|")));
    m_footerTime = new QLabel;
    m_footerTime->setObjectName(QStringLiteral("fgFooterItem"));
    lay->addWidget(m_footerTime);

    return footer;
}

// ====== 参数辅助函数 ======

QFrame *FiveGNrWidget::createParamSection(const QString &title, const QString &badge,
                                          const QString &badgeClass) {
    auto *frame = new QFrame;
    frame->setObjectName(QStringLiteral("fgParamSection"));
    auto *lay = new QVBoxLayout(frame);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(8);

    auto *header = new QHBoxLayout;
    auto *titleLabel = new QLabel(title);
    titleLabel->setObjectName(QStringLiteral("fgSectionLabel"));
    header->addWidget(titleLabel);

    if (!badge.isEmpty()) {
        auto *b = new QLabel(badge);
        b->setObjectName(QStringLiteral("fgLevelBadge"));
        if (badgeClass == QStringLiteral("cell")) b->setProperty("role", QStringLiteral("cell"));
        else if (badgeClass == QStringLiteral("bwp")) b->setProperty("role", QStringLiteral("bwp"));
        else if (badgeClass == QStringLiteral("user")) b->setProperty("role", QStringLiteral("user"));
        header->addWidget(b);
    }
    header->addStretch();
    lay->addLayout(header);
    return frame;
}

QFrame *FiveGNrWidget::createParamRow() {
    auto *frame = new QFrame;
    frame->setObjectName(QStringLiteral("fgParamRow"));
    auto *lay = new QHBoxLayout(frame);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(24);
    return frame;
}

QFrame *FiveGNrWidget::makeParamItem(const QString &label, QWidget *control,
                                     const QString &unit) {
    auto *frame = new QFrame;
    frame->setObjectName(QStringLiteral("fgParamItem"));
    auto *lay = new QHBoxLayout(frame);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(8);

    auto *lbl = new QLabel(label);
    lbl->setObjectName(QStringLiteral("fgParamLabel"));
    lay->addWidget(lbl);

    lay->addWidget(control);

    if (!unit.isEmpty()) {
        auto *u = new QLabel(unit);
        u->setObjectName(QStringLiteral("fgParamUnit"));
        lay->addWidget(u);
    }
    return frame;
}

// ====== 载波页 ======

QWidget *FiveGNrWidget::createCarrierPage() {
    auto *page = new QWidget;
    auto *lay = new QVBoxLayout(page);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(16);

    // 基本参数
    auto *basic = createParamSection(QStringLiteral("🎚  基本参数"), QStringLiteral("CELL"),
                                     QStringLiteral("cell"));
    auto *basicLay = qobject_cast<QVBoxLayout *>(basic->layout());

    auto *row1 = createParamRow();
    auto *row1Lay = qobject_cast<QHBoxLayout *>(row1->layout());

    m_freqRangeCombo = new QComboBox;
    m_freqRangeCombo->addItems({QStringLiteral("FR1 (Sub-6 GHz)"),
                                QStringLiteral("FR2 (mmWave)")});
    m_freqRangeCombo->setCurrentIndex(1);
    row1Lay->addWidget(makeParamItem(QStringLiteral("频率范围"), m_freqRangeCombo));

    m_carrierFreqEdit = new QLineEdit(QString::fromUtf8(kDefaultFreq));
    m_carrierFreqEdit->setObjectName(QStringLiteral("fgFreqEdit"));
    row1Lay->addWidget(makeParamItem(QStringLiteral("载波频率"), m_carrierFreqEdit,
                                     QStringLiteral("GHz")));

    m_bwCombo = new QComboBox;
    m_bwCombo->addItems({QStringLiteral("100 MHz"), QStringLiteral("200 MHz"),
                         QStringLiteral("400 MHz"), QStringLiteral("800 MHz"),
                         QStringLiteral("1600 MHz"), QStringLiteral("2000 MHz")});
    m_bwCombo->setCurrentIndex(3);
    row1Lay->addWidget(makeParamItem(QStringLiteral("带宽"), m_bwCombo));
    row1Lay->addStretch();
    basicLay->addWidget(row1);

    auto *row2 = createParamRow();
    auto *row2Lay = qobject_cast<QHBoxLayout *>(row2->layout());

    m_scsCombo = new QComboBox;
    m_scsCombo->addItems({QStringLiteral("15 kHz (μ=0)"), QStringLiteral("30 kHz (μ=1)"),
                          QStringLiteral("60 kHz (μ=2)"), QStringLiteral("120 kHz (μ=3)"),
                          QStringLiteral("240 kHz (μ=4)")});
    m_scsCombo->setCurrentIndex(3);
    row2Lay->addWidget(makeParamItem(QStringLiteral("子载波间隔 (μ)"), m_scsCombo));

    m_duplexCombo = new QComboBox;
    m_duplexCombo->addItems({QStringLiteral("FDD (频分双工)"),
                             QStringLiteral("TDD (时分双工)")});
    row2Lay->addWidget(makeParamItem(QStringLiteral("双工模式"), m_duplexCombo));

    m_frameCombo = new QComboBox;
    m_frameCombo->addItems({QStringLiteral("标准 (Standard)"),
                            QStringLiteral("自定义 (Custom)")});
    row2Lay->addWidget(makeParamItem(QStringLiteral("帧结构"), m_frameCombo));
    row2Lay->addStretch();
    basicLay->addWidget(row2);
    lay->addWidget(basic);

    // 物理层参数
    auto *phy = createParamSection(QStringLiteral("🧩  物理层参数"), QString(), QString());
    auto *phyLay = qobject_cast<QVBoxLayout *>(phy->layout());

    auto *row3 = createParamRow();
    auto *row3Lay = qobject_cast<QHBoxLayout *>(row3->layout());

    m_cpCombo = new QComboBox;
    m_cpCombo->addItems({QStringLiteral("正常 (Normal)"), QStringLiteral("扩展 (Extended)")});
    row3Lay->addWidget(makeParamItem(QStringLiteral("CP 类型"), m_cpCombo));

    m_ssbCombo = new QComboBox;
    m_ssbCombo->addItems({QStringLiteral("启用"), QStringLiteral("禁用")});
    row3Lay->addWidget(makeParamItem(QStringLiteral("SS/PBCH 块"), m_ssbCombo));

    m_csiRsCombo = new QComboBox;
    m_csiRsCombo->addItems({QStringLiteral("启用"), QStringLiteral("禁用")});
    row3Lay->addWidget(makeParamItem(QStringLiteral("CSI-RS"), m_csiRsCombo));
    row3Lay->addStretch();
    phyLay->addWidget(row3);
    lay->addWidget(phy);

    lay->addStretch();
    return page;
}

// ====== BWP 页 ======

QWidget *FiveGNrWidget::createBwpPage() {
    auto *page = new QWidget;
    auto *lay = new QVBoxLayout(page);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(16);

    auto *section = createParamSection(QStringLiteral("＋  下行 BWP (DL-BWP)"),
                                       QStringLiteral("BWP"), QStringLiteral("bwp"));
    auto *sectionLay = qobject_cast<QVBoxLayout *>(section->layout());

    // DL-BWP 容器
    auto *dlBwp = new QWidget;
    dlBwp->setObjectName(QStringLiteral("fgBwpContainer"));
    auto *dlLay = new QVBoxLayout(dlBwp);
    dlLay->setContentsMargins(14, 12, 14, 12);
    dlLay->setSpacing(10);

    auto *dlHeader = new QHBoxLayout;
    auto *dlTitle = new QLabel(QStringLiteral("↓  DL-BWP 0  (初始)"));
    dlTitle->setObjectName(QStringLiteral("fgBwpTitle"));
    dlHeader->addWidget(dlTitle);
    dlHeader->addStretch();

    // BWP 操作按钮（复制 / 删除）
    auto *dlCopyBtn = new QPushButton(QStringLiteral("📋  复制"));
    dlCopyBtn->setObjectName(QStringLiteral("fgBwpActionBtn"));
    dlCopyBtn->setCursor(Qt::PointingHandCursor);
    dlHeader->addWidget(dlCopyBtn);
    auto *dlDelBtn = new QPushButton(QStringLiteral("🗑  删除"));
    dlDelBtn->setObjectName(QStringLiteral("fgBwpActionBtn"));
    dlDelBtn->setCursor(Qt::PointingHandCursor);
    dlHeader->addWidget(dlDelBtn);
    dlLay->addLayout(dlHeader);

    auto *dlParams = new QHBoxLayout;
    dlParams->setSpacing(20);
    auto *dlId = new QLineEdit(QStringLiteral("0"));
    dlId->setReadOnly(true);
    dlParams->addWidget(makeParamItem(QStringLiteral("BWP ID"), dlId));
    auto *dlName = new QLineEdit(QStringLiteral("Initial-DL-BWP"));
    dlParams->addWidget(makeParamItem(QStringLiteral("名称"), dlName));
    auto *dlNum = new QComboBox;
    dlNum->addItems({QStringLiteral("μ=3 (120kHz)"), QStringLiteral("μ=4 (240kHz)")});
    dlParams->addWidget(makeParamItem(QStringLiteral("Numerology"), dlNum));
    auto *dlRbNum = new QLineEdit(QStringLiteral("66"));
    dlParams->addWidget(makeParamItem(QStringLiteral("RB Number"), dlRbNum,
                                      QStringLiteral("RB")));
    dlParams->addStretch();
    dlLay->addLayout(dlParams);

    // CORESET 子层级
    auto *coreset = new QWidget;
    coreset->setObjectName(QStringLiteral("fgCoresetContainer"));
    auto *csLay = new QVBoxLayout(coreset);
    csLay->setContentsMargins(12, 10, 12, 10);
    csLay->setSpacing(8);

    auto *csHeader = new QHBoxLayout;
    auto *csTitle = new QLabel(QStringLiteral("🎯  CORESET 0  (控制资源集)"));
    csTitle->setObjectName(QStringLiteral("fgCoresetTitle"));
    csHeader->addWidget(csTitle);
    csHeader->addStretch();

    // CORESET 添加按钮
    auto *csAddBtn = new QPushButton(QStringLiteral("＋  添加"));
    csAddBtn->setObjectName(QStringLiteral("fgCoresetAddBtn"));
    csAddBtn->setCursor(Qt::PointingHandCursor);
    csHeader->addWidget(csAddBtn);
    csLay->addLayout(csHeader);

    auto *csParams = new QHBoxLayout;
    csParams->setSpacing(18);
    auto *csId = new QLineEdit(QStringLiteral("0"));
    csId->setReadOnly(true);
    csParams->addWidget(makeParamItem(QStringLiteral("ID"), csId));
    auto *csFreq = new QLineEdit(QStringLiteral("48"));
    csParams->addWidget(makeParamItem(QStringLiteral("频域资源"), csFreq,
                                      QStringLiteral("RB")));
    auto *csLen = new QComboBox;
    csLen->addItems({QStringLiteral("1 symbol"), QStringLiteral("2 symbols"),
                     QStringLiteral("3 symbols")});
    csParams->addWidget(makeParamItem(QStringLiteral("时域长度"), csLen));
    csParams->addStretch();
    csLay->addLayout(csParams);
    dlLay->addWidget(coreset);
    sectionLay->addWidget(dlBwp);
    lay->addWidget(section);

    // UL-BWP 容器
    auto *ulSection = createParamSection(QStringLiteral("＋  上行 BWP (UL-BWP)"),
                                         QStringLiteral("BWP"), QStringLiteral("bwp"));
    auto *ulSectionLay = qobject_cast<QVBoxLayout *>(ulSection->layout());

    auto *ulBwp = new QWidget;
    ulBwp->setObjectName(QStringLiteral("fgBwpContainerUl"));
    auto *ulLay = new QVBoxLayout(ulBwp);
    ulLay->setContentsMargins(14, 12, 14, 12);
    ulLay->setSpacing(10);

    auto *ulHeader = new QHBoxLayout;
    auto *ulTitle = new QLabel(QStringLiteral("↑  UL-BWP 0  (上行初始)"));
    ulTitle->setObjectName(QStringLiteral("fgBwpTitleUl"));
    ulHeader->addWidget(ulTitle);
    ulHeader->addStretch();

    auto *ulCopyBtn = new QPushButton(QStringLiteral("📋  复制"));
    ulCopyBtn->setObjectName(QStringLiteral("fgBwpActionBtn"));
    ulCopyBtn->setCursor(Qt::PointingHandCursor);
    ulHeader->addWidget(ulCopyBtn);
    auto *ulDelBtn = new QPushButton(QStringLiteral("🗑  删除"));
    ulDelBtn->setObjectName(QStringLiteral("fgBwpActionBtn"));
    ulDelBtn->setCursor(Qt::PointingHandCursor);
    ulHeader->addWidget(ulDelBtn);
    ulLay->addLayout(ulHeader);

    auto *ulParams = new QHBoxLayout;
    ulParams->setSpacing(20);
    auto *ulId = new QLineEdit(QStringLiteral("0"));
    ulId->setReadOnly(true);
    ulParams->addWidget(makeParamItem(QStringLiteral("BWP ID"), ulId));
    auto *ulName = new QLineEdit(QStringLiteral("Initial-UL-BWP"));
    ulParams->addWidget(makeParamItem(QStringLiteral("名称"), ulName));
    auto *ulNum = new QComboBox;
    ulNum->addItems({QStringLiteral("μ=3 (120kHz)"), QStringLiteral("μ=4 (240kHz)")});
    ulParams->addWidget(makeParamItem(QStringLiteral("Numerology"), ulNum));
    auto *ulRbNum = new QLineEdit(QStringLiteral("66"));
    ulParams->addWidget(makeParamItem(QStringLiteral("RB Number"), ulRbNum,
                                      QStringLiteral("RB")));
    ulParams->addStretch();
    ulLay->addLayout(ulParams);
    ulSectionLay->addWidget(ulBwp);
    lay->addWidget(ulSection);

    auto *note = new QLabel(QStringLiteral("ℹ  BWP 配置遵循 3GPP 38.211 规范。每个 BWP 有独立的 Numerology 和 RB 分配。"));
    note->setObjectName(QStringLiteral("fgNote"));
    note->setWordWrap(true);
    lay->addWidget(note);

    lay->addStretch();
    return page;
}

// ====== 用户页 ======

QWidget *FiveGNrWidget::createUserPage() {
    auto *page = new QWidget;
    auto *lay = new QVBoxLayout(page);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(16);

    // 用户参数
    auto *user = createParamSection(QStringLiteral("👤  用户参数"), QStringLiteral("UE"),
                                    QStringLiteral("user"));
    auto *userLay = qobject_cast<QVBoxLayout *>(user->layout());
    auto *row1 = createParamRow();
    auto *row1Lay = qobject_cast<QHBoxLayout *>(row1->layout());

    m_ueIdEdit = new QLineEdit(QStringLiteral("UE-1"));
    row1Lay->addWidget(makeParamItem(QStringLiteral("用户 ID"), m_ueIdEdit));
    m_rntiEdit = new QLineEdit(QStringLiteral("0x1234"));
    row1Lay->addWidget(makeParamItem(QStringLiteral("RNTI"), m_rntiEdit));
    m_ueCountCombo = new QComboBox;
    m_ueCountCombo->addItems({QStringLiteral("1"), QStringLiteral("2"),
                              QStringLiteral("4"), QStringLiteral("8")});
    row1Lay->addWidget(makeParamItem(QStringLiteral("用户数量"), m_ueCountCombo));
    m_multiUserCombo = new QComboBox;
    m_multiUserCombo->addItems({QStringLiteral("单用户"), QStringLiteral("MU-MIMO")});
    row1Lay->addWidget(makeParamItem(QStringLiteral("多用户模式"), m_multiUserCombo));
    row1Lay->addStretch();
    userLay->addWidget(row1);
    lay->addWidget(user);

    // PDSCH
    auto *pdsch = createParamSection(QStringLiteral("⇅  PDSCH 配置 (下行)"), QString(), QString());
    auto *pdschLay = qobject_cast<QVBoxLayout *>(pdsch->layout());
    auto *row2 = createParamRow();
    auto *row2Lay = qobject_cast<QHBoxLayout *>(row2->layout());

    m_pdschModCombo = new QComboBox;
    m_pdschModCombo->addItems({QStringLiteral("QPSK"), QStringLiteral("16QAM"),
                               QStringLiteral("64QAM"), QStringLiteral("256QAM")});
    m_pdschModCombo->setCurrentIndex(3);
    row2Lay->addWidget(makeParamItem(QStringLiteral("调制方式"), m_pdschModCombo));

    m_pdschMcsSpin = new QSpinBox;
    m_pdschMcsSpin->setRange(0, 27);
    m_pdschMcsSpin->setValue(27);
    row2Lay->addWidget(makeParamItem(QStringLiteral("MCS 索引"), m_pdschMcsSpin));

    m_pdschRateValue = new QLabel(QStringLiteral("0.925"));
    m_pdschRateValue->setObjectName(QStringLiteral("fgValueDisplay"));
    row2Lay->addWidget(makeParamItem(QStringLiteral("目标码率"), m_pdschRateValue));

    m_pdschTbsValue = new QLabel(QStringLiteral("122976"));
    m_pdschTbsValue->setObjectName(QStringLiteral("fgValueDisplay"));
    row2Lay->addWidget(makeParamItem(QStringLiteral("传输块大小"), m_pdschTbsValue));
    row2Lay->addStretch();
    pdschLay->addWidget(row2);

    auto *row3 = createParamRow();
    auto *row3Lay = qobject_cast<QHBoxLayout *>(row3->layout());
    m_pdschBwpCombo = new QComboBox;
    m_pdschBwpCombo->addItems({QStringLiteral("DL-BWP 0"), QStringLiteral("DL-BWP 1")});
    row3Lay->addWidget(makeParamItem(QStringLiteral("BWP 关联"), m_pdschBwpCombo));
    m_pdschPortCombo = new QComboBox;
    m_pdschPortCombo->addItems({QStringLiteral("1"), QStringLiteral("2"),
                                QStringLiteral("4")});
    row3Lay->addWidget(makeParamItem(QStringLiteral("天线端口"), m_pdschPortCombo));
    row3Lay->addStretch();
    pdschLay->addWidget(row3);
    lay->addWidget(pdsch);

    // PUSCH
    auto *pusch = createParamSection(QStringLiteral("⇅  PUSCH 配置 (上行)"), QString(), QString());
    auto *puschLay = qobject_cast<QVBoxLayout *>(pusch->layout());
    auto *row4 = createParamRow();
    auto *row4Lay = qobject_cast<QHBoxLayout *>(row4->layout());

    m_puschModCombo = new QComboBox;
    m_puschModCombo->addItems({QStringLiteral("QPSK"), QStringLiteral("16QAM"),
                               QStringLiteral("64QAM"), QStringLiteral("256QAM")});
    m_puschModCombo->setCurrentIndex(1);
    row4Lay->addWidget(makeParamItem(QStringLiteral("调制方式"), m_puschModCombo));

    m_puschMcsSpin = new QSpinBox;
    m_puschMcsSpin->setRange(0, 27);
    m_puschMcsSpin->setValue(15);
    row4Lay->addWidget(makeParamItem(QStringLiteral("MCS 索引"), m_puschMcsSpin));

    m_puschBwpCombo = new QComboBox;
    m_puschBwpCombo->addItems({QStringLiteral("UL-BWP 0"), QStringLiteral("UL-BWP 1")});
    row4Lay->addWidget(makeParamItem(QStringLiteral("BWP 关联"), m_puschBwpCombo));
    row4Lay->addStretch();
    puschLay->addWidget(row4);
    lay->addWidget(pusch);

    lay->addStretch();
    return page;
}

// ====== 路由页 ======

QWidget *FiveGNrWidget::createRoutingPage() {
    auto *page = new QWidget;
    auto *lay = new QVBoxLayout(page);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(16);

    auto *routing = createParamSection(QStringLiteral("🧭  输出路由"), QString(), QString());
    auto *routingLay = qobject_cast<QVBoxLayout *>(routing->layout());
    auto *row1 = createParamRow();
    auto *row1Lay = qobject_cast<QHBoxLayout *>(row1->layout());

    m_rfPortCombo = new QComboBox;
    m_rfPortCombo->addItems({QStringLiteral("RF1"), QStringLiteral("RF2"),
                             QStringLiteral("RF1+RF2")});
    row1Lay->addWidget(makeParamItem(QStringLiteral("RF 输出端口"), m_rfPortCombo));

    m_antennaCombo = new QComboBox;
    m_antennaCombo->addItems({QStringLiteral("单天线"), QStringLiteral("1x2"),
                              QStringLiteral("2x2"), QStringLiteral("4x4")});
    row1Lay->addWidget(makeParamItem(QStringLiteral("天线映射"), m_antennaCombo));

    m_beamCombo = new QComboBox;
    m_beamCombo->addItems({QStringLiteral("禁用"), QStringLiteral("启用")});
    row1Lay->addWidget(makeParamItem(QStringLiteral("波束成形"), m_beamCombo));
    row1Lay->addStretch();
    routingLay->addWidget(row1);

    auto *row2 = createParamRow();
    auto *row2Lay = qobject_cast<QHBoxLayout *>(row2->layout());
    m_mimoCombo = new QComboBox;
    m_mimoCombo->addItems({QStringLiteral("1"), QStringLiteral("2"), QStringLiteral("4")});
    row2Lay->addWidget(makeParamItem(QStringLiteral("MIMO 通道"), m_mimoCombo));
    m_precodingCombo = new QComboBox;
    m_precodingCombo->addItems({QStringLiteral("None"), QStringLiteral("Codebook"),
                                QStringLiteral("Non-codebook")});
    row2Lay->addWidget(makeParamItem(QStringLiteral("预编码"), m_precodingCombo));
    row2Lay->addStretch();
    routingLay->addWidget(row2);
    lay->addWidget(routing);

    // 路由摘要
    auto *summary = new QWidget;
    summary->setObjectName(QStringLiteral("fgRouteSummary"));
    auto *sumLay = new QHBoxLayout(summary);
    sumLay->setContentsMargins(14, 12, 14, 12);
    sumLay->setSpacing(16);
    auto *sumText = new QLabel(
        QStringLiteral("✅  RF1: 28.000 GHz · -10 dBm     ✅  RF2: 28.000 GHz · -10 dBm     "
                       "ℹ  相位相干: 已启用"));
    sumText->setObjectName(QStringLiteral("fgRouteSummaryText"));
    sumLay->addWidget(sumText);
    sumLay->addStretch();
    lay->addWidget(summary);

    lay->addStretch();
    return page;
}

// ====== 信号与交互连接 ======

void FiveGNrWidget::setupConnections() {
    // 左导航切换
    for (auto *nav : m_navBtns) {
        connect(nav, &QPushButton::clicked, this, &FiveGNrWidget::onNavClicked);
    }

    // Apps 切换
    for (auto *app : m_appBtns) {
        connect(app, &QPushButton::clicked, this, &FiveGNrWidget::onAppClicked);
    }

    // RF 开关
    if (m_rfToggleBtn) {
        connect(m_rfToggleBtn, &QPushButton::clicked, this, &FiveGNrWidget::onRfToggle);
    }

    // 载波页参数变更 → 更新右侧状态
    if (m_freqRangeCombo) {
        connect(m_freqRangeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &FiveGNrWidget::onCarrierParamChanged);
    }
    if (m_carrierFreqEdit) {
        connect(m_carrierFreqEdit, &QLineEdit::textChanged,
                this, &FiveGNrWidget::onCarrierParamChanged);
    }
    if (m_bwCombo) {
        connect(m_bwCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &FiveGNrWidget::onCarrierParamChanged);
    }
    if (m_scsCombo) {
        connect(m_scsCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &FiveGNrWidget::onCarrierParamChanged);
    }
}

// ====== 槽函数实现 ======

void FiveGNrWidget::onNavClicked() {
    auto *btn = qobject_cast<QPushButton *>(sender());
    if (!btn) return;
    setActiveNav(btn);

    const int idx = btn->property("navIndex").toInt();
    if (m_configStack) {
        m_configStack->setCurrentIndex(idx);
    }

    // 更新面板标题 / 副标题 / 层级徽标 / 描述
    const QStringList titles = {
        QStringLiteral("载波配置 (Cell)"),
        QStringLiteral("BWP 配置 (带宽部分)"),
        QStringLiteral("用户配置 (UE)"),
        QStringLiteral("路由配置")
    };
    const QStringList subs = {
        QStringLiteral("— 设置 5G 小区的基础帧结构和频率参数"),
        QStringLiteral("— 配置 UE 专属的带宽部分 (Bandwidth Part) 参数"),
        QStringLiteral("— 配置终端用户 (UE) 专属参数"),
        QStringLiteral("— 设置射频输出路由和 MIMO 映射")
    };
    const QStringList badges = {
        QStringLiteral("🧊  Cell"),
        QStringLiteral("🗂  BWP"),
        QStringLiteral("👤  User"),
        QStringLiteral("🧭  Route")
    };
    const QStringList descs = {
        QStringLiteral("配置频率范围 (FR1/FR2)、载波频率、带宽、子载波间隔和双工模式。"
                       "这些参数定义了 5G NR 信号的“骨架”。"),
        QStringLiteral("带宽部分 (BWP) 是 UE 专属的资源配置，支持在一个载波内配置多个 "
                       "BWP 并混合使用不同的子载波间隔 (Numerology)。"),
        QStringLiteral("配置 PDSCH/PUSCH 的多用户参数、RNTI、调制编码方案 (MCS) 和传输块大小。"
                       "支持多用户 PUSCH/PDSCH 生成。"),
        QStringLiteral("配置 RF 输出端口、天线映射、波束成形和 MIMO 通道参数。")
    };
    if (m_panelTitle) m_panelTitle->setText(titles.at(idx));
    if (m_panelSub) m_panelSub->setText(subs.at(idx));
    if (m_panelBadge) m_panelBadge->setText(badges.at(idx));
    if (m_panelDesc) m_panelDesc->setText(descs.at(idx));
}

void FiveGNrWidget::onAppClicked() {
    auto *btn = qobject_cast<QPushButton *>(sender());
    if (!btn) return;
    for (auto *b : m_appBtns) {
        b->setChecked(b == btn);
    }
    // 仅 5G NR 有完整界面，其它 App 提示未接入
    if (btn->property("appIndex").toInt() != 0) {
        m_panelDesc->setText(QStringLiteral("该应用尚未接入当前界面。"));
    }
}

void FiveGNrWidget::onRfToggle() {
    if (!m_rfToggleBtn) return;
    m_rfEnabled = m_rfToggleBtn->isChecked();
    if (m_rfEnabled) {
        m_rfToggleBtn->setText(QStringLiteral("●  开启"));
        m_footerStatus->setTextFormat(Qt::RichText);
        m_footerStatus->setText(
            QStringLiteral("<span style='color:%1;'>●</span>  就绪").arg(kGreen));
    } else {
        m_rfToggleBtn->setText(QStringLiteral("●  关闭"));
        m_footerStatus->setTextFormat(Qt::RichText);
        m_footerStatus->setText(
            QStringLiteral("<span style='color:%1;'>○</span>  RF 关闭").arg(kSub));
    }
    updateSignalStatus();
}

void FiveGNrWidget::onPresetClicked() {
    auto *btn = qobject_cast<QPushButton *>(sender());
    if (!btn) return;
    // 更新按钮选中态
    for (auto *w : this->findChildren<QPushButton *>()) {
        if (w->property("preset").isValid()) {
            w->setChecked(w == btn);
        }
    }
    applyPreset(btn->property("preset").toString());
}

void FiveGNrWidget::onGenerateClicked() {
    auto *btn = qobject_cast<QPushButton *>(sender());
    if (!btn) return;
    const QString original = btn->text();
    btn->setText(QStringLiteral("⏳  生成中…"));
    btn->setEnabled(false);
    // 模拟生成耗时
    QTimer::singleShot(1200, this, [btn, original]() {
        btn->setText(QStringLiteral("✅  生成完成!"));
        btn->setEnabled(true);
        QTimer::singleShot(1500, btn, [btn, original]() {
            btn->setText(original);
        });
    });
    spdlog::info("FiveGNrWidget: 生成波形");
}

void FiveGNrWidget::onPreviewClicked() {
    WaveformData data;
    data.generateSine(2.0, 1.0, 100.0, 4, 0.0, 0.0);
    if (m_previewChart) m_previewChart->setData(data);
    spdlog::info("FiveGNrWidget: 预览波形");
}

void FiveGNrWidget::onSavePresetClicked() {
    spdlog::info("FiveGNrWidget: 保存预设");
}

void FiveGNrWidget::onCarrierParamChanged() {
    updateSignalStatus();
}

// ====== 辅助 ======

void FiveGNrWidget::setActiveNav(QPushButton *nav) {
    for (int i = 0; i < m_navBtns.size(); ++i) {
        const bool active = (m_navBtns.at(i) == nav);
        m_navBtns.at(i)->setChecked(active);
        if (i < m_navBadges.size()) {
            // 选中时徽标变蓝（对应设计稿 .nav-item.active .step-badge）
            m_navBadges.at(i)->setProperty("active", active);
            m_navBadges.at(i)->style()->unpolish(m_navBadges.at(i));
            m_navBadges.at(i)->style()->polish(m_navBadges.at(i));
        }
    }
}

void FiveGNrWidget::updateSignalStatus() {
    if (!m_statFrVal) return;

    const QString fr = (m_freqRangeCombo && m_freqRangeCombo->currentIndex() == 1)
                           ? QStringLiteral("FR2 (mmWave)")
                           : QStringLiteral("FR1 (Sub-6 GHz)");
    m_statFrVal->setText(fr);

    const QString freq = m_carrierFreqEdit ? m_carrierFreqEdit->text() : QString::fromUtf8(kDefaultFreq);
    m_statFreqVal->setText(QStringLiteral("%1 GHz").arg(freq));

    const QString bw = m_bwCombo ? m_bwCombo->currentText() : QStringLiteral("800 MHz");
    m_statBwVal->setText(bw);

    const QString scs = m_scsCombo ? m_scsCombo->currentText() : QStringLiteral("120 kHz (μ=3)");
    m_statScsVal->setText(scs);

    // 信号状态：LED 圆点 + 文字（对应设计稿 .led + 文字）
    m_statStatusVal->setTextFormat(Qt::RichText);
    m_statStatusVal->setText(m_rfEnabled
        ? QStringLiteral("<span style='color:%1;'>●</span>  就绪").arg(kGreen)
        : QStringLiteral("<span style='color:%1;'>○</span>  关闭").arg(kSub));

    // 配置层级
    if (m_hierCellVal) {
        m_hierCellVal->setText(QStringLiteral("%1 · %2 · %3")
            .arg(fr).arg(bw).arg(scs));
    }
    if (m_hierBwpVal) {
        m_hierBwpVal->setText(QStringLiteral("ID:0 · 66 RB · μ=3"));
    }
    if (m_hierCoresetVal) {
        m_hierCoresetVal->setText(QStringLiteral("ID:0 · 48 RB"));
    }
    if (m_hierUserVal) {
        m_hierUserVal->setText(QStringLiteral("UE-1 · RNTI=0x1234"));
    }

    // 底部状态栏
    if (m_footerWave) {
        m_footerWave->setText(QStringLiteral("波形: 5G_NR_%1_%2").arg(
            fr.startsWith(QStringLiteral("FR2")) ? QStringLiteral("FR2") : QStringLiteral("FR1"),
            bw.simplified().remove(QStringLiteral(" "))));
    }
}

void FiveGNrWidget::applyPreset(const QString &preset) {
    if (!m_bwCombo || !m_scsCombo || !m_freqRangeCombo) return;

    if (preset.startsWith(QStringLiteral("FR1"))) {
        m_freqRangeCombo->setCurrentIndex(0);
        m_bwCombo->setCurrentText(QStringLiteral("100 MHz"));
        m_scsCombo->setCurrentText(QStringLiteral("30 kHz (μ=1)"));
        m_carrierFreqEdit->setText(QStringLiteral("3.500"));
    } else if (preset == QStringLiteral("FR2 400MHz")) {
        m_freqRangeCombo->setCurrentIndex(1);
        m_bwCombo->setCurrentText(QStringLiteral("400 MHz"));
        m_scsCombo->setCurrentText(QStringLiteral("120 kHz (μ=3)"));
        m_carrierFreqEdit->setText(QStringLiteral("28.000"));
    } else if (preset == QStringLiteral("FR2 800MHz")) {
        m_freqRangeCombo->setCurrentIndex(1);
        m_bwCombo->setCurrentText(QStringLiteral("800 MHz"));
        m_scsCombo->setCurrentText(QStringLiteral("120 kHz (μ=3)"));
        m_carrierFreqEdit->setText(QStringLiteral("28.000"));
    } else if (preset == QStringLiteral("FR2 1600MHz")) {
        m_freqRangeCombo->setCurrentIndex(1);
        m_bwCombo->setCurrentText(QStringLiteral("1600 MHz"));
        m_scsCombo->setCurrentText(QStringLiteral("240 kHz (μ=4)"));
        m_carrierFreqEdit->setText(QStringLiteral("28.000"));
    }
    // 自定义不改变默认值
    updateSignalStatus();
    spdlog::info("FiveGNrWidget: 应用预设 {}", preset.toStdString());
}
