#ifndef FORKED_HTTP_DATA_PROVIDER_H
#define FORKED_HTTP_DATA_PROVIDER_H

#include <QByteArray>
#include <QUrl>

#include "IDataProvider.h"

class QProcess;
class QTimer;

/**
 * @brief 基于子进程的 HTTP 数据提供者（多进程练习）
 *
 * 与 HttpDataProvider 在主进程内用 QNetworkAccessManager 抓取不同，
 * 本类把「网络请求 + JSON 解析」整体交给子进程 fetch_table_child 执行:
 *
 *   父进程(Qt UI): QProcess 启动子进程 → 读 stdout → 还原 TableData → dataReady
 *   子进程      : QNetworkAccessManager 抓取 + QJsonDocument 解析，
 *                 解析崩溃只影响子进程，UI 不受影响
 *
 * 练习要点:
 *   - 进程隔离（子进程崩溃不拖垮 UI）
 *   - 父子进程通过管道(stdout)通信，通过退出码约定成败
 *   - 超时保护（父进程可强杀子进程）
 */
class ForkedHttpDataProvider : public IDataProvider {
    Q_OBJECT

public:
    explicit ForkedHttpDataProvider(const QUrl &url, QObject *parent = nullptr);
    ~ForkedHttpDataProvider() override;

    void fetchData() override;

    void setChildTimeoutMs(int ms) { m_timeoutMs = ms; }

private:
    void onStdoutReady();
    void onProcessFinished(int exitCode);
    void onTimeout();
    TableData parseNormalizedJson(const QByteArray &json) const;

    /// 定位子进程可执行文件（环境变量覆盖 > 应用目录相对路径）
    static QString resolveChildPath();

    QUrl        m_url;
    QProcess   *m_proc     = nullptr;
    QTimer     *m_timer    = nullptr;
    QByteArray  m_stdout;
    int         m_timeoutMs = 5000;
    bool        m_timedOut  = false;
};

#endif // FORKED_HTTP_DATA_PROVIDER_H
