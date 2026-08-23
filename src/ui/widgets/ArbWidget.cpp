#include "ArbWidget.h"
#include "FileManagerDialog.h"

#include <QCheckBox>
#include <QColor>
#include <QComboBox>
#include <QDateTime>
#include <QDebug>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFileDialog>
#include <QFont>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLineEdit>
#include <QList>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QRegularExpression>
#include <QSplitter>
#include <QStackedWidget>
#include <QSpinBox>
#include <QTabBar>
#include <QTableWidget>
#include <QTabWidget>
#include <QTimer>
#include <QVBoxLayout>

#include <spdlog/spdlog.h>

#include <QFileInfo>
#include <QDir>

namespace {
constexpr double kMinFrequency = 1.0;    // GHz
constexpr double kMaxFrequency = 40.0;   // GHz
constexpr double kMinPower = -144.0;     // dBm
constexpr double kMaxPower = 13.0;       // dBm
constexpr double kMinSampleRate = 1.0;   // MHz
constexpr double kMaxSampleRate = 1000.0;// MHz
} // namespace

ArbWidget::ArbWidget(QWidget *parent) : QWidget(parent) {
    loadStyleSheet();
    setupUI();
    setupConnections();

    // 播放引擎初始化：定时器 + 列表数据
    m_playTimer = new QTimer(this);
    m_playTimer->setInterval(100);   // 100ms 推进一次
    connect(m_playTimer, &QTimer::timeout, this, &ArbWidget::onTimerTick);
    initPlaylist();
    renderPlaylist();
    startPlayback();   // 默认进入播放态，复刻设计稿“播放中”状态

    updateHardwareStatus();
    spdlog::info("ArbWidget initialized");
}

ArbWidget::~ArbWidget() {
    spdlog::info("ArbWidget destroyed");
}

// ====== 样式加载 ======

void ArbWidget::loadStyleSheet() {
    QFile f(QStringLiteral(":/arb_widget.qss"));
    if (f.open(QFile::ReadOnly | QFile::Text)) {
        setStyleSheet(QString::fromUtf8(f.readAll()));
    } else {
        spdlog::warn("ArbWidget: failed to load :/arb_widget.qss");
    }
}

// ====== 主布局：导航栏 + 主体(左右分栏) + 底部状态栏（标题栏由全局 AppHeaderBar 提供） ======

void ArbWidget::setupUI() {
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // 主体：左导航 / 中央标签页 / 右侧前面板·侧边栏
    auto *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setObjectName(QStringLiteral("arbSplitter"));
    splitter->setChildrenCollapsible(false);
    root->addWidget(splitter, 1);

    // 左侧：导航栏（载波/波形/硬件/播放，对齐 5GNR 左侧导航样式）
    m_navPanel = createNavPanel();
    m_navPanel->setMinimumWidth(200);
    splitter->addWidget(m_navPanel);

    // 中央：标签页容器（页签条隐藏，由左侧导航栏切换）
    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setObjectName(QStringLiteral("arbTabWidget"));
    m_tabWidget->tabBar()->hide();
    splitter->addWidget(m_tabWidget);

    // 载波页容器
    m_carrierStack = new QStackedWidget(this);

    // 创建四个标签页
    setupCarrierTab();
    setupWaveformTab();
    setupHardwareTab();
    setupPlaybackTab();

    // 右侧：前面板（仅载波页显示）
    m_frontPanel = createFrontPanel();
    m_frontPanel->setMinimumWidth(300);
    splitter->addWidget(m_frontPanel);

    // 分割器初始比例（导航 0 : 中央 2 : 右侧 1.2）
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 2);
    splitter->setStretchFactor(2, 1);
    splitter->setSizes({200, 560, 300});

    // 底部状态栏
    m_footer = createFooter();
    root->addWidget(m_footer);
}

// ====== 左侧导航栏 ======

QWidget *ArbWidget::createNavPanel() {
    auto *nav = new QWidget;
    nav->setObjectName(QStringLiteral("arbNavPanel"));
    auto *lay = new QVBoxLayout(nav);
    lay->setContentsMargins(0, 12, 0, 12);
    lay->setSpacing(2);

    // 信号配置小节
    auto *secHeader = new QLabel(QStringLiteral("信号配置"));
    secHeader->setObjectName(QStringLiteral("arbNavHeader"));
    lay->addWidget(secHeader);

    struct NavEntry { const char *icon; const char *label; };
    const NavEntry navs[] = {
        {"📡", "载波 (Carrier)"},
        {"〰", "波形 (Waveform)"},
        {"🖥", "硬件 (Hardware)"},
        {"▶", "播放 (Playback)"}
    };
    const QStringList navSteps = {QStringLiteral("1"), QStringLiteral("2"),
                                  QStringLiteral("3"), QStringLiteral("4")};
    m_navBtns.clear();
    for (int i = 0; i < 4; ++i) {
        auto *btn = new QPushButton(QString::fromUtf8(navs[i].icon) + QStringLiteral("  ") +
                                    QString::fromUtf8(navs[i].label));
        btn->setObjectName(QStringLiteral("arbNavItem"));
        btn->setCheckable(true);
        btn->setChecked(i == 0);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setProperty("tabIndex", i);

        // 步骤徽标放在最右
        auto *stepBadge = new QLabel(navSteps.at(i));
        stepBadge->setObjectName(QStringLiteral("arbNavBadge"));
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
        connect(btn, &QPushButton::clicked, this, [this, i]() {
            if (m_tabWidget) m_tabWidget->setCurrentIndex(i);
        });
    }

    // 分隔线
    auto *divider = new QFrame;
    divider->setObjectName(QStringLiteral("arbNavDivider"));
    lay->addWidget(divider);

    // 工具小节
    auto *toolHeader = new QLabel(QStringLiteral("工具"));
    toolHeader->setObjectName(QStringLiteral("arbNavHeader"));
    lay->addWidget(toolHeader);

    const QStringList tools = {
        QStringLiteral("📥  导入波形"),
        QStringLiteral("💾  保存预设"),
        QStringLiteral("📤  导出设置")
    };
    for (const QString &t : tools) {
        auto *btn = new QPushButton(t);
        btn->setObjectName(QStringLiteral("arbNavToolItem"));
        btn->setCursor(Qt::PointingHandCursor);
        lay->addWidget(btn);
    }

    lay->addStretch();
    return nav;
}

// ====== 底部状态栏 ======

