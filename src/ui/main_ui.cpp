#include <QApplication>
#include <QComboBox>
#include <QCoreApplication>
#include <QDir>
#include <QDoubleSpinBox>
#include <QElapsedTimer>
#include <QFile>
#include <QFileDialog>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMetaObject>
#include <QPushButton>
#include <QSlider>
#include <QTabWidget>
#include <QThread>
#include <QVBoxLayout>
#include <QWidget>
#include <memory>
#include <map>
#include <spdlog/spdlog.h>
#include <QColor>
#include <QUrl>
#include <QQmlContext>
#include <QQmlEngine>
#include <QtQuickWidgets/QQuickWidget>
#include "../utils/logging/logutil.h"

#include "widgets/TableWidget.h"
#include "widgets/VirtualKeyboard.h"
#include "widgets/WaveformChart.h"
#include "widgets/ArbWidget.h"
#include "widgets/BasicToolsWidget.h"
#include "widgets/FileManagerWidget.h"
#include "widgets/FiveGNrWidget.h"
#include "components/ui_common/app_header_bar.h"
#include "core/WaveformData.h"
#include "core/WaveformGenerator.h"
#include "core/ToastService.h"
#include "providers/SampleDataProvider.h"
#include "providers/HttpDataProvider.h"
#ifdef USE_PROTO_DATA
#include "providers/HttpProtoDataProvider.h"
#endif

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    // macOS 原生文件面板依赖 Bundle ID；命令行直接跑二进制时 Info.plist 可能未加载
    QCoreApplication::setOrganizationName(QStringLiteral("cpp_project_demo"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("com.cppprojectdemo"));
    QCoreApplication::setApplicationName(QStringLiteral("qt_table_app"));
    LogUtil::init("qt_table_app");

    auto *tablePage = new QWidget;
    auto *tableLayout = new QVBoxLayout(tablePage);
    tableLayout->setContentsMargins(0, 0, 0, 0);
    tableLayout->setSpacing(0);
    auto *table = new TableWidget(tablePage);
    tableLayout->addWidget(table);

#ifdef USE_PROTO_DATA
    auto *protoProvider = new HttpProtoDataProvider(
        QUrl("http://localhost:8080/api/table/proto"), table);
    QObject::connect(protoProvider, &IDataProvider::dataReady, table, &TableWidget::loadData);
    QObject::connect(protoProvider, &IDataProvider::errorOccurred,
                     [](const QString &msg) { spdlog::warn("Proto HTTP Error: {}", msg.toStdString()); });
    protoProvider->fetchData();
#elif defined(USE_HTTP_DATA)
    auto *httpProvider = new HttpDataProvider(
        QUrl("http://localhost:8080/api/table"), table);
    QObject::connect(httpProvider, &IDataProvider::dataReady, table, &TableWidget::loadData);
    httpProvider->fetchData();
#else
    auto *dataProvider = new SampleDataProvider(20, table);
    QObject::connect(dataProvider, &IDataProvider::dataReady, table, &TableWidget::loadData);
    dataProvider->fetchData();
#endif

    auto *keyboard = new VirtualKeyboard(tablePage);
    keyboard->setVisible(false);
    tableLayout->addWidget(keyboard);
    QObject::connect(table, &TableWidget::cellEditingStarted,
                     keyboard, [keyboard](int, int) { keyboard->setVisible(true); });
    QObject::connect(table, &TableWidget::cellEditingFinished,
                     keyboard, [keyboard]() { keyboard->setVisible(false); });
    QObject::connect(keyboard, &VirtualKeyboard::keyPressed,
                     table, &TableWidget::handleKeyInput);

    // Waveform tab
    auto *wavePage = new QWidget;
    auto *waveLayout = new QVBoxLayout(wavePage);
    waveLayout->setContentsMargins(10, 10, 10, 10);
    auto *ctrlBar = new QHBoxLayout();
    auto *waveCombo = new QComboBox();
    waveCombo->addItems({QStringLiteral("正弦波"), QStringLiteral("方波"),
                         QStringLiteral("三角波"), QStringLiteral("锯齿波"),
                         QStringLiteral("脉冲波"), QStringLiteral("扫频波"),
                         QStringLiteral("CSV 自定义波形")});
    auto *freqCombo = new QComboBox();
    freqCombo->addItems({"0.5 Hz", "1.0 Hz", "2.0 Hz", "5.0 Hz"});
    freqCombo->setCurrentIndex(1);
    ctrlBar->addWidget(new QLabel(QStringLiteral("波形:")));
    ctrlBar->addWidget(waveCombo);
    ctrlBar->addWidget(new QLabel(QStringLiteral("  频率:")));
    ctrlBar->addWidget(freqCombo);
    ctrlBar->addStretch();
    waveLayout->addLayout(ctrlBar);

    // ---- 参数区（按波形类型动态显隐） ----
    auto *paramGrid = new QGridLayout;
    auto addParam = [&](const QString &label, QWidget *w, int row) {
        auto *lbl = new QLabel(label);
        paramGrid->addWidget(lbl, row, 0);
        paramGrid->addWidget(w, row, 1);
    };

    auto *ampSpin    = new QDoubleSpinBox; ampSpin->setRange(0.1, 10.0);   ampSpin->setValue(2.5);  ampSpin->setSingleStep(0.5);
    auto *offsetSpin = new QDoubleSpinBox; offsetSpin->setRange(-5.0, 5.0); offsetSpin->setValue(0.0); offsetSpin->setSingleStep(0.5);
    auto *phaseSpin  = new QDoubleSpinBox; phaseSpin->setRange(-180.0, 180.0); phaseSpin->setValue(0.0); phaseSpin->setSingleStep(15.0);
    auto *dutySpin   = new QDoubleSpinBox; dutySpin->setRange(0.05, 0.95); dutySpin->setValue(0.5);  dutySpin->setSingleStep(0.05);
    auto *freqEndSpin= new QDoubleSpinBox; freqEndSpin->setRange(1.0, 100.0); freqEndSpin->setValue(10.0); freqEndSpin->setSingleStep(1.0);
    auto *sampleCombo = new QComboBox();
    sampleCombo->addItems({QStringLiteral("500 点/秒"), QStringLiteral("5k 点/秒"),
                           QStringLiteral("50k 点/秒"), QStringLiteral("200k 点/秒")});
    sampleCombo->setCurrentIndex(0);

    addParam(QStringLiteral("幅值"), ampSpin, 0);
    addParam(QStringLiteral("偏置"), offsetSpin, 1);
    addParam(QStringLiteral("相位(°)"), phaseSpin, 2);
    addParam(QStringLiteral("占空比"), dutySpin, 3);
    addParam(QStringLiteral("扫频结束频率(Hz)"), freqEndSpin, 4);
    addParam(QStringLiteral("采样率/数据量"), sampleCombo, 5);
    waveLayout->addLayout(paramGrid);

    auto *csvLoadBtn = new QPushButton(QStringLiteral("加载 CSV…"));
    csvLoadBtn->setVisible(false);
    auto *csvPathLabel = new QLabel(QStringLiteral("未选择文件"));
    csvPathLabel->setVisible(false);
    csvPathLabel->setWordWrap(true);
    auto *csvBar = new QHBoxLayout();
    csvBar->addWidget(csvLoadBtn);
    csvBar->addWidget(csvPathLabel, 1);
    waveLayout->addLayout(csvBar);

    auto *chart = new WaveformChart();
    waveLayout->addWidget(chart, 1);

    // ---- 多线程练习：后台线程生成 vs 同步生成 ----
    auto *modeCombo = new QComboBox();
    modeCombo->addItems({QStringLiteral("后台线程生成（推荐）"),
                         QStringLiteral("同步生成（主线程，会卡顿）")});
    auto *delaySlider = new QSlider(Qt::Horizontal);
    delaySlider->setRange(0, 3000);
    delaySlider->setValue(500);
    delaySlider->setSingleStep(50);
    auto *delayValueLabel = new QLabel(QStringLiteral("500 ms"));
    auto *statusLabel = new QLabel(QStringLiteral("就绪"));

    auto *modeBar = new QHBoxLayout();
    modeBar->addWidget(new QLabel(QStringLiteral("生成方式:")));
    modeBar->addWidget(modeCombo);
    modeBar->addWidget(new QLabel(QStringLiteral("  模拟耗时:")));
    modeBar->addWidget(delaySlider, 1);
    modeBar->addWidget(delayValueLabel);
    modeBar->addStretch();
    waveLayout->addLayout(modeBar);
    waveLayout->addWidget(statusLabel);

    // 波形类型 → 需要动态显示的控件
    auto paramWidgets = std::map<QString, QList<QWidget*>>{
        {QStringLiteral("正弦波"), {ampSpin, offsetSpin, phaseSpin}},
        {QStringLiteral("方波"),   {ampSpin, offsetSpin, phaseSpin}},
        {QStringLiteral("三角波"), {ampSpin, offsetSpin, phaseSpin}},
        {QStringLiteral("锯齿波"), {ampSpin, offsetSpin, phaseSpin}},
        {QStringLiteral("脉冲波"), {ampSpin, offsetSpin, phaseSpin, dutySpin}},
        {QStringLiteral("扫频波"), {ampSpin, offsetSpin, phaseSpin, freqEndSpin}},
        {QStringLiteral("CSV 自定义波形"), {}},
    };
    // 采样率对所有非 CSV 波形都可用
    auto nonCsvWidgets = QList<QWidget*>{sampleCombo};

    // CSV 波形文件路径（由「加载 CSV…」按钮选择，跨 lambda 共享）
    auto csvPath = std::make_shared<QString>();

    auto collectParams = [=]() -> WaveParams {
        WaveParams p;
        const QString name = waveCombo->currentText();
        if (name == QStringLiteral("正弦波"))      p.type = WaveType::Sine;
        else if (name == QStringLiteral("方波"))    p.type = WaveType::Square;
        else if (name == QStringLiteral("三角波"))  p.type = WaveType::Triangle;
        else if (name == QStringLiteral("锯齿波"))  p.type = WaveType::Sawtooth;
        else if (name == QStringLiteral("脉冲波"))  p.type = WaveType::Pulse;
        else if (name == QStringLiteral("扫频波"))  p.type = WaveType::Sweep;
        else if (name == QStringLiteral("CSV 自定义波形")) p.type = WaveType::Csv;
        else                                        p.type = WaveType::Sine;
        p.frequency    = freqCombo->currentText().split(' ')[0].toDouble();
        p.freqEnd      = freqEndSpin->value();
        p.amplitude    = ampSpin->value();
        p.offset       = offsetSpin->value();
        p.phaseDeg     = phaseSpin->value();
        p.dutyCycle    = dutySpin->value();
        p.sampleRate   = sampleCombo->currentText().split(' ')[0].toDouble();
        if (p.sampleRate == 500.0) p.sampleRate = 500.0;   // "500" 直接解析
        p.sweepDurationSec = 2.0;
        p.csvPath = *csvPath;
        return p;
    };

    auto updateParamVisibility = [=]() {
        const QString name = waveCombo->currentText();
        const bool csv = (name == QStringLiteral("CSV 自定义波形"));
        // 隐藏参数网格中所有行，再按需显示
        for (QWidget *w : {static_cast<QWidget*>(ampSpin), static_cast<QWidget*>(offsetSpin),
                           static_cast<QWidget*>(phaseSpin), static_cast<QWidget*>(dutySpin),
                           static_cast<QWidget*>(freqEndSpin), static_cast<QWidget*>(sampleCombo)})
            w->setVisible(false);
        csvLoadBtn->setVisible(csv);
        csvPathLabel->setVisible(csv);
        if (csv) return;
        for (auto *w : paramWidgets.at(name)) w->setVisible(true);
        for (auto *w : nonCsvWidgets) w->setVisible(true);
    };

    // 后台工作线程：对象 moveToThread 后，槽在子线程执行，信号经队列回主线程
    auto *workerThread = new QThread;
    auto *generator = new WaveformGenerator;   // 无父对象，退出时手动释放
    generator->moveToThread(workerThread);
    workerThread->start();

    // 共享请求序号：快速切换时据此丢弃过期结果
    auto seq = std::make_shared<int>(0);

    auto requestGeneration = [=]() {
        // CSV 类型且未选择文件：提示并跳过生成
        if (waveCombo->currentText() == QStringLiteral("CSV 自定义波形") && csvPath->isEmpty()) {
            statusLabel->setText(QStringLiteral("请先点击「加载 CSV…」选择波形文件"));
            return;
        }
        const int myId = ++(*seq);
        const WaveParams params = collectParams();
        const int simulateMs = delaySlider->value();
        const bool async = modeCombo->currentIndex() == 0;

        // 标题属 UI 状态，只在主线程设置
        QString title = waveCombo->currentText();
        if (params.type != WaveType::Csv)
            title += QStringLiteral(" %1 Hz").arg(params.frequency);
        chart->setTitle(title);

        // 参数摘要
        const QString summary = QStringLiteral("%1 %2Hz A=%3 偏置=%4 相位=%5° 采样=%6")
            .arg(title).arg(params.frequency).arg(params.amplitude)
            .arg(params.offset).arg(params.phaseDeg)
            .arg(sampleCombo->currentText());
        statusLabel->setText(summary + QStringLiteral(" | 生成中…"));

        if (async) {
            // 跨线程投递到 worker 槽：自定义类型 WaveParams 在投递时拷贝
            QMetaObject::invokeMethod(generator, "generate", Qt::QueuedConnection,
                Q_ARG(int, myId), Q_ARG(WaveParams, params), Q_ARG(int, simulateMs));
        } else {
            statusLabel->setText(summary + QStringLiteral(" | 生成中…（主线程，UI 将卡顿）"));
            QElapsedTimer timer;
            timer.start();
            if (simulateMs > 0) QThread::msleep(static_cast<unsigned long>(simulateMs));
            WaveformData data;
            if (WaveformGenerator::buildWaveform(params, data)) {
                chart->setData(data);
                statusLabel->setText(summary + QStringLiteral(" | 完成：%1 ms，共 %2 点")
                                     .arg(timer.elapsed()).arg(data.count()));
            } else {
                statusLabel->setText(summary + QStringLiteral(" | 同步生成失败"));
            }
        }
    };

    // 结果回主线程（wavePage 为接收上下文，QueuedConnection 自动生效）
    QObject::connect(generator, &WaveformGenerator::waveReady, wavePage,
                     [=](int id, const WaveformData &data) {
        if (id != *seq) {   // 过期结果：后发请求先完成时丢弃
            statusLabel->setText(QStringLiteral("已丢弃过期结果（请求 %1，当前 %2）").arg(id).arg(*seq));
            return;
        }
        chart->setData(data);
        statusLabel->setText(QStringLiteral("完成：共 %1 点").arg(data.count()));
    });
    QObject::connect(generator, &WaveformGenerator::waveFailed, wavePage,
                     [=](int id, const QString &msg) {
        if (id != *seq) return;
        statusLabel->setText(QStringLiteral("后台生成失败：%1").arg(msg));
    });

    QObject::connect(waveCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), updateParamVisibility);
    QObject::connect(waveCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), requestGeneration);
    QObject::connect(freqCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), requestGeneration);
    QObject::connect(modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), requestGeneration);
    QObject::connect(delaySlider, &QSlider::sliderReleased, requestGeneration);
    QObject::connect(sampleCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), requestGeneration);
    for (auto *w : {ampSpin, offsetSpin, phaseSpin, dutySpin, freqEndSpin})
        QObject::connect(w, QOverload<double>::of(&QDoubleSpinBox::valueChanged), requestGeneration);
    QObject::connect(delaySlider, &QSlider::valueChanged, delayValueLabel,
                     [=](int v) { delayValueLabel->setText(QStringLiteral("%1 ms").arg(v)); });
    QObject::connect(csvLoadBtn, &QPushButton::clicked, wavePage, [=]() {
        const QString path = QFileDialog::getOpenFileName(
            wavePage, QStringLiteral("加载 CSV 波形"), QString(),
            QStringLiteral("CSV 文件 (*.csv);;所有文件 (*)"));
        if (path.isEmpty()) return;
        *csvPath = path;
        csvPathLabel->setText(path);
        requestGeneration();
    });
    updateParamVisibility();
    requestGeneration();

    auto *tabs = new QTabWidget;
    auto *basicTools = new BasicToolsWidget(tablePage, wavePage, new FileManagerWidget);
    auto *arbWidget = new ArbWidget;
    auto *fiveGWidget = new FiveGNrWidget;
    tabs->addTab(basicTools, QStringLiteral("基础工具"));
    tabs->addTab(arbWidget, QStringLiteral("ARB 任意波形"));
    tabs->addTab(fiveGWidget, QStringLiteral("5G NR 信号配置"));

    // 数字调制 QML 界面（对应设计稿 vsg_digital modulation.html，嵌入 QQuickWidget）
    auto *digitalModWidget = new QQuickWidget;
    digitalModWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    digitalModWidget->setClearColor(QColor(QStringLiteral("#f0f3f7")));
    auto *toastService = new ToastService(digitalModWidget);
    digitalModWidget->engine()->rootContext()->setContextProperty(
        QStringLiteral("ToastService"), toastService);
    digitalModWidget->setSource(QUrl(QStringLiteral("qrc:/qml/VsgApp.qml")));
    tabs->addTab(digitalModWidget, QStringLiteral("数字调制"));

    auto *window = new QWidget;
    window->setWindowTitle(QStringLiteral("Qt Demo"));
    window->resize(1050, 760);
    // main_tabs.qss 含全局标题栏 + Tab 样式，需挂在 window 上才能覆盖 AppHeaderBar
    QFile tabsStyle(QStringLiteral(":/main_tabs.qss"));
    if (tabsStyle.open(QIODevice::ReadOnly))
        window->setStyleSheet(QString::fromUtf8(tabsStyle.readAll()));
    else
        spdlog::warn("main_ui: failed to load :/main_tabs.qss");
    auto *mainLayout = new QVBoxLayout(window);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // 全局标题栏置于 Tab 导航栏之上（以 Arb createHeader 为基准）
    auto *appHeader = new AppHeaderBar(window);
    mainLayout->addWidget(appHeader);
    mainLayout->addWidget(tabs, 1);

    // 全局 RF 开关同步到 ARB 界面
    QObject::connect(appHeader, &AppHeaderBar::rfToggled, arbWidget,
                     &ArbWidget::setRfEnabled);
    QObject::connect(arbWidget, &ArbWidget::rfToggled, appHeader,
                     &AppHeaderBar::setRfEnabled);

    window->show();
    const int ret = app.exec();

    // 优雅退出工作线程：停止事件循环 → 等待 → 释放对象
    workerThread->quit();
    workerThread->wait();
    delete generator;
    delete workerThread;
    LogUtil::shutdown();
    return ret;
}
