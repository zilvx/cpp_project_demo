#ifndef VIRTUAL_KEYBOARD_H
#define VIRTUAL_KEYBOARD_H

#include <QWidget>
#include <QPushButton>
#include <QVector>

/**
 * @brief 98 键机械键盘布局虚拟键盘组件
 *
 * 1800 紧凑布局，包含 F 键区、主键区、方向键、导航键。
 * 按键点击通过 keyPressed(const QString &key) 信号发出。
 *
 * 用法:
 *   auto *kb = new VirtualKeyboard(parent);
 *   connect(kb, &VirtualKeyboard::keyPressed, this, &MyWidget::onKey);
 */
class VirtualKeyboard : public QWidget {
    Q_OBJECT

public:
    explicit VirtualKeyboard(QWidget *parent = nullptr);

signals:
    /// 按键被点击时发出，key 为按键标识（单字符或功能键名）
    void keyPressed(const QString &key);

private:
    void setupUI();
    void loadStyleSheet();

    QPushButton *createKey(const QString &text,
                           float widthUnits = 1.0f,
                           const QString &keyValue = QString());
};

#endif // VIRTUAL_KEYBOARD_H