QWidget *ArbWidget::createFooter() {
    auto *footer = new QWidget;
    footer->setObjectName(QStringLiteral("arbFooter"));
    auto *lay = new QHBoxLayout(footer);
    lay->setContentsMargins(24, 6, 24, 6);
    lay->setSpacing(12);

    // 左侧：播放状态 + 当前波形（对齐 Playback.html footer）
    m_footerStatus = new QLabel(QStringLiteral("● 播放中"));
    m_footerStatus->setStyleSheet(QStringLiteral("color: #5a6a7a; font-size: 12px;"));
    lay->addWidget(m_footerStatus);

    auto *sep1 = new QLabel(QStringLiteral("|"));
    sep1->setStyleSheet(QStringLiteral("color: #cdd3db;"));
    lay->addWidget(sep1);

    m_footerWave = new QLabel(QStringLiteral("波形: WLAN_11n_HT20_5G"));
    m_footerWave->setStyleSheet(QStringLiteral("color: #5a6a7a; font-size: 12px;"));
    lay->addWidget(m_footerWave);

    auto *sep2 = new QLabel(QStringLiteral("|"));
    sep2->setStyleSheet(QStringLiteral("color: #cdd3db;"));
    lay->addWidget(sep2);

    m_footerMem = new QLabel(QStringLiteral("内存占用: 12%"));
    m_footerMem->setStyleSheet(QStringLiteral("color: #5a6a7a; font-size: 12px;"));
    lay->addWidget(m_footerMem);

    auto *sep3 = new QLabel(QStringLiteral("|"));
    sep3->setStyleSheet(QStringLiteral("color: #cdd3db;"));
    lay->addWidget(sep3);

    m_footerSr = new QLabel(QStringLiteral("波形采样率: 80 MHz"));
    m_footerSr->setStyleSheet(QStringLiteral("color: #5a6a7a; font-size: 12px;"));
    lay->addWidget(m_footerSr);

    lay->addStretch();

    auto *fwLabel = new QLabel(QStringLiteral("固件 v3.2.1"));
    fwLabel->setStyleSheet(QStringLiteral("color: #5a6a7a; font-size: 12px;"));
    lay->addWidget(fwLabel);

    m_footerTime = new QLabel(QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm")));
    m_footerTime->setStyleSheet(QStringLiteral("color: #5a6a7a; font-size: 12px;"));
    lay->addWidget(m_footerTime);

    return footer;
}

void ArbWidget::setupConnections() {
    // 标签页切换
    if (m_tabWidget) {
        connect(m_tabWidget, &QTabWidget::currentChanged,
                this, &ArbWidget::onTabChanged);
    }

    // 载波页：应用/停止/浏览/下载
    if (m_applyBtn) {
        connect(m_applyBtn, &QPushButton::clicked,
                this, &ArbWidget::onApplyAndPlay);
    }
    if (m_stopBtn) {
        connect(m_stopBtn, &QPushButton::clicked,
                this, &ArbWidget::onStop);
    }
    if (m_browseBtn) {
        connect(m_browseBtn, &QPushButton::clicked,
                this, &ArbWidget::onBrowseWaveform);
    }
    if (m_downloadBtn) {
        connect(m_downloadBtn, &QPushButton::clicked,
                this, &ArbWidget::onDownloadWaveform);
    }

    // 载波页参数调整按钮（频率/功率上下、采样率上下）
    if (m_upFreqBtn) {
        connect(m_upFreqBtn, &QPushButton::clicked,
                this, &ArbWidget::onFreqUp);
    }
    if (m_downFreqBtn) {
        connect(m_downFreqBtn, &QPushButton::clicked,
                this, &ArbWidget::onFreqDown);
    }
    if (m_upPowerBtn) {
        connect(m_upPowerBtn, &QPushButton::clicked,
                this, &ArbWidget::onPowerUp);
    }
    if (m_downPowerBtn) {
        connect(m_downPowerBtn, &QPushButton::clicked,
                this, &ArbWidget::onPowerDown);
    }
    if (m_upSampleRateBtn) {
        connect(m_upSampleRateBtn, &QPushButton::clicked,
                this, &ArbWidget::onSampleRateUp);
    }
    if (m_downSampleRateBtn) {
        connect(m_downSampleRateBtn, &QPushButton::clicked,
                this, &ArbWidget::onSampleRateDown);
    }

    // 前面板硬件按钮
    if (m_frontArbBtn) {
        connect(m_frontArbBtn, &QPushButton::clicked,
                this, &ArbWidget::onFrontArb);
    }
    if (m_frontPauseBtn) {
        connect(m_frontPauseBtn, &QPushButton::clicked,
                this, &ArbWidget::onFrontPause);
    }
    if (m_frontStopBtn) {
        connect(m_frontStopBtn, &QPushButton::clicked,
                this, &ArbWidget::onFrontStop);
    }
    if (m_frontRfBtn) {
        connect(m_frontRfBtn, &QPushButton::clicked,
                this, &ArbWidget::onFrontRf);
    }

    // 波形页连接：选中行 → 同步右侧属性栏；若已有默认选中则立即刷新
    if (m_waveformTable) {
        connect(m_waveformTable, &QTableWidget::itemSelectionChanged,
                this, &ArbWidget::onWaveformSelectionChanged);
        if (m_waveformTable->currentRow() >= 0)
            onWaveformSelectionChanged();
    }

    // 播放页连接
    if (m_playBtn) {
        connect(m_playBtn, &QPushButton::clicked,
                this, &ArbWidget::onPlay);
    }
    if (m_pauseBtn) {
        connect(m_pauseBtn, &QPushButton::clicked,
                this, &ArbWidget::onPause);
    }
    if (m_playStopBtn) {
        connect(m_playStopBtn, &QPushButton::clicked,
                this, &ArbWidget::onPlaybackStop);
    }
}

// ====== 标签页初始化 ======

void ArbWidget::setupCarrierTab() {
    // 载波页参数面板（左侧，白色背景）
    auto *carrierPanel = new QWidget(this);
    carrierPanel->setObjectName(QStringLiteral("arbParamsPanel"));
    auto *layout = new QVBoxLayout(carrierPanel);
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(16);

    // ---- section 标题：信号参数配置 ----
    auto *sectionTitle = new QLabel(QStringLiteral("🎚  信号参数配置"));
    sectionTitle->setObjectName(QStringLiteral("arbSectionTitle"));
    layout->addWidget(sectionTitle);

    // ---- 频率参数组 ----
    auto *freqGroup = createParamGroup(
        QStringLiteral("载波频率 (RF Frequency)"),
        createValueRow(m_freqEdit = new QLineEdit(QStringLiteral("2.4000")),
                      QStringLiteral("GHz"),
                      m_upFreqBtn = new QPushButton(QStringLiteral("▲")),
                      m_downFreqBtn = new QPushButton(QStringLiteral("▼")))
    );
    layout->addWidget(freqGroup);

    // ---- 功率参数组 ----
    auto *powerGroup = createParamGroup(
        QStringLiteral("输出功率 (Power Level)"),
        createValueRow(m_powerEdit = new QLineEdit(QStringLiteral("-10.0")),
                      QStringLiteral("dBm"),
                      m_upPowerBtn = new QPushButton(QStringLiteral("▲")),
                      m_downPowerBtn = new QPushButton(QStringLiteral("▼")))
    );
    layout->addWidget(powerGroup);

    // ---- ARB 波形加载子标题 ----
    auto *arbTitle = new QLabel(QStringLiteral("🎵  ARB 波形加载"));
    arbTitle->setObjectName(QStringLiteral("arbSubTitle"));
    layout->addWidget(arbTitle);

    // ---- 波形选择器（卡片式） ----
    auto *waveformSelector = new QWidget;
    waveformSelector->setObjectName(QStringLiteral("arbWaveformSelector"));
    auto *wsLay = new QHBoxLayout(waveformSelector);
    wsLay->setContentsMargins(16, 12, 16, 12);
    wsLay->setSpacing(12);

    // 文件信息
    auto *fileInfoWidget = new QWidget;
    auto *fileInfoLay = new QHBoxLayout(fileInfoWidget);
    fileInfoLay->setContentsMargins(0, 0, 0, 0);
    fileInfoLay->setSpacing(12);

    auto *fileIcon = new QLabel(QStringLiteral("📄"));
    fileIcon->setStyleSheet(QStringLiteral("font-size: 28px; color: #2a6f9c;"));
    fileInfoLay->addWidget(fileIcon);

    auto *detailsWidget = new QWidget;
    auto *detailsLay = new QVBoxLayout(detailsWidget);
    detailsLay->setContentsMargins(0, 0, 0, 0);
    detailsLay->setSpacing(2);

    m_waveFileName = new QLabel(QStringLiteral("WLAN_11n_HT20_5G.wfm"));
    m_waveFileName->setStyleSheet(QStringLiteral("font-weight: 600; font-size: 14px; color: #1a2a3a;"));
    detailsLay->addWidget(m_waveFileName);

    m_waveFileMeta = new QLabel(QStringLiteral("大小: 1.2 MB · 采样率: 80 MHz · 长度: 4096 点"));
    m_waveFileMeta->setStyleSheet(QStringLiteral("font-size: 12px; color: #6a7a8a;"));
    detailsLay->addWidget(m_waveFileMeta);

    fileInfoLay->addWidget(detailsWidget);
    wsLay->addWidget(fileInfoWidget);
    wsLay->addStretch();

    // 操作按钮
    auto *wsActions = new QHBoxLayout();
    wsActions->setSpacing(6);

    m_browseBtn = new QPushButton(QStringLiteral("浏览"));
    m_browseBtn->setObjectName(QStringLiteral("arbBrowseBtn"));
    m_browseBtn->setCursor(Qt::PointingHandCursor);
    wsActions->addWidget(m_browseBtn);

    m_downloadBtn = new QPushButton(QStringLiteral("下载"));
    m_downloadBtn->setObjectName(QStringLiteral("arbDownloadBtn"));
    m_downloadBtn->setCursor(Qt::PointingHandCursor);
    wsActions->addWidget(m_downloadBtn);

    wsLay->addLayout(wsActions);
    layout->addWidget(waveformSelector);

    // ---- 采样率 + 循环模式（并排） ----
    auto *dualRow = new QHBoxLayout();
    dualRow->setSpacing(16);

    auto *srGroup = createParamGroup(
        QStringLiteral("采样率 (Sampling Rate)"),
        createValueRow(m_sampleRateEdit = new QLineEdit(QStringLiteral("80.0")),
                      QStringLiteral("MHz"),
                      m_upSampleRateBtn = new QPushButton(QStringLiteral("▲")),
                      m_downSampleRateBtn = new QPushButton(QStringLiteral("▼")))
    );
    dualRow->addWidget(srGroup, 1);

    // 循环模式参数组
    auto *modeFrame = new QFrame();
    modeFrame->setObjectName(QStringLiteral("arbParamGroup"));
    auto *modeFrameLay = new QVBoxLayout(modeFrame);
    modeFrameLay->setContentsMargins(0, 0, 0, 0);
    modeFrameLay->setSpacing(6);

    auto *modeTitle = new QLabel(QStringLiteral("循环模式"));
    modeTitle->setStyleSheet(QStringLiteral("font-size: 12px; font-weight: 500; color: #4a5a6a;"));
    modeFrameLay->addWidget(modeTitle);

    m_loopModeCombo = new QComboBox();
    m_loopModeCombo->addItems({QStringLiteral("连续 (Continuous)"),
                               QStringLiteral("单次 (Single)"),
                               QStringLiteral("触发 (Triggered)")});
    m_loopModeCombo->setCursor(Qt::PointingHandCursor);
    modeFrameLay->addWidget(m_loopModeCombo);
    dualRow->addWidget(modeFrame, 1);

    layout->addLayout(dualRow);

    // ---- 应用并播放 / 停止（并排） ----
    auto *actionBtns = new QHBoxLayout();
    actionBtns->setSpacing(10);

    m_applyBtn = new QPushButton(QStringLiteral("▶  应用并播放"));
    m_applyBtn->setObjectName(QStringLiteral("arbApplyBtn"));
    m_applyBtn->setCursor(Qt::PointingHandCursor);
    actionBtns->addWidget(m_applyBtn, 1);

    m_stopBtn = new QPushButton(QStringLiteral("■  停止"));
    m_stopBtn->setObjectName(QStringLiteral("arbStopBtn"));
    m_stopBtn->setCursor(Qt::PointingHandCursor);
    actionBtns->addWidget(m_stopBtn, 1);

    layout->addLayout(actionBtns);

    layout->addStretch();

    m_carrierStack->addWidget(carrierPanel);
    m_carrierStack->setCurrentWidget(carrierPanel);

    // 载波页作为第一个标签页
    m_tabWidget->addTab(m_carrierStack, QStringLiteral("载波"));
}

void ArbWidget::setupWaveformTab() {
    // 波形页：左右分栏（左侧波形库列表+预览，右侧属性侧边栏）
    auto *waveformPage = new QWidget(this);
    waveformPage->setObjectName(QStringLiteral("arbWaveformPage"));
    auto *pageLayout = new QHBoxLayout(waveformPage);
    pageLayout->setContentsMargins(0, 0, 0, 0);
    pageLayout->setSpacing(0);

    // ===== 左侧波形面板 =====
    auto *waveformPanel = new QWidget;
    waveformPanel->setObjectName(QStringLiteral("arbWaveformPanel"));
    auto *layout = new QVBoxLayout(waveformPanel);
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(16);

    // section 标题
    auto *sectionTitle = new QLabel(QStringLiteral("📊  波形库 (Waveform Library)"));
    sectionTitle->setObjectName(QStringLiteral("arbSectionTitle"));
    layout->addWidget(sectionTitle);

    // ---- 工具栏按钮行 ----
    auto *toolbar = new QHBoxLayout();
    toolbar->setSpacing(8);

    auto makeToolBtn = [](const QString &text, const QString &objName) {
        auto *btn = new QPushButton(text);
        btn->setObjectName(objName);
        btn->setCursor(Qt::PointingHandCursor);
        return btn;
    };

    auto *newBtn = makeToolBtn(QStringLiteral("＋  新建波形"), QStringLiteral("arbToolPrimaryBtn"));
    auto *importBtn = makeToolBtn(QStringLiteral("📂  导入"), QStringLiteral("arbToolBtn"));
    auto *editBtn = makeToolBtn(QStringLiteral("✏  编辑"), QStringLiteral("arbToolBtn"));
    auto *copyBtn = makeToolBtn(QStringLiteral("⧉  复制"), QStringLiteral("arbToolBtn"));
    auto *deleteBtn = makeToolBtn(QStringLiteral("🗑  删除"), QStringLiteral("arbToolBtn"));
    auto *downloadBtn = makeToolBtn(QStringLiteral("⬇  下载到仪器"), QStringLiteral("arbToolBtn"));

    toolbar->addWidget(newBtn);
    toolbar->addWidget(importBtn);
    toolbar->addWidget(editBtn);
    toolbar->addWidget(copyBtn);
    toolbar->addWidget(deleteBtn);
    toolbar->addWidget(downloadBtn);
    toolbar->addStretch();
    layout->addLayout(toolbar);

    connect(newBtn, &QPushButton::clicked, this, &ArbWidget::onNewWaveform);
    connect(importBtn, &QPushButton::clicked, this, &ArbWidget::onImportWaveform);
    connect(editBtn, &QPushButton::clicked, this, &ArbWidget::onEditWaveform);
    connect(copyBtn, &QPushButton::clicked, this, &ArbWidget::onCopyWaveform);
    connect(deleteBtn, &QPushButton::clicked, this, &ArbWidget::onDeleteWaveform);
    connect(downloadBtn, &QPushButton::clicked, this, &ArbWidget::onDownloadWaveform);

    // ---- 波形库列表 ----
    m_waveformTable = new QTableWidget(this);
    m_waveformTable->setObjectName(QStringLiteral("arbWaveformList"));
    m_waveformTable->setColumnCount(5);
    m_waveformTable->setHorizontalHeaderLabels({
        QStringLiteral(""), QStringLiteral("名称"),
        QStringLiteral("采样率"), QStringLiteral("长度"), QStringLiteral("状态")
    });
    m_waveformTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    m_waveformTable->horizontalHeader()->resizeSection(0, 30);
    m_waveformTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_waveformTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_waveformTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_waveformTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_waveformTable->verticalHeader()->setVisible(false);
    m_waveformTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_waveformTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_waveformTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_waveformTable->setAlternatingRowColors(true);
    m_waveformTable->setMouseTracking(true);
    m_waveformTable->viewport()->setMouseTracking(true);
    m_waveformTable->verticalHeader()->setDefaultSectionSize(36);

    // 示例数据（对齐设计稿 5 行）
    struct WaveRow { QString name; QString sampleRate; QString length; QString status; bool loaded; };
    const QList<WaveRow> rows = {
        {QStringLiteral("WLAN_11n_HT20_5G"),    QStringLiteral("80 MHz"),     QStringLiteral("4096"), QStringLiteral("已加载"), true},
        {QStringLiteral("LTE_10MHz_QPSK"),       QStringLiteral("30.72 MHz"),  QStringLiteral("1024"), QStringLiteral("待下载"), false},
        {QStringLiteral("5G_NR_100MHz_256QAM"),  QStringLiteral("122.88 MHz"), QStringLiteral("8192"), QStringLiteral("已加载"), true},
        {QStringLiteral("Multitone_64ch"),       QStringLiteral("100 MHz"),    QStringLiteral("2048"), QStringLiteral("待下载"), false},
        {QStringLiteral("Bluetooth_LE_1M"),      QStringLiteral("1 MHz"),      QStringLiteral("512"),  QStringLiteral("已加载"), true},
    };

    m_waveformTable->setRowCount(rows.size());
    for (int i = 0; i < rows.size(); ++i) {
        const auto &r = rows[i];

        // 复选框列（含 Selectable，整行选中高亮才能覆盖第 0 列）
        auto *checkItem = new QTableWidgetItem;
        checkItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        checkItem->setCheckState(Qt::Unchecked);
        m_waveformTable->setItem(i, 0, checkItem);

        // 名称（显示带图标；UserRole 存干净名称供属性栏同步）
        auto *nameItem = new QTableWidgetItem(QStringLiteral("📶  ") + r.name);
        nameItem->setData(Qt::UserRole, r.name);
        nameItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        nameItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        m_waveformTable->setItem(i, 1, nameItem);

        // 采样率显示 "80 MHz"；UserRole 存数值字符串
        QString srValue = r.sampleRate;
        const int mhzIdx = srValue.indexOf(QStringLiteral("MHz"), 0, Qt::CaseInsensitive);
        if (mhzIdx > 0)
            srValue = srValue.left(mhzIdx).trimmed();
        auto *srItem = new QTableWidgetItem(r.sampleRate);
        srItem->setData(Qt::UserRole, srValue);
        srItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        m_waveformTable->setItem(i, 2, srItem);

        auto *lenItem = new QTableWidgetItem(r.length);
        lenItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        m_waveformTable->setItem(i, 3, lenItem);

        // 状态 badge
        auto *statusItem = new QTableWidgetItem(r.status);
        statusItem->setData(Qt::UserRole, r.loaded ? QStringLiteral("loaded") : QStringLiteral("pending"));
        statusItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        m_waveformTable->setItem(i, 4, statusItem);
    }

    // 默认选中首行，触发属性栏同步
    if (m_waveformTable->rowCount() > 0)
        m_waveformTable->selectRow(0);

    layout->addWidget(m_waveformTable);

    // ---- 波形预览区（深色画布卡片） ----
    auto *previewWidget = new QWidget;
    previewWidget->setObjectName(QStringLiteral("arbWaveformPreview"));
    auto *previewLay = new QVBoxLayout(previewWidget);
    previewLay->setContentsMargins(16, 16, 16, 16);
    previewLay->setSpacing(10);

    // 预览标题行
    auto *previewHeader = new QHBoxLayout;
    m_previewTitle = new QLabel(QStringLiteral("👁  波形预览: WLAN_11n_HT20_5G"));
    m_previewTitle->setStyleSheet(QStringLiteral("font-size: 13px; font-weight: 600; color: #1a2a3a;"));
    previewHeader->addWidget(m_previewTitle);
    previewHeader->addStretch();

    auto *iqLabel = new QLabel(QStringLiteral("I/Q 时域"));
    iqLabel->setStyleSheet(QStringLiteral("font-size: 12px; color: #6a7a8a;"));
    previewHeader->addWidget(iqLabel);
    previewLay->addLayout(previewHeader);

    // 波形图表
    m_previewChart = new WaveformChart(this);
    m_previewChart->setTitle(QStringLiteral(""));
    m_previewChart->setXLabel(QStringLiteral("时间 (μs)"));
    m_previewChart->setYLabel(QStringLiteral("幅值 (V)"));

    // 示例预览数据
    WaveformData previewData;
    previewData.generateSine(1.0, 1.0, 100.0, 2, 0.0, 0.0);
    m_previewChart->setData(previewData);

    previewLay->addWidget(m_previewChart);
    layout->addWidget(previewWidget);

    pageLayout->addWidget(waveformPanel, 1);

    // ===== 右侧属性侧边栏 =====
    m_waveformSidebar = createWaveformSidebar();
    m_waveformSidebar->setObjectName(QStringLiteral("arbWaveformSidebar"));
    pageLayout->addWidget(m_waveformSidebar);

    m_tabWidget->addTab(waveformPage, QStringLiteral("波形"));
}

void ArbWidget::setupHardwareTab() {
    // 硬件页：左右分栏（左侧硬件配置卡片网格，右侧实时状态侧边栏）
    auto *hwPage = new QWidget(this);
    hwPage->setObjectName(QStringLiteral("arbHardwarePage"));
    auto *pageLayout = new QHBoxLayout(hwPage);
    pageLayout->setContentsMargins(0, 0, 0, 0);
    pageLayout->setSpacing(0);

    // ===== 左侧硬件面板 =====
    auto *hwPanel = new QWidget;
    hwPanel->setObjectName(QStringLiteral("arbHardwarePanel"));
    auto *layout = new QVBoxLayout(hwPanel);
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(16);

    // section 标题
    auto *sectionTitle = new QLabel(QStringLiteral("🔧  硬件配置 (Hardware Configuration)"));
    sectionTitle->setObjectName(QStringLiteral("arbSectionTitle"));
    layout->addWidget(sectionTitle);

    // ---- 2×2 硬件卡片网格 ----
    auto *hwGrid = new QGridLayout;
    hwGrid->setSpacing(16);

    // 辅助函数：创建参数行（label | value，value 可带 LED）
    auto makeParamRow = [](const QString &label, const QString &value,
                           const QString &ledColor = QString()) {
        auto *row = new QHBoxLayout;
        auto *lbl = new QLabel(label);
        lbl->setObjectName(QStringLiteral("arbHwRowLabel"));
        row->addWidget(lbl);
        row->addStretch();

        if (!ledColor.isEmpty()) {
            auto *led = new QLabel(QStringLiteral("●"));
            led->setStyleSheet(QStringLiteral("color: %1; font-size: 10px; margin-right: 4px;").arg(ledColor));
            row->addWidget(led);
        }

        auto *val = new QLabel(value);
        val->setObjectName(QStringLiteral("arbHwRowValue"));
        row->addWidget(val);
        return row;
    };

    // 辅助函数：创建操作按钮
    auto makeHwBtn = [](const QString &text, bool primary) {
        auto *btn = new QPushButton(text);
        btn->setObjectName(primary ? QStringLiteral("arbHwCardPrimaryBtn")
                                   : QStringLiteral("arbHwCardBtn"));
        btn->setCursor(Qt::PointingHandCursor);
        return btn;
    };

    // 辅助函数：创建按钮组
    auto makeBtnGroup = [&makeHwBtn](const QList<QPair<QString, bool>> &btns) {
        auto *group = new QHBoxLayout;
        group->setSpacing(8);
        for (const auto &b : btns) {
            group->addWidget(makeHwBtn(b.first, b.second));
        }
        group->addStretch();
        return group;
    };

    // 辅助函数：创建硬件卡片
    auto makeHwCard = [this](const QString &iconTitle, QGridLayout *grid,
                             int row, int col) {
        auto *card = new QWidget;
        card->setObjectName(QStringLiteral("arbHwCard"));
        auto *cardLay = new QVBoxLayout(card);
        cardLay->setContentsMargins(16, 16, 16, 16);
        cardLay->setSpacing(8);

        auto *title = new QLabel(iconTitle);
        title->setObjectName(QStringLiteral("arbHwCardTitle"));
        cardLay->addWidget(title);

        // 分隔线
        auto *sep = new QLabel;
        sep->setFixedHeight(1);
        sep->setStyleSheet(QStringLiteral("background: #e8ecf1;"));
        cardLay->addWidget(sep);

        grid->addWidget(card, row, col);
        return cardLay;
    };

    // 卡片1：频率参考
    auto *freqCardLay = makeHwCard(QStringLiteral("🕐  频率参考 (Frequency Reference)"), hwGrid, 0, 0);
    freqCardLay->addLayout(makeParamRow(QStringLiteral("参考源"), QStringLiteral("内部 10 MHz")));
    freqCardLay->addLayout(makeParamRow(QStringLiteral("精度"), QStringLiteral("±0.1 ppm")));
    freqCardLay->addLayout(makeParamRow(QStringLiteral("状态"), QStringLiteral("已锁定"), QStringLiteral("#2ecc71")));
    freqCardLay->addLayout(makeBtnGroup({
        {QStringLiteral("校准"), true},
        {QStringLiteral("外部参考"), false}
    }));

    // 卡片2：射频输出
    auto *rfCardLay = makeHwCard(QStringLiteral("📡  射频输出 (RF Output)"), hwGrid, 0, 1);
    rfCardLay->addLayout(makeParamRow(QStringLiteral("输出状态"), QStringLiteral("已开启"), QStringLiteral("#2ecc71")));
    rfCardLay->addLayout(makeParamRow(QStringLiteral("功率范围"), QStringLiteral("-144 ~ +13 dBm")));
    rfCardLay->addLayout(makeParamRow(QStringLiteral("阻抗"), QStringLiteral("50 Ω")));
    rfCardLay->addLayout(makeBtnGroup({
        {QStringLiteral("输出"), true},
        {QStringLiteral("衰减"), false},
        {QStringLiteral("反射计"), false}
    }));

    // 卡片3：温度监控
    auto *tempCardLay = makeHwCard(QStringLiteral("🌡  温度监控 (Temperature)"), hwGrid, 1, 0);
    tempCardLay->addLayout(makeParamRow(QStringLiteral("当前温度"), QStringLiteral("42.5 °C")));
    tempCardLay->addLayout(makeParamRow(QStringLiteral("风扇转速"), QStringLiteral("2400 RPM")));
    tempCardLay->addLayout(makeParamRow(QStringLiteral("状态"), QStringLiteral("正常"), QStringLiteral("#2ecc71")));
    tempCardLay->addLayout(makeBtnGroup({
        {QStringLiteral("风扇控制"), false},
        {QStringLiteral("日志"), false}
    }));

    // 卡片4：接口与连接
    auto *ioCardLay = makeHwCard(QStringLiteral("🔌  接口与连接 (I/O)"), hwGrid, 1, 1);
    ioCardLay->addLayout(makeParamRow(QStringLiteral("LAN"), QStringLiteral("192.168.1.100"), QStringLiteral("#2ecc71")));
    ioCardLay->addLayout(makeParamRow(QStringLiteral("USB"), QStringLiteral("已连接"), QStringLiteral("#2ecc71")));
    ioCardLay->addLayout(makeParamRow(QStringLiteral("GPIB"), QStringLiteral("未连接"), QStringLiteral("#f1c40f")));
    ioCardLay->addLayout(makeBtnGroup({
        {QStringLiteral("IP 设置"), false},
        {QStringLiteral("重启"), false}
    }));

    // 网格列等宽
    hwGrid->setColumnStretch(0, 1);
    hwGrid->setColumnStretch(1, 1);
    layout->addLayout(hwGrid);

    // ---- 底部设备信息栏 ----
    auto *infoBar = new QWidget;
    infoBar->setObjectName(QStringLiteral("arbHwInfoBar"));
    auto *infoLay = new QHBoxLayout(infoBar);
    infoLay->setContentsMargins(16, 12, 16, 12);
    infoLay->setSpacing(8);

    auto *infoLabel = new QLabel(QStringLiteral("ℹ  设备信息: AP5041A G3 · 固件 v3.2.1 · 序列号: US12345678"));
    infoLabel->setStyleSheet(QStringLiteral("font-size: 13px; color: #3a4a5a;"));
    infoLay->addWidget(infoLabel);
    infoLay->addStretch();

    auto *exportBtn = new QPushButton(QStringLiteral("导出诊断"));
    exportBtn->setObjectName(QStringLiteral("arbHwCardBtn"));
    exportBtn->setCursor(Qt::PointingHandCursor);
    infoLay->addWidget(exportBtn);

    layout->addWidget(infoBar);

    pageLayout->addWidget(hwPanel, 1);

    // ===== 右侧实时状态侧边栏 =====
    m_hardwareSidebar = createHardwareSidebar();
    m_hardwareSidebar->setObjectName(QStringLiteral("arbHardwareSidebar"));
    pageLayout->addWidget(m_hardwareSidebar);

    m_tabWidget->addTab(hwPage, QStringLiteral("硬件"));
}

void ArbWidget::setupPlaybackTab() {
    // 播放页：左右分栏（对齐 Playback.html：左播放控制+列表，右触发与同步）
    auto *playbackPage = new QWidget(this);
    playbackPage->setObjectName(QStringLiteral("arbPlaybackPage"));
    auto *pageLayout = new QHBoxLayout(playbackPage);
    pageLayout->setContentsMargins(0, 0, 0, 0);
    pageLayout->setSpacing(0);

    // ===== 左侧播放面板 =====
    auto *playbackPanel = new QWidget;
    playbackPanel->setObjectName(QStringLiteral("arbPlaybackPanel"));
    auto *layout = new QVBoxLayout(playbackPanel);
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(16);

    auto *sectionTitle = new QLabel(QStringLiteral("▶  播放控制 (Playback Control)"));
    sectionTitle->setObjectName(QStringLiteral("arbSectionTitle"));
    layout->addWidget(sectionTitle);

    // ---- 播放控制区 ----
    auto *ctrlWidget = new QWidget;
    ctrlWidget->setObjectName(QStringLiteral("arbPlaybackCtrl"));
    auto *ctrlLay = new QHBoxLayout(ctrlWidget);
    ctrlLay->setContentsMargins(20, 16, 20, 16);
    ctrlLay->setSpacing(16);

    m_playBtn = new QPushButton(QStringLiteral("▶"));
    m_playBtn->setObjectName(QStringLiteral("arbPlayCircleBtn"));
    m_playBtn->setFixedSize(48, 48);
    m_playBtn->setCursor(Qt::PointingHandCursor);
    m_playBtn->setToolTip(QStringLiteral("播放"));
    ctrlLay->addWidget(m_playBtn);

    m_pauseBtn = new QPushButton(QStringLiteral("❚❚"));
    m_pauseBtn->setObjectName(QStringLiteral("arbPauseCircleBtn"));
    m_pauseBtn->setFixedSize(48, 48);
    m_pauseBtn->setCursor(Qt::PointingHandCursor);
    m_pauseBtn->setToolTip(QStringLiteral("暂停"));
    ctrlLay->addWidget(m_pauseBtn);

    m_playStopBtn = new QPushButton(QStringLiteral("■"));
    m_playStopBtn->setObjectName(QStringLiteral("arbStopCircleBtn"));
    m_playStopBtn->setFixedSize(48, 48);
    m_playStopBtn->setCursor(Qt::PointingHandCursor);
    m_playStopBtn->setToolTip(QStringLiteral("停止"));
    ctrlLay->addWidget(m_playStopBtn);

    auto *infoWidget = new QWidget;
    auto *infoLay = new QVBoxLayout(infoWidget);
    infoLay->setContentsMargins(0, 0, 0, 0);
    infoLay->setSpacing(4);

    m_playWaveName = new QLabel(QStringLiteral("WLAN_11n_HT20_5G"));
    m_playWaveName->setObjectName(QStringLiteral("arbPlayWaveName"));
    infoLay->addWidget(m_playWaveName);

    m_playStatus = new QLabel(QStringLiteral("● 播放中 · 00:00 / 00:35"));
    m_playStatus->setObjectName(QStringLiteral("arbPlayStatus"));
    infoLay->addWidget(m_playStatus);

    ctrlLay->addWidget(infoWidget, 1);

    auto *nextBtn = new QPushButton(QStringLiteral("⏭  下一段"));
    nextBtn->setObjectName(QStringLiteral("arbToolBtn"));
    nextBtn->setCursor(Qt::PointingHandCursor);
    ctrlLay->addWidget(nextBtn);

    layout->addWidget(ctrlWidget);

    // ---- 时间线（设计稿：左 00:00 / 右总时长） ----
    auto *timelineWidget = new QWidget;
    timelineWidget->setObjectName(QStringLiteral("arbTimeline"));
    auto *timelineLay = new QVBoxLayout(timelineWidget);
    timelineLay->setContentsMargins(4, 4, 4, 4);
    timelineLay->setSpacing(4);

    m_playProgress = new QProgressBar;
    m_playProgress->setRange(0, 1000);
    m_playProgress->setValue(0);
    m_playProgress->setTextVisible(false);
    m_playProgress->setFixedHeight(6);
    timelineLay->addWidget(m_playProgress);

    auto *timeRow = new QHBoxLayout;
    m_playTimeStart = new QLabel(QStringLiteral("00:00"));
    m_playTimeStart->setObjectName(QStringLiteral("arbTimeLabel"));
    timeRow->addWidget(m_playTimeStart);
    timeRow->addStretch();
    m_playTimeEnd = new QLabel(QStringLiteral("00:35"));
    m_playTimeEnd->setObjectName(QStringLiteral("arbTimeLabel"));
    timeRow->addWidget(m_playTimeEnd);
    m_playDurationLabel = m_playTimeEnd;
    timelineLay->addLayout(timeRow);

    layout->addWidget(timelineWidget);

    // ---- 播放设置 2×2（setting-card） ----
    auto *settingsGrid = new QGridLayout;
    settingsGrid->setSpacing(16);

    auto makeSettingCard = [](const QString &label, QWidget *field) {
        auto *card = new QWidget;
        card->setObjectName(QStringLiteral("arbSettingCard"));
        auto *lay = new QVBoxLayout(card);
        lay->setContentsMargins(14, 14, 14, 14);
        lay->setSpacing(4);
        auto *lbl = new QLabel(label);
        lbl->setObjectName(QStringLiteral("arbSettingLabel"));
        lay->addWidget(lbl);
        lay->addWidget(field);
        return card;
    };

    m_playLoopCombo = new QComboBox;
    m_playLoopCombo->addItems({
        QStringLiteral("连续 (Continuous)"),
        QStringLiteral("单次 (Single)"),
        QStringLiteral("触发 (Triggered)"),
        QStringLiteral("列表循环 (List Loop)")
    });
    m_playLoopCombo->setCursor(Qt::PointingHandCursor);
    settingsGrid->addWidget(
        makeSettingCard(QStringLiteral("循环模式 (Play Mode)"), m_playLoopCombo), 0, 0);

    m_playTrigSrc = new QComboBox;
    m_playTrigSrc->addItems({
        QStringLiteral("内部 (Internal)"),
        QStringLiteral("外部 (External)"),
        QStringLiteral("GPIB"),
        QStringLiteral("手动 (Manual)")
    });
    m_playTrigSrc->setCursor(Qt::PointingHandCursor);
    settingsGrid->addWidget(
        makeSettingCard(QStringLiteral("触发源 (Trigger Source)"), m_playTrigSrc), 0, 1);

    auto *delayRow = new QWidget;
    auto *delayLay = new QHBoxLayout(delayRow);
    delayLay->setContentsMargins(0, 0, 0, 0);
    delayLay->setSpacing(8);
    m_playTrigDelay = new QLineEdit(QStringLiteral("0.0"));
    m_playTrigDelay->setMaximumWidth(100);
    delayLay->addWidget(m_playTrigDelay);
    auto *delayUnit = new QLabel(QStringLiteral("μs"));
    delayUnit->setObjectName(QStringLiteral("arbSettingUnit"));
    delayLay->addWidget(delayUnit);
    delayLay->addStretch();
    settingsGrid->addWidget(
        makeSettingCard(QStringLiteral("触发延迟 (Trigger Delay)"), delayRow), 1, 0);

    auto *gapRow = new QWidget;
    auto *gapLay = new QHBoxLayout(gapRow);
    gapLay->setContentsMargins(0, 0, 0, 0);
    gapLay->setSpacing(8);
    m_playGap = new QLineEdit(QStringLiteral("1.0"));
    m_playGap->setMaximumWidth(100);
    gapLay->addWidget(m_playGap);
    auto *gapUnit = new QLabel(QStringLiteral("ms"));
    gapUnit->setObjectName(QStringLiteral("arbSettingUnit"));
    gapLay->addWidget(gapUnit);
    gapLay->addStretch();
    settingsGrid->addWidget(
        makeSettingCard(QStringLiteral("段间间隔 (Segment Gap)"), gapRow), 1, 1);

    settingsGrid->setColumnStretch(0, 1);
    settingsGrid->setColumnStretch(1, 1);
    layout->addLayout(settingsGrid);

    // ---- 播放列表 ----
    auto *playlistTitleRow = new QHBoxLayout;
    auto *playlistTitle = new QLabel(QStringLiteral("📋  播放列表 (Playlist)"));
    playlistTitle->setStyleSheet(QStringLiteral("font-size: 13px; font-weight: 600; color: #1a2a3a;"));
    playlistTitleRow->addWidget(playlistTitle);
    playlistTitleRow->addStretch();

    auto *addBtn = new QPushButton(QStringLiteral("＋  添加"));
    addBtn->setObjectName(QStringLiteral("arbToolBtn"));
    addBtn->setCursor(Qt::PointingHandCursor);
    playlistTitleRow->addWidget(addBtn);
    layout->addLayout(playlistTitleRow);

    m_playlistTable = new QTableWidget(this);
    m_playlistTable->setObjectName(QStringLiteral("arbPlaylistTable"));
    m_playlistTable->setColumnCount(5);
    m_playlistTable->setHorizontalHeaderLabels({
        QStringLiteral("#"), QStringLiteral("波形名称"),
        QStringLiteral("时长"), QStringLiteral("循环"), QStringLiteral("")
    });
    m_playlistTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    m_playlistTable->horizontalHeader()->resizeSection(0, 36);
    m_playlistTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_playlistTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    m_playlistTable->horizontalHeader()->resizeSection(2, 72);
    m_playlistTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    m_playlistTable->horizontalHeader()->resizeSection(3, 64);
    m_playlistTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Fixed);
    m_playlistTable->horizontalHeader()->resizeSection(4, 48);
    m_playlistTable->verticalHeader()->setVisible(false);
    m_playlistTable->setShowGrid(false);
    m_playlistTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_playlistTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_playlistTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_playlistTable->setAlternatingRowColors(false);
    m_playlistTable->setFocusPolicy(Qt::NoFocus);
    m_playlistTable->verticalHeader()->setDefaultSectionSize(36);
    m_playlistTable->setMinimumHeight(140);

    layout->addWidget(m_playlistTable, 1);

    connect(nextBtn, &QPushButton::clicked, this, &ArbWidget::onNextSegment);
    connect(addBtn, &QPushButton::clicked, this, &ArbWidget::onAddToPlaylist);
    connect(m_playlistTable, &QTableWidget::itemSelectionChanged,
            this, &ArbWidget::onPlaylistSelectionChanged);

    pageLayout->addWidget(playbackPanel, 1);

    m_playbackSidebar = createPlaybackSidebar();
    m_playbackSidebar->setObjectName(QStringLiteral("arbPlaybackSidebar"));
    pageLayout->addWidget(m_playbackSidebar);

    m_tabWidget->addTab(playbackPage, QStringLiteral("播放"));
}

