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

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // ====== Tab 1: 表格 + 虚拟键盘 ======
    auto *tablePage = new QWidget;
    auto *tableLayout = new QVBoxLayout(tablePage);
    tableLayout->setContentsMargins(0, 0, 0, 0);
    tableLayout->setSpacing(0);

    auto *table = new TableWidget(20, 6, tablePage);
    tableLayout->addWidget(table);

    auto *keyboard = new VirtualKeyboard(tablePage);
    keyboard->setVisible(false);
    tableLayout->addWidget(keyboard);

    QObject::connect(table, &TableWidget::cellEditingStarted,
                     keyboard, [keyboard](int, int) { keyboard->setVisible(true); });
    QObject::connect(table, &TableWidget::cellEditingFinished,
                     keyboard, [keyboard]() { keyboard->setVisible(false); });
    QObject::connect(keyboard, &VirtualKeyboard::keyPressed,
                     table, &TableWidget::handleKeyInput);

    // ====== Tab 2: 波形示波器 ======
    auto *wavePage = new QWidget;
    auto *waveLayout = new QVBoxLayout(wavePage);
    waveLayout->setContentsMargins(10, 10, 10, 10);

    // 控制栏
    auto *ctrlBar = new QHBoxLayout();
    auto *waveLabel = new QLabel(QStringLiteral("波形类型:"));
    auto *waveCombo = new QComboBox();
    waveCombo->addItems({
        QStringLiteral("正弦波 (Sine)"),
        QStringLiteral("方波 (Square)"),
        QStringLiteral("锯齿波 (Sawtooth)"),
        QStringLiteral("CSV 自定义波形"),
    });

    auto *freqLabel = new QLabel(QStringLiteral("  频率:"));
    auto *freqCombo = new QComboBox();
    freqCombo->addItems({"0.5 Hz", "1.0 Hz", "2.0 Hz", "5.0 Hz"});
    freqCombo->setCurrentIndex(1);

    ctrlBar->addWidget(waveLabel);
    ctrlBar->addWidget(waveCombo);
    ctrlBar->addWidget(freqLabel);
    ctrlBar->addWidget(freqCombo);
    ctrlBar->addStretch();
    waveLayout->addLayout(ctrlBar);

    auto *chart = new WaveformChart();
    chart->setXLabel(QStringLiteral("时间 (s)"));
    chart->setYLabel(QStringLiteral("幅值 (V)"));
    waveLayout->addWidget(chart, 1);

    // 波形切换逻辑
    auto updateWave = [=]() {
        WaveformData data;
        const int idx = waveCombo->currentIndex();
        const double freq = freqCombo->currentText().split(' ')[0].toDouble();

        switch (idx) {
        case 0:
            data.generateSine(freq, 2.5, 500.0, 3);
            chart->setTitle(QStringLiteral("正弦波 %1 Hz").arg(freq));
            break;
        case 1:
            data.generateSquare(freq, 2.5, 500.0, 3);
            chart->setTitle(QStringLiteral("方波 %1 Hz").arg(freq));
            break;
        case 2:
            data.generateSawtooth(freq, 2.5, 500.0, 3);
            chart->setTitle(QStringLiteral("锯齿波 %1 Hz").arg(freq));
            break;
        case 3: {
            // CSV 路径：优先项目相对路径，回退到可执行文件相对路径
            QString csvPath = QStringLiteral("data/arbitrary_wave.csv");
            if (!QFile::exists(csvPath)) {
                csvPath = QCoreApplication::applicationDirPath()
                          + QStringLiteral("/../../../../data/arbitrary_wave.csv");
            }
            data.loadCSV(csvPath);
            chart->setTitle(QStringLiteral("CSV 自定义波形"));
            break;
        }
        }
        chart->setData(data);
    };

    QObject::connect(waveCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                     updateWave);
    QObject::connect(freqCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                     updateWave);

    updateWave();  // 初始显示

    // ====== 主窗口 ======
    auto *tabs = new QTabWidget;
    tabs->addTab(tablePage, QStringLiteral("人员信息表"));
    tabs->addTab(wavePage,  QStringLiteral("波形示波器"));

    auto *window = new QWidget;
    window->setWindowTitle(QStringLiteral("Qt Demo"));
    window->resize(1050, 760);
    auto *mainLayout = new QVBoxLayout(window);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(tabs);
    window->show();

    return app.exec();
}
