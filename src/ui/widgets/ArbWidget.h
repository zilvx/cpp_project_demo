#ifndef ARB_WIDGET_H
#define ARB_WIDGET_H

#include <QList>
#include <QWidget>

#include "../core/WaveformData.h"
#include "WaveformChart.h"

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QFrame;
class QLabel;
class QLineEdit;
class QProgressBar;
class QPushButton;
class QSplitter;
class QStackedWidget;
class QTableWidget;
class QTabWidget;
class QTimer;

/**
 * @brief ARB 任意波形界面 — Keysight VSG / Signal Studio 风格
 *
 * 布局对齐设计稿：
 *   标题栏(RF 开关) → 内容区(左导航 + 中央标签页 + 右前面板/侧边栏) → 底部状态栏
 *   载波页：中央参数配置 + 右前面板
 *   波形/硬件/播放页：主面板 + 右侧属性栏
 */
class ArbWidget : public QWidget {
    Q_OBJECT

public:
    explicit ArbWidget(QWidget *parent = nullptr);
    ~ArbWidget() override;

signals:
    void rfToggled(bool enabled);

private slots:
    void onNavClicked();
    void onRfToggle();
    void onFreqUp();
    void onFreqDown();
    void onFreqEdited();
    void onPowerUp();
    void onPowerDown();
    void onPowerEdited();
    void onSampleRateUp();
    void onSampleRateDown();
    void onSampleRateEdited();
    void onBrowseWaveform();
    void onDownloadWaveform();
    void onApplyAndPlay();
    void onStop();
    void onFrontArb();
    void onFrontPause();
    void onFrontStop();
    void onFrontRf();
    void onWaveformSelectionChanged();
    void onPlay();
    void onPause();
    void onPlaybackStop();
    void onNextSegment();
    void onAddToPlaylist();
    void onNewWaveform();
    void onImportWaveform();
    void onEditWaveform();
    void onCopyWaveform();
    void onDeleteWaveform();
    void onPlayPreview();
    void onTabChanged(int index);
    void onTimerTick();
    void onPlaylistSelectionChanged();
    void onSyncPlay();

private:
    // 播放列表条目（驱动播放引擎的数据源，UI 由 renderPlaylist 重绘）
    struct ArbPlaylistEntry {
        QString name;          // 波形名称
        double durationSec = 0.0; // 单段时长（秒）
        int loopCount = -1;    // -1 = ∞（无限循环）；>=1 = 循环次数
        int remainingLoops = -1; // 剩余循环（finite 时有效）
        bool playing = false;  // 是否为当前播放段
    };

    void loadStyleSheet();
    void setupUI();
    void setupConnections();
    void setActiveTab(int index);
    void refreshDisplays();
    void adjustFrequency(double delta);
    void adjustPower(double delta);
    void adjustSampleRate(double delta);
    void updateHardwareStatus();
    QString selectWaveformFile();
    bool loadWaveformData(const QString &filePath);
    void setWaveformMeta(const QString &name, const QString &meta);
    QLabel *makeLedDot(const QString &colorClass) const;
    QFrame *makeHwCard(const QString &title, const QStringList &rows,
                       const QStringList &btnLabels) const;

    QWidget *createHeader();
    QWidget *createNavPanel();
    QWidget *createFooter();
    QWidget *createCarrierPage();
    QWidget *createWaveformPage();
    QWidget *createHardwarePage();
    QWidget *createPlaybackPage();
    QWidget *createFrontPanel();
    QWidget *createWaveformSidebar();
    QWidget *createHardwareSidebar();
    QWidget *createPlaybackSidebar();

    // 辅助函数
    QFrame *createParamGroup(const QString &label, QWidget *valueWidget);
    QFrame *createValueRow(QWidget *valueWidget, const QString &unit,
                          QWidget *upBtn, QWidget *downBtn = nullptr);
    QFrame *createHwCard(const QString &title, const QStringList &rows,
                         const QStringList &btnLabels);

    // 标签页初始化函数
    void setupCarrierTab();
    void setupWaveformTab();
    void setupHardwareTab();
    void setupPlaybackTab();

    // ---- 播放引擎 ----
    void initPlaylist();
    void renderPlaylist();
    void updatePlaybackUI();
    void startPlayback();
    void pausePlayback();
    void stopPlayback();
    void selectSegment(int idx);
    void advanceSegment();
    QString formatTime(double sec) const;

    // ---- 状态 ----
    WaveformData m_currentData;
    QString m_currentWaveformPath;
    QString m_waveformDisplayName = QStringLiteral("WLAN_11n_HT20_5G.wfm");
    double m_frequency = 2.4;
    double m_power = -10.0;
    double m_sampleRate = 80.0;
    bool m_rfEnabled = true;
    bool m_arbEnabled = true;
    bool m_playing = true;
    int m_activeTab = 0;

    // ---- 顶栏 / 导航 / 状态栏 ----
    QWidget *m_header = nullptr;           // 标题栏
    QWidget *m_navPanel = nullptr;         // 左侧导航栏
    QTabWidget *m_tabWidget = nullptr;
    QStackedWidget *m_carrierStack = nullptr;
    QWidget *m_carrierControlPanel = nullptr;
    QWidget *m_frontPanel = nullptr;
    QWidget *m_waveformSidebar = nullptr;
    QWidget *m_hardwareSidebar = nullptr;
    QWidget *m_playbackSidebar = nullptr;
    QWidget *m_footer = nullptr;           // 底部状态栏