// ====== 前面板 ======

QWidget *ArbWidget::createFrontPanel() {
    auto *panel = new QWidget();
    panel->setObjectName(QStringLiteral("arbFrontPanel"));
    auto *layout = new QVBoxLayout(panel);
    layout->setContentsMargins(28, 24, 28, 24);
    layout->setSpacing(18);

    // ---- 品牌行 ----
    auto *brandWidget = new QWidget;
    brandWidget->setObjectName(QStringLiteral("arbPanelBrand"));
    auto *brandLay = new QHBoxLayout(brandWidget);
    brandLay->setContentsMargins(0, 0, 0, 10);

    auto *brandLabel = new QLabel(QStringLiteral("KEYSIGHT"));
    brandLabel->setStyleSheet(QStringLiteral("color: #8a9aaa; font-size: 12px; letter-spacing: 0.5px;"));
    brandLay->addWidget(brandLabel);
    brandLay->addStretch();

    auto *modelLabel = new QLabel(QStringLiteral("AP5041A G3"));
    modelLabel->setStyleSheet(QStringLiteral("color: #c8d0dc; font-weight: 600; font-size: 14px;"));
    brandLay->addWidget(modelLabel);
    brandLay->addStretch();

    m_panelReady = new QLabel(QStringLiteral("● 就绪"));
    m_panelReady->setStyleSheet(QStringLiteral("color: #2ecc71; font-size: 12px;"));
    brandLay->addWidget(m_panelReady);

    layout->addWidget(brandWidget);

    // ---- 模拟 LCD 屏幕 ----
    auto *screenWidget = new QWidget;
    screenWidget->setObjectName(QStringLiteral("arbPanelScreen"));
    auto *screenLay = new QVBoxLayout(screenWidget);
    screenLay->setContentsMargins(18, 14, 18, 14);
    screenLay->setSpacing(6);

    // FREQ 行
    auto *row1 = new QHBoxLayout;
    auto *freqLabel = new QLabel(QStringLiteral("FREQ"));
    freqLabel->setStyleSheet(QStringLiteral("color: #5a7a8a; font-size: 11px;"));
    row1->addWidget(freqLabel);
    row1->addStretch();
    m_screenFreq = new QLabel(QStringLiteral("2.400 000 000 GHz"));
    m_screenFreq->setObjectName(QStringLiteral("freqScreen"));
    m_screenFreq->setStyleSheet(QStringLiteral("font-family: 'Menlo'; font-size: 14px; "
                                               "color: #6fc8f0; font-weight: 600;"));
    row1->addWidget(m_screenFreq);
    screenLay->addLayout(row1);

    // POWER 行
    auto *row2 = new QHBoxLayout;
    auto *powerLabel = new QLabel(QStringLiteral("POWER"));
    powerLabel->setStyleSheet(QStringLiteral("color: #5a7a8a; font-size: 11px;"));
    row2->addWidget(powerLabel);
    row2->addStretch();
    m_screenPower = new QLabel(QStringLiteral("-10.00 dBm"));
    m_screenPower->setObjectName(QStringLiteral("powerScreen"));
    m_screenPower->setStyleSheet(QStringLiteral("font-family: 'Menlo'; font-size: 14px; "
                                                "color: #f0c86f; font-weight: 600;"));
    row2->addWidget(m_screenPower);
    screenLay->addLayout(row2);

    // 分隔线
    auto *screenSep = new QLabel;
    screenSep->setFixedHeight(1);
    screenSep->setStyleSheet(QStringLiteral("background: #1a2a3a;"));
    screenLay->addWidget(screenSep);

    // ARB 状态行
    auto *row3 = new QHBoxLayout;
    auto *arbLabel = new QLabel(QStringLiteral("ARB 状态"));
    arbLabel->setStyleSheet(QStringLiteral("color: #5a7a8a; font-size: 11px;"));
    row3->addWidget(arbLabel);
    row3->addStretch();
    m_screenArbStatus = new QLabel(QStringLiteral("▶ 播放中"));
    m_screenArbStatus->setObjectName(QStringLiteral("statusScreen"));
    m_screenArbStatus->setStyleSheet(QStringLiteral("font-family: 'Menlo'; font-size: 14px; "
                                                    "color: #7ad0a0; font-weight: 500;"));
    row3->addWidget(m_screenArbStatus);
    screenLay->addLayout(row3);

    // 波形名行
    auto *row4 = new QHBoxLayout;
    auto *waveLabel = new QLabel(QStringLiteral("波形"));
    waveLabel->setStyleSheet(QStringLiteral("color: #5a7a8a; font-size: 11px;"));
    row4->addWidget(waveLabel);
    row4->addStretch();
    m_screenWaveName = new QLabel(QStringLiteral("WLAN_11n_HT20_5G"));
    m_screenWaveName->setObjectName(QStringLiteral("waveScreen"));
    m_screenWaveName->setStyleSheet(QStringLiteral("font-size: 12px; color: #8a9aaa;"));
    row4->addWidget(m_screenWaveName);
    screenLay->addLayout(row4);

    layout->addWidget(screenWidget);

    // ---- 控制区：旋钮组 + 按钮组 ----
    auto *controlsWidget = new QWidget;
    auto *controlsLay = new QHBoxLayout(controlsWidget);
    controlsLay->setContentsMargins(0, 0, 0, 0);
    controlsLay->setSpacing(16);

    // 旋钮组
    auto *knobGroup = new QHBoxLayout;
    knobGroup->setSpacing(10);

    auto makeKnob = [this](const QString &label, QPushButton *&btn) {
        auto *w = new QWidget;
        auto *l = new QVBoxLayout(w);
        l->setContentsMargins(0, 0, 0, 0);
        l->setSpacing(4);
        l->setAlignment(Qt::AlignCenter);

        btn = new QPushButton(QStringLiteral("▲"));
        btn->setObjectName(QStringLiteral("arbKnob"));
        btn->setFixedSize(50, 50);
        btn->setCursor(Qt::PointingHandCursor);
        l->addWidget(btn, 0, Qt::AlignCenter);

        auto *lbl = new QLabel(label);
        lbl->setStyleSheet(QStringLiteral("color: #8a9aaa; font-size: 10px; text-align: center; "
                                         "letter-spacing: 0.3px;"));
        lbl->setAlignment(Qt::AlignCenter);
        l->addWidget(lbl);

        return w;
    };

    QPushButton *knobFreqBtn = nullptr;
    QPushButton *knobPowerBtn = nullptr;
    knobGroup->addWidget(makeKnob(QStringLiteral("频率"), knobFreqBtn));
    knobGroup->addWidget(makeKnob(QStringLiteral("功率"), knobPowerBtn));
    if (knobFreqBtn) {
        connect(knobFreqBtn, &QPushButton::clicked, this, &ArbWidget::onFreqUp);
    }
    if (knobPowerBtn) {
        connect(knobPowerBtn, &QPushButton::clicked, this, &ArbWidget::onPowerUp);
    }
    controlsLay->addLayout(knobGroup);

    controlsLay->addStretch();

    // 按钮组
    auto *btnGroup = new QHBoxLayout;
    btnGroup->setSpacing(6);

    m_frontArbBtn = new QPushButton(QStringLiteral("▶ ARB"));
    m_frontArbBtn->setObjectName(QStringLiteral("arbFrontArbBtn"));
    m_frontArbBtn->setCheckable(true);
    m_frontArbBtn->setChecked(true);
    m_frontArbBtn->setCursor(Qt::PointingHandCursor);
    btnGroup->addWidget(m_frontArbBtn);

    m_frontPauseBtn = new QPushButton(QStringLiteral("❚❚ 暂停"));
    m_frontPauseBtn->setObjectName(QStringLiteral("arbFrontPauseBtn"));
    m_frontPauseBtn->setCheckable(true);
    m_frontPauseBtn->setCursor(Qt::PointingHandCursor);
    btnGroup->addWidget(m_frontPauseBtn);

    m_frontStopBtn = new QPushButton(QStringLiteral("■ 停止"));
    m_frontStopBtn->setObjectName(QStringLiteral("arbFrontStopBtn"));
    m_frontStopBtn->setCheckable(true);
    m_frontStopBtn->setCursor(Qt::PointingHandCursor);
    btnGroup->addWidget(m_frontStopBtn);

    m_frontRfBtn = new QPushButton(QStringLiteral("⏻ RF"));
    m_frontRfBtn->setObjectName(QStringLiteral("arbFrontRfBtn"));
    m_frontRfBtn->setCheckable(true);
    m_frontRfBtn->setChecked(false);
    m_frontRfBtn->setCursor(Qt::PointingHandCursor);
    btnGroup->addWidget(m_frontRfBtn);

    controlsLay->addLayout(btnGroup);

    layout->addWidget(controlsWidget);

    // ---- LED 指示灯行 ----
    auto *ledWidget = new QWidget;
    ledWidget->setObjectName(QStringLiteral("arbLedRow"));
    auto *ledLay = new QHBoxLayout(ledWidget);
    ledLay->setContentsMargins(0, 6, 0, 0);
    ledLay->setSpacing(16);

    auto makeLed = [](const QString &text, const QString &color, QLabel *&dot) {
        auto *w = new QWidget;
        auto *l = new QHBoxLayout(w);
        l->setContentsMargins(0, 0, 0, 0);
        l->setSpacing(4);

        dot = new QLabel(QStringLiteral("●"));
        dot->setStyleSheet(QStringLiteral("color: %1; font-size: 10px;").arg(color));
        l->addWidget(dot);

        auto *lbl = new QLabel(text);
        lbl->setStyleSheet(QStringLiteral("color: #5a7a8a; font-size: 10px;"));
        l->addWidget(lbl);

        return w;
    };

    ledLay->addWidget(makeLed(QStringLiteral("MOD"), QStringLiteral("#2ecc71"), m_ledMod));
    ledLay->addWidget(makeLed(QStringLiteral("ARB"), QStringLiteral("#2ecc71"), m_ledArb));
    ledLay->addWidget(makeLed(QStringLiteral("OVLD"), QStringLiteral("#f1c40f"), m_ledOvld));
    ledLay->addWidget(makeLed(QStringLiteral("LOCK"), QStringLiteral("#2ecc71"), m_ledLock));
    ledLay->addStretch();

    layout->addWidget(ledWidget);

    layout->addStretch();

    return panel;
}

