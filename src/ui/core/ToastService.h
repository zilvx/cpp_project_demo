#pragma once

#include <QObject>
#include <QString>

// 数字调制 QML 页面的 Toast 桥接（C++ → QML 单向通知）
// QML 通过 context property "ToastService" 调用 show()，界面侧监听 showRequested 弹出提示
class ToastService : public QObject {
    Q_OBJECT
public:
    explicit ToastService(QObject *parent = nullptr) : QObject(parent) {}

    Q_INVOKABLE void show(const QString &msg) { emit showRequested(msg); }

signals:
    void showRequested(const QString &msg);
};