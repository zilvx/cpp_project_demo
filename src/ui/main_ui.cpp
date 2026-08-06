#include <QApplication>
#include <QComboBox>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QHBoxLayout>
#include <QLabel>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QWidget>

#include "widgets/TableWidget.h"
#include "widgets/VirtualKeyboard.h"
#include "widgets/WaveformChart.h"
#include "core/WaveformData.h"
#include "providers/SampleDataProvider.h"
#include "providers/HttpDataProvider.h"
#ifdef USE_PROTO_DATA
#include "providers/HttpProtoDataProvider.h"
#endif

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

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
                     [](const QString &msg) { qWarning().noquote() << "Proto HTTP Error:" << msg; });
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
                         QStringLiteral("锯齿波"), QStringLiteral("CSV 自定义波形")});
    auto *freqCombo = new QComboBox();
    freqCombo->addItems({"0.5 Hz", "1.0 Hz", "2.0 Hz", "5.0 Hz"});
    freqCombo->setCurrentIndex(1);
    ctrlBar->addWidget(new QLabel(QStringLiteral("波形:")));
    ctrlBar->addWidget(waveCombo);
    ctrlBar->addWidget(new QLabel(QStringLiteral("  频率:")));
    ctrlBar->addWidget(freqCombo);
    ctrlBar->addStretch();
    waveLayout->addLayout(ctrlBar);
    auto *chart = new WaveformChart();
    waveLayout->addWidget(chart, 1);

    auto updateWave = [=]() {
        WaveformData data;
        const int idx = waveCombo->currentIndex();
        const double freq = freqCombo->currentText().split(' ')[0].toDouble();
        switch (idx) {
        case 0: data.generateSine(freq, 2.5, 500.0, 3); chart->setTitle(QStringLiteral("正弦波 %1 Hz").arg(freq)); break;
        case 1: data.generateSquare(freq, 2.5, 500.0, 3); chart->setTitle(QStringLiteral("方波 %1 Hz").arg(freq)); break;
        case 2: data.generateSawtooth(freq, 2.5, 500.0, 3); chart->setTitle(QStringLiteral("锯齿波 %1 Hz").arg(freq)); break;
        case 3: {
            QString csvPath = QStringLiteral("data/arbitrary_wave.csv");
            if (!QFile::exists(csvPath))
                csvPath = QCoreApplication::applicationDirPath() + QStringLiteral("/../../../../data/arbitrary_wave.csv");
            data.loadCSV(csvPath);
            chart->setTitle(QStringLiteral("CSV 自定义波形"));
            break;
        }
        }
        chart->setData(data);
    };
    QObject::connect(waveCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), updateWave);
    QObject::connect(freqCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), updateWave);
    updateWave();

    auto *tabs = new QTabWidget;
    tabs->addTab(tablePage, QStringLiteral("人员信息表"));
    tabs->addTab(wavePage, QStringLiteral("波形示波器"));
    auto *window = new QWidget;
    window->setWindowTitle(QStringLiteral("Qt Demo"));
    window->resize(1050, 760);
    auto *mainLayout = new QVBoxLayout(window);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(tabs);
    window->show();
    return app.exec();
}