// ====== 侧边栏创建 ======

QWidget *ArbWidget::createWaveformSidebar() {
    auto *sidebar = new QWidget();
    auto *layout = new QVBoxLayout(sidebar);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(16);

    // section 标题：波形属性
    auto *propTitle = new QLabel(QStringLiteral("🎛  波形属性"));
    propTitle->setObjectName(QStringLiteral("arbSidebarTitle"));
    layout->addWidget(propTitle);

    // 波形名称
    auto *nameGroup = createParamGroup(QStringLiteral("波形名称"),
        createValueRow(m_propName = new QLineEdit(QStringLiteral("WLAN_11n_HT20_5G")),
                      QStringLiteral(""), nullptr, nullptr));
    layout->addWidget(nameGroup);

    // 波形类型
    auto *typeGroup = createParamGroup(QStringLiteral("波形类型"),
        createValueRow(m_propType = new QComboBox(),
                      QStringLiteral(""), nullptr, nullptr));
    m_propType->addItems({QStringLiteral("自定义 (ARB)"),
                          QStringLiteral("多音 (Multitone)"),
                          QStringLiteral("调制 (Modulated)")});
    layout->addWidget(typeGroup);

    // 采样率
    auto *srGroup = createParamGroup(QStringLiteral("采样率"),
        createValueRow(m_propSampleRate = new QLineEdit(QStringLiteral("80.0")),
                      QStringLiteral("MHz"), nullptr, nullptr));
    layout->addWidget(srGroup);

    // 点数
    auto *samplesGroup = createParamGroup(QStringLiteral("点数 (Samples)"),
        createValueRow(m_propSamples = new QLineEdit(QStringLiteral("4096")),
                      QStringLiteral(""), nullptr, nullptr));
    layout->addWidget(samplesGroup);

    // I/Q 数据格式
    auto *formatGroup = createParamGroup(QStringLiteral("I/Q 数据格式"),
        createValueRow(m_propFormat = new QComboBox(),
                      QStringLiteral(""), nullptr, nullptr));
    m_propFormat->addItems({QStringLiteral("16-bit 定点"), QStringLiteral("32-bit 浮点")});
    layout->addWidget(formatGroup);

    // section 标题：快速操作
    auto *actionTitle = new QLabel(QStringLiteral("⚡  快速操作"));
    actionTitle->setObjectName(QStringLiteral("arbSidebarTitle"));
    layout->addWidget(actionTitle);

    // 预览播放按钮
    auto *previewBtn = new QPushButton(QStringLiteral("▶  预览播放"));
    previewBtn->setObjectName(QStringLiteral("arbSidebarPrimaryBtn"));
    previewBtn->setCursor(Qt::PointingHandCursor);
    connect(previewBtn, &QPushButton::clicked, this, &ArbWidget::onPlayPreview);
    layout->addWidget(previewBtn);

    // 保存波形按钮
    auto *saveBtn = new QPushButton(QStringLiteral("💾  保存波形"));
    saveBtn->setObjectName(QStringLiteral("arbSidebarSecondaryBtn"));
    saveBtn->setCursor(Qt::PointingHandCursor);
    layout->addWidget(saveBtn);

    layout->addStretch();

    return sidebar;
}

