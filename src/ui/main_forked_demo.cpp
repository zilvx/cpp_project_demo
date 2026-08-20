// qt_table_app_fork — ForkedHttpDataProvider 的演示入口（多进程练习）
//
// 与 qt_table_app 的区别:
//   qt_table_app     : 主进程内直接 QNetworkAccessManager + 解析（HttpDataProvider）
//   qt_table_app_fork: 网络请求与解析放到子进程 fetch_table_child，
//                      父进程通过 QProcess 管道接收结果
//
// 运行前先启动后端:  ./build/table_data_server 8080
// 然后:              open build/qt_table_app_fork.app
// 按下「刷新」按钮可以反复触发子进程抓取，方便对照观察（活动监视器可见子进程）。
//
// URL 可用环境变量 FORKED_DATA_URL 覆盖。

#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

#include <spdlog/spdlog.h>
#include "../utils/logging/logutil.h"

#include "widgets/TableWidget.h"
#include "providers/ForkedHttpDataProvider.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    LogUtil::init("qt_table_app_fork");

    QUrl url(QString::fromLocal8Bit(qgetenv("FORKED_DATA_URL")));
    if (!url.isValid())
        url = QUrl(QStringLiteral("http://localhost:8080/api/table"));

    auto *window  = new QWidget;
    auto *layout  = new QVBoxLayout(window);
    auto *status  = new QLabel(QStringLiteral("等待首次加载..."));
    auto *refresh = new QPushButton(QStringLiteral("重新抓取（启动子进程）"));
    auto *table   = new TableWidget(window);

    auto *topBar = new QHBoxLayout;
    topBar->addWidget(status, 1);
    topBar->addWidget(refresh);
    layout->addLayout(topBar);
    layout->addWidget(table, 1);

    auto *provider = new ForkedHttpDataProvider(url, table);
    QObject::connect(provider, &IDataProvider::dataReady,
                     table, &TableWidget::loadData);
    QObject::connect(provider, &IDataProvider::dataReady,
                     [status](const TableData &d) {
        status->setText(QStringLiteral("已从子进程取回 %1 行 × %2 列")
                            .arg(d.rowCount()).arg(d.columnCount()));
    });
    QObject::connect(provider, &IDataProvider::errorOccurred,
                     [status](const QString &msg) {
        status->setText(QStringLiteral("失败: %1").arg(msg));
        spdlog::error("{}", msg.toStdString());
    });
    QObject::connect(refresh, &QPushButton::clicked, provider, &IDataProvider::fetchData);

    provider->fetchData();

    window->setWindowTitle(QStringLiteral("Qt Demo — 多进程数据抓取"));
    window->resize(800, 600);
    window->show();

    const int ret = app.exec();
    LogUtil::shutdown();
    return ret;
}
