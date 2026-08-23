#ifndef FIVE_G_NR_WIDGET_H
#define FIVE_G_NR_WIDGET_H

#include <QWidget>

#include "WaveformChart.h"

class QComboBox;
class QFrame;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QSplitter;
class QStackedWidget;

/**
 * @brief 5G NR 信号配置界面 — Keysight Signal Studio for 5G NR (M9484C VXG) 风格
 *
 * 布局对齐设计稿：
 *   Apps 菜单栏(5G NR/WLAN/LTE/... + RF) →
 *   三栏主体(左导航 / 中参数配置 / 右辅助面板) → 底部状态栏
 *   （全局标题栏由 main_ui 的 AppHeaderBar 提供）
 *   左导航 4 个配置步骤：载波(Carrier) / BWP / 用户(UE) / 路由(Routing)
 */
class FiveGNrWidget : public QWidget {
    Q_OBJECT

public:
    explicit FiveGNrWidget(QWidget *parent = nullptr);
    ~FiveGNrWidget() override;

private slots:
    void onNavClicked();
    void onAppClicked();
    void onRfToggle();
    void onPresetClicked();
    void onGenerateClicked();
    void onPreviewClicked();
    void onSavePresetClicked();

    // 载波页参数变更 → 同步右侧信号状态
    void onCarrierParamChanged();

private:
    void loadStyleSheet();
    void setupUI();
    void setupConnections();

    // 容器构建
    QWidget *createAppsBar();
    QWidget *createNavPanel();
    QWidget *createConfigPanel();
    QWidget *createSidePanel();
    QWidget *createFooter();

    // 四个配置步骤页
    QWidget *createCarrierPage();
    QWidget *createBwpPage();
    QWidget *createUserPage();
    QWidget *createRoutingPage();

    // 辅助：参数组 / 参数行（label + 控件 + 单位）
    QFrame *createParamSection(const QString &title, const QString &badge,
                               const QString &badgeClass);
    QFrame *createParamRow();
    QFrame *makeParamItem(const QString &label, QWidget *control,
                          const QString &unit = QString());

    void setActiveNav(QPushButton *nav);
    void updateSignalStatus();
    void applyPreset(const QString &preset);

    // ===== Apps / 导航 / 状态栏 =====
    QWidget *m_appsBar = nullptr;
    QSplitter *m_mainSplitter = nullptr;
    QWidget *m_footer = nullptr;

    // Apps 栏
    QPushButton *m_rfToggleBtn = nullptr;
    QList<QPushButton *> m_appBtns;

    // 左导航
    QList<QPushButton *> m_navBtns;
    QList<QLabel *> m_navBadges;

    // 中央配置面板
    QStackedWidget *m_configStack = nullptr;
    QLabel *m_panelTitle = nullptr;
    QLabel *m_panelSub = nullptr;
    QLabel *m_panelBadge = nullptr;
    QLabel *m_panelDesc = nullptr;

    // 载波页控件
    QComboBox *m_freqRangeCombo = nullptr;
    QLineEdit *m_carrierFreqEdit = nullptr;
    QComboBox *m_bwCombo = nullptr;
    QComboBox *m_scsCombo = nullptr;
    QComboBox *m_duplexCombo = nullptr;
    QComboBox *m_frameCombo = nullptr;
    QComboBox *m_cpCombo = nullptr;
    QComboBox *m_ssbCombo = nullptr;
    QComboBox *m_csiRsCombo = nullptr;

    // 用户页控件
    QLineEdit *m_ueIdEdit = nullptr;
    QLineEdit *m_rntiEdit = nullptr;
    QComboBox *m_ueCountCombo = nullptr;
    QComboBox *m_multiUserCombo = nullptr;
    QComboBox *m_pdschModCombo = nullptr;
    QSpinBox *m_pdschMcsSpin = nullptr;
    QLabel *m_pdschRateValue = nullptr;
    QLabel *m_pdschTbsValue = nullptr;
    QComboBox *m_pdschBwpCombo = nullptr;
    QComboBox *m_pdschPortCombo = nullptr;
    QComboBox *m_puschModCombo = nullptr;
    QSpinBox *m_puschMcsSpin = nullptr;
    QComboBox *m_puschBwpCombo = nullptr;

    // 路由页控件
    QComboBox *m_rfPortCombo = nullptr;
    QComboBox *m_antennaCombo = nullptr;
    QComboBox *m_beamCombo = nullptr;
    QComboBox *m_mimoCombo = nullptr;
    QComboBox *m_precodingCombo = nullptr;

    // 右侧辅助面板
    QLabel *m_hierCellVal = nullptr;
    QLabel *m_hierBwpVal = nullptr;
    QLabel *m_hierCoresetVal = nullptr;
    QLabel *m_hierUserVal = nullptr;
    QLabel *m_statFrVal = nullptr;
    QLabel *m_statFreqVal = nullptr;
    QLabel *m_statBwVal = nullptr;
    QLabel *m_statScsVal = nullptr;
    QLabel *m_statStatusVal = nullptr;
    WaveformChart *m_previewChart = nullptr;

    // 底部状态栏
    QLabel *m_footerStatus = nullptr;
    QLabel *m_footerWave = nullptr;
    QLabel *m_footerMem = nullptr;
    QLabel *m_footerModel = nullptr;
    QLabel *m_footerFw = nullptr;
    QLabel *m_footerTime = nullptr;

    // 状态
    bool m_rfEnabled = true;
};

#endif // FIVE_G_NR_WIDGET_H