QWidget *ArbWidget::createHardwareSidebar() {
    auto *sidebar = new QWidget();
    auto *layout = new QVBoxLayout(sidebar);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(0);

    // section 标题：实时状态
    auto *title = new QLabel(QStringLiteral("📊  实时状态"));
    title->setObjectName(QStringLiteral("arbSidebarTitle"));
    layout->addWidget(title);

    // 状态项列表
    struct StatusItem { QString label; QString value; };
    const QList<StatusItem> statusItems = {
        {QStringLiteral("CPU 负载"),    QStringLiteral("23%")},
        {QStringLiteral("内存使用"),    QStringLiteral("156 MB / 512 MB")},
        {QStringLiteral("ARB 内存"),    QStringLiteral("12% 已用")},
        {QStringLiteral("输出功率"),    QStringLiteral("-10.0 dBm")},
        {QStringLiteral("VSWR"),        QStringLiteral("1.25 : 1")},
        {QStringLiteral("校准有效期"),  QStringLiteral("2027-01-15")}
    };

    for (const auto &item : statusItems) {
        auto *row = new QHBoxLayout;
        auto *label = new QLabel(item.label);
        label->setObjectName(QStringLiteral("arbHwRowLabel"));
        row->addWidget(label);
        row->addStretch();

        auto *value = new QLabel(item.value);
        value->setObjectName(QStringLiteral("arbHwRowValue"));
        row->addWidget(value);

        auto *rowWidget = new QWidget;
        rowWidget->setObjectName(QStringLiteral("arbHwInfoItem"));
        auto *rowLay = new QHBoxLayout(rowWidget);
        rowLay->setContentsMargins(0, 8, 0, 8);
        rowLay->addLayout(row);
        layout->addWidget(rowWidget);
    }

    // 最近事件卡片
    auto *eventCard = new QWidget;
    eventCard->setObjectName(QStringLiteral("arbHwEventCard"));
    auto *eventLay = new QVBoxLayout(eventCard);
    eventLay->setContentsMargins(12, 12, 12, 12);
    eventLay->setSpacing(4);

    auto *eventTitle = new QLabel(QStringLiteral("最近事件"));
    eventTitle->setStyleSheet(QStringLiteral("font-size: 12px; color: #6a7a8a; margin-bottom: 4px;"));
    eventLay->addWidget(eventTitle);

    const QStringList events = {
        QStringLiteral("[14:28] 波形加载成功"),
        QStringLiteral("[14:25] RF 输出开启"),
        QStringLiteral("[14:20] 频率锁定")
    };
    for (const auto &e : events) {
        auto *eLabel = new QLabel(e);
        eLabel->setStyleSheet(QStringLiteral("font-size: 12px; color: #1a2a3a; padding: 2px 0;"));
        eventLay->addWidget(eLabel);
    }

    layout->addWidget(eventCard);

    layout->addStretch();

    return sidebar;
}