    // 标题栏控件
    QLabel *m_waveformNameLabel = nullptr;  // 载波页波形文件名
    QLabel *m_statusLabel = nullptr;        // 前面板 ARB 状态
    QLabel *m_freqLabel = nullptr;
    QLabel *m_powerLabel = nullptr;

    // 前面板 LCD 屏幕标签
    QLabel *m_screenFreq = nullptr;         // LCD 频率显示
    QLabel *m_screenPower = nullptr;        // LCD 功率显示
    QLabel *m_screenArbStatus = nullptr;    // LCD ARB 状态
    QLabel *m_screenWaveName = nullptr;     // LCD 波形名

    // 底部状态栏
    QLabel *m_footerStatus = nullptr;
    QLabel *m_footerWave = nullptr;   // 当前波形名（播放页对齐设计稿）
    QLabel *m_footerMem = nullptr;
    QLabel *m_footerSr = nullptr;
    QLabel *m_footerTime = nullptr;

    // ---- 导航按钮 ----
    QPushButton *m_rfToggleBtn = nullptr;  // 标题栏 RF 开关
    QList<QPushButton *> m_navBtns;        // 载波/波形/硬件/播放
    QList<QLabel *> m_navBadges;           // 导航步骤徽标（激活项蓝色高亮）

    // ---- 载波 ----
    QLineEdit *m_freqEdit = nullptr;
    QLineEdit *m_powerEdit = nullptr;
    QLineEdit *m_sampleRateEdit = nullptr;
    QComboBox *m_loopModeCombo = nullptr;
    QLabel *m_waveFileName = nullptr;
    QLabel *m_waveFileMeta = nullptr;
    QPushButton *m_browseBtn = nullptr;
    QPushButton *m_downloadBtn = nullptr;
    QPushButton *m_applyBtn = nullptr;
    QPushButton *m_stopBtn = nullptr;
    QPushButton *m_upFreqBtn = nullptr;
    QPushButton *m_downFreqBtn = nullptr;
    QPushButton *m_upPowerBtn = nullptr;
    QPushButton *m_downPowerBtn = nullptr;
    QPushButton *m_upSampleRateBtn = nullptr;
    QPushButton *m_downSampleRateBtn = nullptr;

    // ---- 前面板 ----
    QLabel *m_panelReady = nullptr;
    QPushButton *m_frontArbBtn = nullptr;
    QPushButton *m_frontPauseBtn = nullptr;
    QPushButton *m_frontStopBtn = nullptr;
    QPushButton *m_frontRfBtn = nullptr;
    QLabel *m_ledMod = nullptr;
    QLabel *m_ledArb = nullptr;
    QLabel *m_ledOvld = nullptr;
    QLabel *m_ledLock = nullptr;

    // ---- 波形页 ----
    QTableWidget *m_waveformTable = nullptr;
    WaveformChart *m_previewChart = nullptr;
    QLabel *m_previewTitle = nullptr;
    QLineEdit *m_propName = nullptr;
    QComboBox *m_propType = nullptr;
    QLineEdit *m_propSampleRate = nullptr;
    QLineEdit *m_propSamples = nullptr;
    QComboBox *m_propFormat = nullptr;

    // ---- 硬件页 ----
    QLabel *m_hwCpu = nullptr;
    QLabel *m_hwMem = nullptr;
    QLabel *m_hwArbMem = nullptr;
    QLabel *m_hwOutPower = nullptr;
    QLabel *m_hwVswr = nullptr;
    QLabel *m_hwCal = nullptr;

    // ---- 播放页 ----
    QLabel *m_playWaveName = nullptr;
    QLabel *m_playStatus = nullptr;
    QProgressBar *m_playProgress = nullptr;
    QLabel *m_playTimeStart = nullptr;
    QLabel *m_playTimeEnd = nullptr;
    QComboBox *m_playLoopCombo = nullptr;
    QComboBox *m_playTrigSrc = nullptr;
    QLineEdit *m_playTrigDelay = nullptr;
    QLineEdit *m_playGap = nullptr;
    QTableWidget *m_playlistTable = nullptr;
    QPushButton *m_playBtn = nullptr;
    QPushButton *m_pauseBtn = nullptr;
    QPushButton *m_playStopBtn = nullptr;

    // ---- 播放引擎状态 ----
    QTimer *m_playTimer = nullptr;     // 播放推进定时器（100ms 一次）
    int m_currentSegment = 0;          // 当前播放段索引（0 基）
    double m_playElapsedSec = 0.0;     // 当前段已播放秒数
    double m_playDurationSec = 35.0;   // 当前段总时长（秒）
    bool m_isPlaying = false;          // 是否正在播放
    bool m_syncMode = false;           // 同步播放（列表循环）模式
    QLabel *m_playDurationLabel = nullptr; // 时间线“总时长”标签
    QList<ArbPlaylistEntry> m_playlist; // 播放列表数据
};

#endif // ARB_WIDGET_H
