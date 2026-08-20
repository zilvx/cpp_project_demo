#include "ForkedHttpDataProvider.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QProcess>
#include <QTimer>

#include <spdlog/spdlog.h>

// ============================================================
//  构造 / 析构
// ============================================================

ForkedHttpDataProvider::ForkedHttpDataProvider(const QUrl &url, QObject *parent)
    : IDataProvider(parent), m_url(url) {
    m_proc = new QProcess(this);
    connect(m_proc, &QProcess::readyReadStandardOutput,
            this, &ForkedHttpDataProvider::onStdoutReady);
    connect(m_proc, &QProcess::finished,
            this, [this](int exitCode, QProcess::ExitStatus) {
        onProcessFinished(exitCode);
    });

    m_timer = new QTimer(this);
    m_timer->setSingleShot(true);
    connect(m_timer, &QTimer::timeout, this, &ForkedHttpDataProvider::onTimeout);
}

ForkedHttpDataProvider::~ForkedHttpDataProvider() {
    // 析构时若子进程还在跑，直接强杀，避免留下孤儿进程
    if (m_proc && m_proc->state() != QProcess::NotRunning) {
        m_proc->kill();
        m_proc->waitForFinished(2000);
    }
}

// ============================================================
//  抓取入口
// ============================================================

void ForkedHttpDataProvider::fetchData() {
    const QString childPath = resolveChildPath();
    if (childPath.isEmpty()) {
        spdlog::error("ForkedHttpDataProvider: 未找到子进程程序 fetch_table_child");
        emit errorOccurred(QStringLiteral(
            "未找到子进程程序 fetch_table_child（可用环境变量 FETCH_CHILD_PATH 指定）"));
        return;
    }

    m_stdout.clear();
    m_timedOut = false;

    const QStringList args = {
        m_url.toString(),
        QString::number(m_timeoutMs),
    };

    spdlog::info("ForkedHttpDataProvider: 启动子进程 {} {}", childPath.toStdString(),
                 m_url.toString().toStdString());
    m_proc->start(childPath, args);

    if (!m_proc->waitForStarted(3000)) {
        emit errorOccurred(QStringLiteral("子进程启动失败: %1").arg(m_proc->errorString()));
        return;
    }

    m_timer->start(m_timeoutMs);
}

// ============================================================
//  子进程输出 / 结束 / 超时
// ============================================================

void ForkedHttpDataProvider::onStdoutReady() {
    // 管道可能分批到达，先累积，等 finished 后统一解析
    m_stdout += m_proc->readAllStandardOutput();
}

void ForkedHttpDataProvider::onProcessFinished(int exitCode) {
    m_timer->stop();
    if (m_timedOut) return;  // 超时路径已处理过

    const QString stderrText =
        QString::fromUtf8(m_proc->readAllStandardError()).trimmed();

    if (exitCode != 0) {
        spdlog::warn("ForkedHttpDataProvider: 子进程退出码 {}: {}", exitCode,
                     stderrText.toStdString());
        emit errorOccurred(QStringLiteral("子进程退出码 %1: %2")
                               .arg(exitCode).arg(stderrText));
        return;
    }

    const TableData data = parseNormalizedJson(m_stdout);
    if (!data.isValid()) {
        spdlog::warn("ForkedHttpDataProvider: 子进程输出解析失败");
        emit errorOccurred(QStringLiteral("子进程输出解析失败"));
        return;
    }
    emit dataReady(data);
}

void ForkedHttpDataProvider::onTimeout() {
    m_timedOut = true;
    if (m_proc && m_proc->state() != QProcess::NotRunning) {
        spdlog::warn("ForkedHttpDataProvider: 子进程超时，正在强杀");
        m_proc->kill();
    }
    emit errorOccurred(QStringLiteral("子进程超时（%1 ms），已终止").arg(m_timeoutMs));
}

// ============================================================
//  数据还原（子进程输出的已是受信任的规范化 JSON）
// ============================================================

TableData ForkedHttpDataProvider::parseNormalizedJson(const QByteArray &json) const {
    TableData result;
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(json, &err);
    if (err.error != QJsonParseError::NoError) {
        spdlog::warn("ForkedHttpDataProvider: 规范化 JSON 解析错误: {}",
                     err.errorString().toStdString());
        return result;
    }
    const QJsonObject root = doc.object();

    const QJsonArray colArr = root.value(QStringLiteral("columns")).toArray();
    for (const auto &v : colArr) result.columns.append(v.toString());

    const QJsonArray editArr = root.value(QStringLiteral("editable")).toArray();
    for (const auto &v : editArr) result.editable.append(v.toBool());

    const QJsonArray rowArr = root.value(QStringLiteral("rows")).toArray();
    for (const auto &rv : rowArr) {
        const QJsonArray cellArr = rv.toArray();
        QStringList row;
        for (const auto &cv : cellArr) row.append(cv.toString());
        result.rows.append(row);
    }
    return result;
}

// ============================================================
//  子进程路径定位
// ============================================================

QString ForkedHttpDataProvider::resolveChildPath() {
    // 1) 环境变量显式指定（便于手动调整路径）
    const QByteArray env = qgetenv("FETCH_CHILD_PATH");
    if (!env.isEmpty()) return QString::fromUtf8(env);

    // 2) 相对应用目录的候选路径
    //    直接运行 build/qt_table_app_fork 时:   <appDir>/fetch_table_child
    //    .app bundle 里运行时(Contents/MacOS):  上溯 3 级即 build/
    const QString appDir = QCoreApplication::applicationDirPath();
    const QStringList candidates = {
        appDir + QStringLiteral("/fetch_table_child"),
        appDir + QStringLiteral("/../fetch_table_child"),
        appDir + QStringLiteral("/../../fetch_table_child"),
        appDir + QStringLiteral("/../../../fetch_table_child"),
        appDir + QStringLiteral("/../../../../fetch_table_child"),
    };
    for (const QString &c : candidates) {
        if (QFileInfo::exists(c)) return c;
    }
    return {};
}