QWidget *ArbWidget::createPlaybackSidebar() {
    auto *sidebar = new QWidget();
    auto *layout = new QVBoxLayout(sidebar);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(0);

    // section 标题：触发与同步
    auto *title = new QLabel(QStringLiteral("⚡  触发与同步"));
    title->setObjectName(QStringLiteral("arbSidebarTitle"));
    layout->addWidget(title);

    // 触发项列表
    struct TriggerItem { QString label; QString value; };
    const QList<TriggerItem> triggerItems = {
        {QStringLiteral("触发模式"), QStringLiteral("上升沿")},
        {QStringLiteral("触发输入"), QStringLiteral("BNC (Rear)")},
        {QStringLiteral("触发输出"), QStringLiteral("已启用")},
        {QStringLiteral("同步时钟"), QStringLiteral("内部 10 MHz")},
        {QStringLiteral("帧同步"), QStringLiteral("Marker 1")}
    };

    for (const auto &item : triggerItems) {
        auto *rowWidget = new QWidget;
        rowWidget->setObjectName(QStringLiteral("arbHwInfoItem"));
        auto *rowLay = new QHBoxLayout(rowWidget);
        rowLay->setContentsMargins(0, 8, 0, 8);

        auto *label = new QLabel(item.label);
        label->setObjectName(QStringLiteral("arbHwRowLabel"));
        rowLay->addWidget(label);
        rowLay->addStretch();

        auto *value = new QLabel(item.value);
        value->setObjectName(QStringLiteral("arbHwRowValue"));
        rowLay->addWidget(value);

        layout->addWidget(rowWidget);
    }

    // ---- Marker 设置卡片 ----
    auto *markerCard = new QWidget;
    markerCard->setObjectName(QStringLiteral("arbMarkerCard"));
    auto *markerLay = new QVBoxLayout(markerCard);
    markerLay->setContentsMargins(12, 12, 12, 12);
    markerLay->setSpacing(6);

    auto *markerTitle = new QLabel(QStringLiteral("Marker 设置"));
    markerTitle->setStyleSheet(QStringLiteral("font-size: 12px; color: #6a7a8a; margin-bottom: 4px;"));
    markerLay->addWidget(markerTitle);

    auto *markerRow = new QHBoxLayout;
    markerRow->setSpacing(12);
    for (int i = 1; i <= 3; ++i) {
        auto *check = new QCheckBox(QStringLiteral("Marker %1").arg(i));
        if (i == 1) check->setChecked(true);
        check->setStyleSheet(QStringLiteral("font-size: 12px; color: #3a4a5a;"));
        markerRow->addWidget(check);
    }
    markerRow->addStretch();
    markerLay->addLayout(markerRow);

    layout->addWidget(markerCard);

    auto *syncBtn = new QPushButton(QStringLiteral("🔄  同步播放"));
    syncBtn->setObjectName(QStringLiteral("arbSidebarPrimaryBtn"));
    syncBtn->setCursor(Qt::PointingHandCursor);
    connect(syncBtn, &QPushButton::clicked, this, &ArbWidget::onSyncPlay);
    layout->addSpacing(12);
    layout->addWidget(syncBtn);

    layout->addStretch();

    return sidebar;
}

// ====== 辅助函数 ======

QFrame *ArbWidget::createParamGroup(const QString &label, QWidget *valueWidget) {
    auto *group = new QFrame();
    auto *layout = new QVBoxLayout(group);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    auto *titleLabel = new QLabel(label);
    titleLabel->setStyleSheet("font-size: 12px; font-weight: 500; color: #4a5a6a;");

    auto *valueLayout = new QHBoxLayout();
    valueLayout->addWidget(valueWidget);

    layout->addWidget(titleLabel);
    layout->addLayout(valueLayout);

    return group;
}

QFrame *ArbWidget::createValueRow(QWidget *valueWidget, const QString &unit,
                                  QWidget *upBtn, QWidget *downBtn) {
    auto *row = new QFrame();
    auto *layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);

    layout->addWidget(valueWidget);
    layout->addWidget(new QLabel(unit));

    if (upBtn) {
        upBtn->setObjectName(QStringLiteral("arbStepBtn"));
        upBtn->setFixedSize(30, 30);
        layout->addWidget(upBtn);
    }

    if (downBtn) {
        downBtn->setObjectName(QStringLiteral("arbStepBtn"));
        downBtn->setFixedSize(30, 30);
        layout->addWidget(downBtn);
    }

    layout->addStretch();

    return row;
}

// ====== 槽函数实现 ======

void ArbWidget::onApplyAndPlay() {
    spdlog::info("ArbWidget: Apply and play requested");
    m_rfEnabled = true;
    m_arbEnabled = true;
    updateHardwareStatus();
}

void ArbWidget::onStop() {
    spdlog::info("ArbWidget: Stop requested");
    m_rfEnabled = false;
    m_arbEnabled = false;
    updateHardwareStatus();
}

void ArbWidget::onBrowseWaveform() {
    const QString path = selectWaveformFile();
    if (path.isEmpty()) {
        return;
    }

    if (loadWaveformData(path)) {
        m_currentWaveformPath = path;
        QString fileName = QFileInfo(path).fileName();

        // 更新载波页波形文件名标签
        if (m_waveFileName) {
            m_waveFileName->setText(fileName);
        }

        // 更新前面板 LCD 波形名
        if (m_screenWaveName) {
            m_screenWaveName->setText(fileName);
        }

        // 更新前面板 ARB 状态
        if (m_screenArbStatus) {
            m_screenArbStatus->setText(QStringLiteral("▶ 播放中"));
        }

        m_rfEnabled = true;
        m_arbEnabled = true;
        updateHardwareStatus();
        spdlog::info("ArbWidget: Waveform uploaded successfully: {}", path.toStdString());
    } else {
        QMessageBox::warning(this, QStringLiteral("加载失败"),
                             QStringLiteral("无法解析波形文件！"));
    }
}

void ArbWidget::onDownloadWaveform() {
    const QString path = FileManagerDialog::selectFile(this, QStringLiteral("下载波形 - 选择目标位置"));
    if (!path.isEmpty())
        spdlog::info("ArbWidget: 下载波形到 {}", path.toStdString());
}

void ArbWidget::setRfEnabled(bool enabled) {
    if (m_rfEnabled == enabled)
        return;
    m_rfEnabled = enabled;
    if (!m_rfEnabled)
        m_arbEnabled = false;
    updateHardwareStatus();
    emit rfToggled(m_rfEnabled);
}

void ArbWidget::onRfToggle() {
    setRfEnabled(!m_rfEnabled);
}

void ArbWidget::onFreqUp() {
    adjustFrequency(0.1);
}

void ArbWidget::onFreqDown() {
    adjustFrequency(-0.1);
}

void ArbWidget::onPowerUp() {
    adjustPower(1.0);
}

void ArbWidget::onPowerDown() {
    adjustPower(-1.0);
}

void ArbWidget::onSampleRateUp() {
    adjustSampleRate(1.0);
}

void ArbWidget::onSampleRateDown() {
    adjustSampleRate(-1.0);
}

