// fetch_table_child — ForkedHttpDataProvider 的子进程程序（多进程练习）
//
// 职责:
//   把「网络请求 + JSON 解析」整体搬到子进程里执行。父进程(Qt UI)只负责
//   启动本程序并读取 stdout，解析崩溃只会发生在子进程，不会拖垮 UI。
//
// 用法:
//   fetch_table_child <url> [timeoutMs]
//
// 进程间约定:
//   成功 → stdout 输出规范化后的紧凑 JSON（已解析过），退出码 0
//   失败 → stderr 输出原因，退出码 1（网络/解析错误）
//   超时 → stderr 输出原因，退出码 2
//   用法错 → 退出码 3
//
// 关键点:
//   子进程里不能依赖父进程的任何 Qt 对象（fork/exec 之后是全新进程）。
//   本程序自建 QCoreApplication 事件循环，靠 QNetworkAccessManager 异步抓取。

#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>

#include <cstdio>
#include <spdlog/spdlog.h>

#include "logutil.h"

namespace {

// 把整块数据一次性写入 stdout（子进程输出走的是 QProcess 管道）
bool writeAllStdout(const QByteArray &data) {
    return fwrite(data.constData(), 1, static_cast<size_t>(data.size()), stdout)
               == static_cast<size_t>(data.size())
           && fflush(stdout) == 0;
}

} // namespace

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    // stdout 承载 JSON 协议，控制台日志必须走 stderr；独立文件避免与父进程抢同一 log
    LogUtil::init("fetch_table_child", "", 1024 * 1024 * 5, 3, /*use_stderr=*/true);

    if (argc < 2) {
        spdlog::error("用法: fetch_table_child <url> [timeoutMs]");
        LogUtil::shutdown();
        return 3;
    }

    const QString urlStr  = QString::fromLocal8Bit(argv[1]);
    const int     timeout = (argc >= 3) ? QString::fromLocal8Bit(argv[2]).toInt() : 5000;
    const QUrl    url(urlStr);
    if (!url.isValid()) {
        spdlog::error("无效 URL: {}", argv[1]);
        LogUtil::shutdown();
        return 3;
    }

    spdlog::info("抓取开始 url={} timeoutMs={}", urlStr.toStdString(), timeout);

    auto *nam   = new QNetworkAccessManager;
    auto *reply = nam->get(QNetworkRequest(url));

    // 超时标记：abort() 也会触发 finished，用标志位避免双路退出
    bool timedOut = false;

    QObject::connect(reply, &QNetworkReply::finished, [&]() {
        if (timedOut) return;
        reply->deleteLater();
        nam->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            spdlog::error("HTTP 错误: {}", reply->errorString().toStdString());
            LogUtil::shutdown();
            QCoreApplication::exit(1);
            return;
        }

        const QByteArray body = reply->readAll();

        // 真正的解析动作发生在子进程；解析崩溃只影响本进程
        QJsonParseError perr;
        const QJsonDocument doc = QJsonDocument::fromJson(body, &perr);
        if (perr.error != QJsonParseError::NoError || !doc.isObject()) {
            spdlog::error("JSON 解析失败 offset={}: {}", perr.offset,
                          perr.errorString().toStdString());
            LogUtil::shutdown();
            QCoreApplication::exit(1);
            return;
        }

        // 回传已经解析成功的规范化 JSON（父进程只需做受信任的结构还原）
        const QByteArray out = doc.toJson(QJsonDocument::Compact);
        if (!writeAllStdout(out)) {
            spdlog::error("写入 stdout 失败");
            LogUtil::shutdown();
            QCoreApplication::exit(1);
            return;
        }
        spdlog::info("抓取成功 bytes={}", out.size());
        LogUtil::shutdown();
        QCoreApplication::exit(0);
    });

    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&timer, &QTimer::timeout, [&]() {
        timedOut = true;
        reply->abort();
        spdlog::error("超时 {} ms", timeout);
        LogUtil::shutdown();
        QCoreApplication::exit(2);
    });
    timer.start(timeout);

    return app.exec();
}