void ArbWidget::onWaveformSelectionChanged() {
    if (!m_waveformTable)
        return;

    const auto selected = m_waveformTable->selectionModel()
                              ? m_waveformTable->selectionModel()->selectedRows()
                              : QModelIndexList();
    if (selected.isEmpty())
        return;

    const int row = selected.first().row();
    auto *nameItem = m_waveformTable->item(row, 1);
    auto *srItem = m_waveformTable->item(row, 2);
    auto *lenItem = m_waveformTable->item(row, 3);
    if (!nameItem || !srItem || !lenItem)
        return;

    // 优先读 UserRole 中的干净数据；无则回退解析显示文本
    QString name = nameItem->data(Qt::UserRole).toString();
    if (name.isEmpty()) {
        name = nameItem->text().trimmed();
        static const QString kIconPrefix = QStringLiteral("📶");
        if (name.startsWith(kIconPrefix))
            name = name.mid(kIconPrefix.size()).trimmed();
    }

    QString srText = srItem->data(Qt::UserRole).toString();
    if (srText.isEmpty()) {
        srText = srItem->text().trimmed();
        const int mhzIdx = srText.indexOf(QStringLiteral("MHz"), 0, Qt::CaseInsensitive);
        if (mhzIdx > 0)
            srText = srText.left(mhzIdx).trimmed();
        else
            srText.remove(QRegularExpression(QStringLiteral("[^0-9.]")));
    }

    const QString samples = lenItem->text().trimmed();

    if (m_propName)
        m_propName->setText(name);
    if (m_propSampleRate)
        m_propSampleRate->setText(srText);
    if (m_propSamples)
        m_propSamples->setText(samples);

    // 按名称启发式推断波形类型
    if (m_propType) {
        const QString upper = name.toUpper();
        int typeIdx = 0; // 自定义 (ARB)
        if (upper.contains(QStringLiteral("MULTITONE")) ||
            (upper.contains(QStringLiteral("TONE")) && !upper.contains(QStringLiteral("BLUETOOTH"))))
            typeIdx = 1; // 多音
        else if (upper.contains(QStringLiteral("WLAN")) || upper.contains(QStringLiteral("LTE")) ||
                 upper.contains(QStringLiteral("5G")) || upper.contains(QStringLiteral("NR")) ||
                 upper.contains(QStringLiteral("BLUETOOTH")) || upper.contains(QStringLiteral("QAM")) ||
                 upper.contains(QStringLiteral("QPSK")))
            typeIdx = 2; // 调制
        if (typeIdx >= 0 && typeIdx < m_propType->count())
            m_propType->setCurrentIndex(typeIdx);
    }

    if (m_previewTitle)
        m_previewTitle->setText(QStringLiteral("👁  波形预览: %1").arg(name));
}

void ArbWidget::onNewWaveform() {
    const QString path = FileManagerDialog::selectFile(this, QStringLiteral("新建波形 - 选择文件位置"));
    if (!path.isEmpty())
        spdlog::info("ArbWidget: 新建波形目标路径 {}", path.toStdString());
}

void ArbWidget::onImportWaveform() {
    const QString path = FileManagerDialog::selectFile(this, QStringLiteral("导入波形 - 选择波形文件"));
    if (!path.isEmpty())
        spdlog::info("ArbWidget: 导入波形 {}", path.toStdString());
}

void ArbWidget::onEditWaveform() {
    const QString path = FileManagerDialog::selectFile(this, QStringLiteral("编辑波形 - 选择波形文件"));
    if (!path.isEmpty())
        spdlog::info("ArbWidget: 编辑波形 {}", path.toStdString());
}

void ArbWidget::onCopyWaveform() {
    const QString path = FileManagerDialog::selectFile(this, QStringLiteral("复制波形 - 选择波形文件"));
    if (!path.isEmpty())
        spdlog::info("ArbWidget: 复制波形 {}", path.toStdString());
}

void ArbWidget::onDeleteWaveform() {
    const QString path = FileManagerDialog::selectFile(this, QStringLiteral("删除波形 - 选择波形文件"));
    if (!path.isEmpty())
        spdlog::info("ArbWidget: 删除波形 {}", path.toStdString());
}

void ArbWidget::onPlay() {
    startPlayback();
}

void ArbWidget::onPause() {
    pausePlayback();
}

void ArbWidget::onPlaybackStop() {
    stopPlayback();
}

// ====== 播放引擎 ======

void ArbWidget::initPlaylist() {
    m_playlist.clear();
    m_playlist.append({QStringLiteral("WLAN_11n_HT20_5G"),    35.0, -1, -1, true});
    m_playlist.append({QStringLiteral("5G_NR_100MHz_256QAM"), 62.0,  3,  3, false});
    m_playlist.append({QStringLiteral("LTE_10MHz_QPSK"),      18.0,  1,  1, false});
    m_currentSegment = 0;
    m_playElapsedSec = 0.0;
    m_playDurationSec = m_playlist.isEmpty() ? 35.0 : m_playlist.first().durationSec;
    m_isPlaying = false;
    m_syncMode = false;
}

void ArbWidget::renderPlaylist() {
    if (!m_playlistTable) return;
    const int n = m_playlist.size();
    m_playlistTable->blockSignals(true);
    m_playlistTable->setRowCount(n);
    for (int i = 0; i < n; ++i) {
        const auto &e = m_playlist.at(i);
        const bool infinite = e.loopCount < 0;
        const QString loopTxt = infinite
            ? QStringLiteral("∞")
            : QStringLiteral("%1x").arg(e.loopCount);

        QString statusTxt = QStringLiteral("⏸");
        QString statusColor = QStringLiteral("#6a7a8a");
        if (i == m_currentSegment && m_isPlaying) {
            statusTxt = QStringLiteral("▶");
            statusColor = QStringLiteral("#2ecc71");
        } else if (i == m_currentSegment && m_playElapsedSec > 0.0) {
            statusTxt = QStringLiteral("❚❚");
            statusColor = QStringLiteral("#f39c12");
        }

        auto *idxItem = new QTableWidgetItem(QString::number(i + 1));
        idxItem->setTextAlignment(Qt::AlignCenter);
        idxItem->setForeground(QColor(QStringLiteral("#6a7a8a")));
        m_playlistTable->setItem(i, 0, idxItem);

        auto *nameItem = new QTableWidgetItem(e.name);
        {
            QFont f = m_playlistTable->font();
            f.setWeight(QFont::DemiBold);
            nameItem->setFont(f);
        }
        m_playlistTable->setItem(i, 1, nameItem);

        auto *durItem = new QTableWidgetItem(
            QStringLiteral("%1s").arg(static_cast<int>(e.durationSec + 0.5)));
        durItem->setForeground(QColor(QStringLiteral("#5a6a7a")));
        m_playlistTable->setItem(i, 2, durItem);

        // 循环列：用 QLabel badge 复刻设计稿胶囊样式
        auto *loopBadge = new QLabel(loopTxt);
        loopBadge->setAlignment(Qt::AlignCenter);
        // 设计稿：∞ 绿色 badge；有限次数黄色 badge.inf
        loopBadge->setObjectName(infinite ? QStringLiteral("arbLoopBadge")
                                          : QStringLiteral("arbLoopBadgeInf"));
        m_playlistTable->setCellWidget(i, 3, loopBadge);

        auto *stItem = new QTableWidgetItem(statusTxt);
        stItem->setTextAlignment(Qt::AlignCenter);
        stItem->setForeground(QColor(statusColor));
        m_playlistTable->setItem(i, 4, stItem);
    }
    if (m_currentSegment >= 0 && m_currentSegment < n)
        m_playlistTable->selectRow(m_currentSegment);
    m_playlistTable->blockSignals(false);
}

QString ArbWidget::formatTime(double sec) const {
    const int total = static_cast<int>(sec + 0.0001);
    const int m = total / 60;
    const int s = total % 60;
    return QStringLiteral("%1:%2").arg(m, 2, 10, QChar('0')).arg(s, 2, 10, QChar('0'));
}

void ArbWidget::updatePlaybackUI() {
    QString waveName = m_waveformDisplayName;
    if (!m_playlist.isEmpty() && m_currentSegment >= 0
        && m_currentSegment < m_playlist.size()) {
        const auto &e = m_playlist.at(m_currentSegment);
        waveName = e.name;
        m_playDurationSec = e.durationSec;
    }
    // 去掉扩展名显示，对齐设计稿
    if (waveName.endsWith(QStringLiteral(".wfm"), Qt::CaseInsensitive))
        waveName.chop(4);

    if (m_playWaveName) m_playWaveName->setText(waveName);

    const int pct = m_playDurationSec > 0.0
        ? static_cast<int>(qBound(0.0, m_playElapsedSec / m_playDurationSec, 1.0) * 1000.0)
        : 0;
    if (m_playProgress) m_playProgress->setValue(pct);
    if (m_playTimeStart) m_playTimeStart->setText(formatTime(m_playElapsedSec));
    if (m_playTimeEnd)   m_playTimeEnd->setText(formatTime(m_playDurationSec));

    // 设计稿：播放中 · 00:12 / 00:35
    if (m_playStatus) {
        const QString t = formatTime(m_playElapsedSec) + QStringLiteral(" / ")
                        + formatTime(m_playDurationSec);
        if (m_isPlaying) {
            m_playStatus->setText(QStringLiteral("● 播放中 · ") + t);
            m_playStatus->setStyleSheet(QStringLiteral(
                "font-size: 13px; color: #6a7a8a;"));
        } else if (m_playElapsedSec > 0.0) {
            m_playStatus->setText(QStringLiteral("● 已暂停 · ") + t);
            m_playStatus->setStyleSheet(QStringLiteral(
                "font-size: 13px; color: #f39c12;"));
        } else {
            m_playStatus->setText(QStringLiteral("● 已停止 · ") + t);
            m_playStatus->setStyleSheet(QStringLiteral(
                "font-size: 13px; color: #95a5a6;"));
        }
    }

    if (m_footerWave)
        m_footerWave->setText(QStringLiteral("波形: ") + waveName);
    if (m_footerStatus) {
        if (m_isPlaying)
            m_footerStatus->setText(QStringLiteral("● 播放中"));
        else if (m_playElapsedSec > 0.0)
            m_footerStatus->setText(QStringLiteral("● 已暂停"));
        else
            m_footerStatus->setText(m_arbEnabled ? QStringLiteral("● 运行中")
                                                 : QStringLiteral("● 已停止"));
    }
    if (m_footerTime)
        m_footerTime->setText(
            QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm")));
}

void ArbWidget::startPlayback() {
    if (m_playlist.isEmpty()) return;
    if (m_playElapsedSec >= m_playDurationSec) m_playElapsedSec = 0.0;
    m_isPlaying = true;
    m_syncMode = false;
    m_arbEnabled = true;
    m_rfEnabled = true;
    m_playTimer->start();
    updateHardwareStatus();
    renderPlaylist();
    updatePlaybackUI();
    spdlog::info("ArbWidget: playback started at segment {}", m_currentSegment + 1);
}

void ArbWidget::pausePlayback() {
    if (!m_isPlaying) return;
    m_isPlaying = false;
    m_playTimer->stop();
    updateHardwareStatus();
    renderPlaylist();
    updatePlaybackUI();
    spdlog::info("ArbWidget: playback paused");
}

void ArbWidget::stopPlayback() {
    m_isPlaying = false;
    m_syncMode = false;
    m_playTimer->stop();
    m_playElapsedSec = 0.0;
    m_arbEnabled = false;
    updateHardwareStatus();
    renderPlaylist();
    updatePlaybackUI();
    spdlog::info("ArbWidget: playback stopped");
}

void ArbWidget::selectSegment(int idx) {
    if (idx < 0 || idx >= m_playlist.size()) return;
    m_currentSegment = idx;
    m_playElapsedSec = 0.0;
    for (int i = 0; i < m_playlist.size(); ++i)
        m_playlist[i].playing = (i == idx);
    m_playDurationSec = m_playlist.at(idx).durationSec;
    renderPlaylist();
    updatePlaybackUI();
    spdlog::info("ArbWidget: selected segment {}", idx + 1);
}

void ArbWidget::advanceSegment() {
    if (m_playlist.isEmpty()) return;
    selectSegment((m_currentSegment + 1) % m_playlist.size());
}

void ArbWidget::onTimerTick() {
    if (!m_isPlaying || m_playlist.isEmpty()) return;
    m_playElapsedSec += 0.1;   // 100ms 步进 ≈ 实时
    auto &e = m_playlist[m_currentSegment];

    if (m_playElapsedSec >= m_playDurationSec) {
        if (e.loopCount < 0) {
            m_playElapsedSec = 0.0;             // ∞：原地循环
        } else {
            e.remainingLoops -= 1;
            if (e.remainingLoops > 0) {
                m_playElapsedSec = 0.0;         // 本段下一轮
            } else {
                m_playElapsedSec = 0.0;
                advanceSegment();               // 列表循环：进入下一段
            }
        }
    }
    updatePlaybackUI();
}

void ArbWidget::onNextSegment() {
    advanceSegment();
    spdlog::info("ArbWidget: next segment -> {}", m_currentSegment + 1);
}

void ArbWidget::onAddToPlaylist() {
    bool ok = false;
    const QString name = QInputDialog::getText(
        this, QStringLiteral("添加到播放列表"),
        QStringLiteral("波形名称:"), QLineEdit::Normal,
        QStringLiteral("CUSTOM_WAVE"), &ok);
    if (!ok || name.trimmed().isEmpty()) return;

    ArbPlaylistEntry e;
    e.name = name.trimmed();
    e.durationSec = 20.0;
    e.loopCount = 1;
    e.remainingLoops = 1;
    e.playing = false;
    m_playlist.append(e);
    renderPlaylist();
    updatePlaybackUI();
    spdlog::info("ArbWidget: added playlist entry {}", e.name.toStdString());
}

void ArbWidget::onPlaylistSelectionChanged() {
    if (!m_playlistTable) return;
    const int row = m_playlistTable->currentRow();
    if (row < 0) return;
    selectSegment(row);
}

void ArbWidget::onSyncPlay() {
    if (m_playlist.isEmpty()) return;
    m_syncMode = true;
    m_currentSegment = 0;
    m_playElapsedSec = 0.0;
    for (auto &e : m_playlist) {
        if (e.loopCount >= 0) e.remainingLoops = e.loopCount;
        e.playing = false;
    }
    m_playlist.first().playing = true;
    m_isPlaying = true;
    m_arbEnabled = true;
    m_rfEnabled = true;
    m_playTimer->start();
    updateHardwareStatus();
    renderPlaylist();
    updatePlaybackUI();
    spdlog::info("ArbWidget: synchronized playback started (list loop)");
}

void ArbWidget::onTabChanged(int index) {
    m_activeTab = index;
    spdlog::info("ArbWidget: Tab changed to {}", index);

    // 同步左侧导航高亮 + 步骤徽标
    for (int i = 0; i < m_navBtns.size(); ++i) {
        m_navBtns.at(i)->setChecked(i == index);
        if (i < m_navBadges.size()) {
            // 选中时徽标变蓝（对齐 5GNR 左侧导航 .step-badge 高亮）
            m_navBadges.at(i)->setProperty("active", i == index);
            m_navBadges.at(i)->style()->unpolish(m_navBadges.at(i));
            m_navBadges.at(i)->style()->polish(m_navBadges.at(i));
        }
    }

    // 载波页：右侧前面板；其它页侧边栏已内嵌各 page，无需再切 splitter
    if (m_frontPanel)
        m_frontPanel->setVisible(index == 0);
}

void ArbWidget::adjustFrequency(double delta) {
    m_frequency += delta;
    if (m_frequency < kMinFrequency) m_frequency = kMinFrequency;
    if (m_frequency > kMaxFrequency) m_frequency = kMaxFrequency;

    updateHardwareStatus();
}

void ArbWidget::adjustPower(double delta) {
    m_power += delta;
    if (m_power < kMinPower) m_power = kMinPower;
    if (m_power > kMaxPower) m_power = kMaxPower;

    updateHardwareStatus();
}

void ArbWidget::adjustSampleRate(double delta) {
    m_sampleRate += delta;
    if (m_sampleRate < kMinSampleRate) m_sampleRate = kMinSampleRate;
    if (m_sampleRate > kMaxSampleRate) m_sampleRate = kMaxSampleRate;

    updateHardwareStatus();
}

QString ArbWidget::selectWaveformFile() {
    const QString filter = QStringLiteral("波形文件 (*.wfm *.arb *.csv);;所有文件 (*)");
    return QFileDialog::getOpenFileName(this, QStringLiteral("选择波形文件"),
                                         QDir::homePath(), filter);
}

bool ArbWidget::loadWaveformData(const QString &filePath) {
    WaveformData data;
    if (!data.loadCSV(filePath)) {
        QMessageBox::warning(this, QStringLiteral("加载失败"),
                             QStringLiteral("无法解析 CSV 波形文件"));
        return false;
    }

    m_currentData = data;
    return true;
}

void ArbWidget::updateHardwareStatus() {
    // 更新载波页输入框
    if (m_freqEdit) {
        m_freqEdit->setText(QStringLiteral("%1").arg(m_frequency, 4, 'f', 4));
    }
    if (m_powerEdit) {
        m_powerEdit->setText(QStringLiteral("%1").arg(m_power, 4, 'f', 2));
    }
    if (m_sampleRateEdit) {
        m_sampleRateEdit->setText(QStringLiteral("%1").arg(m_sampleRate, 4, 'f', 1));
    }

    // 更新前面板 LCD 显示
    if (m_screenFreq) {
        m_screenFreq->setText(QStringLiteral("%1 GHz").arg(m_frequency, 0, 'f', 9).insert(4, ' '));
    }
    if (m_screenPower) {
        m_screenPower->setText(QStringLiteral("%1 dBm").arg(m_power, 0, 'f', 2));
    }

    // 更新前面板 ARB 状态行
    if (m_screenArbStatus) {
        if (m_arbEnabled && m_rfEnabled) {
            m_screenArbStatus->setText(QStringLiteral("▶ 播放中"));
            m_screenArbStatus->setStyleSheet(QStringLiteral("font-family: 'Menlo'; "
                                                            "font-size: 14px; color: #7ad0a0; font-weight: 500;"));
        } else if (m_arbEnabled) {
            m_screenArbStatus->setText(QStringLiteral("就绪"));
            m_screenArbStatus->setStyleSheet(QStringLiteral("font-family: 'Menlo'; "
                                                            "font-size: 14px; color: #8a9aaa; font-weight: 500;"));
        } else {
            m_screenArbStatus->setText(QStringLiteral("停止"));
            m_screenArbStatus->setStyleSheet(QStringLiteral("font-family: 'Menlo'; "
                                                            "font-size: 14px; color: #e74c3c; font-weight: 500;"));
        }
    }

    // 更新前面板就绪指示
    if (m_panelReady) {
        if (m_rfEnabled) {
            m_panelReady->setText(QStringLiteral("● 就绪"));
            m_panelReady->setStyleSheet(QStringLiteral("color: #2ecc71; font-size: 12px;"));
        } else {
            m_panelReady->setText(QStringLiteral("● 待机"));
            m_panelReady->setStyleSheet(QStringLiteral("color: #e74c3c; font-size: 12px;"));
        }
    }

    // 同步前面板 RF 按钮
    if (m_frontRfBtn) {
        m_frontRfBtn->setChecked(m_rfEnabled);
    }

    // 底部状态栏（播放态由 updatePlaybackUI 覆盖）
    if (m_footerStatus && !m_isPlaying && m_playElapsedSec <= 0.0) {
        m_footerStatus->setText(m_arbEnabled ? QStringLiteral("● 运行中")
                                             : QStringLiteral("● 已停止"));
    }
    if (m_footerSr) {
        m_footerSr->setText(QStringLiteral("波形采样率: %1 MHz").arg(m_sampleRate, 0, 'f', 1));
    }
}

void ArbWidget::onFrontRf() {
    m_rfEnabled = !m_rfEnabled;
    m_arbEnabled = m_rfEnabled && m_arbEnabled;
    updateHardwareStatus();
}

void ArbWidget::onFrontArb() {
    m_arbEnabled = !m_arbEnabled;
    updateHardwareStatus();
}

void ArbWidget::onFrontPause() {
    m_arbEnabled = !m_arbEnabled;
    updateHardwareStatus();
}

void ArbWidget::onFrontStop() {
    stopPlayback();
}

void ArbWidget::onPowerEdited() {
    bool ok;
    double power = m_powerEdit->text().toDouble(&ok);
    if (ok) {
        m_power = power;
        adjustPower(0);
    }
}

void ArbWidget::onFreqEdited() {
    bool ok;
    double freq = m_freqEdit->text().toDouble(&ok);
    if (ok) {
        m_frequency = freq;
        adjustFrequency(0);
    }
}

void ArbWidget::onSampleRateEdited() {
    bool ok;
    double sr = m_sampleRateEdit->text().toDouble(&ok);
    if (ok) {
        m_sampleRate = sr;
        adjustSampleRate(0);
    }
}

void ArbWidget::onPlayPreview() {
    if (!m_previewChart)
        return;

    // 从右侧属性栏读取参数
    const QString name = m_propName ? m_propName->text().trimmed()
                                    : QStringLiteral("preview");
    bool srOk = false;
    double srMhz = m_propSampleRate ? m_propSampleRate->text().toDouble(&srOk) : 80.0;
    if (!srOk || srMhz <= 0.0)
        srMhz = 80.0;

    bool nOk = false;
    int samples = m_propSamples ? m_propSamples->text().toInt(&nOk) : 4096;
    if (!nOk || samples < 16)
        samples = 4096;
    constexpr int kMaxPreviewPts = 4096;
    if (samples > kMaxPreviewPts)
        samples = kMaxPreviewPts;

    const int typeIdx = m_propType ? m_propType->currentIndex() : 0;

    // 数值映射：采样率 MHz 数值 → fs；点数 ≈ samples，约 kCycles 个周期
    // 横轴单位与图表标签「时间 (μs)」一致（1 单位 = 1 μs）
    constexpr int kCycles = 4;
    constexpr double kAmp = 1.0;
    const double fs = srMhz;
    const double f = (kCycles * fs) / static_cast<double>(samples);
    const double duration = static_cast<double>(samples) / fs;

    WaveformData data;
    switch (typeIdx) {
    case 1: // 多音：扫频近似多音时域形态
        data.generateSweep(f, f * 5.0, kAmp, fs, duration, 0.0, 0.0);
        break;
    case 2: // 调制：方波近似数字调制包络边沿
        data.generateSquare(f, kAmp, fs, kCycles, 0.02);
        break;
    default: // 自定义 ARB：正弦
        data.generateSine(f, kAmp, fs, kCycles, 0.0, 0.0);
        break;
    }

    if (data.isEmpty()) {
        spdlog::warn("ArbWidget: 预览波形生成失败 name={}", name.toStdString());
        return;
    }

    m_previewChart->setData(data);
    if (m_previewTitle)
        m_previewTitle->setText(QStringLiteral("👁  波形预览: %1").arg(name));

    spdlog::info("ArbWidget: 预览播放 name={} type={} sr={}MHz samples={}",
                 name.toStdString(), typeIdx, srMhz, samples);
}

void ArbWidget::onNavClicked() {
    // 导航栏点击处理（预留）
}
