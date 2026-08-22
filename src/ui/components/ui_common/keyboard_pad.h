#ifndef KEYBOARD_PAD_H
#define KEYBOARD_PAD_H

#include <QWidget>
#include <QPushButton>
#include <QGridLayout>

/**
 * @brief 键盘垫组件（97键机械键盘布局）
 *
 * 用途：提供虚拟键盘输入界面
 * 示例：
 *   auto *kbd = new KeyboardPad(this);
 *   connect(kbd, &KeyboardPad::keyPressed, this, &MyWidget::handleKeyPress);
 */
class KeyboardPad : public QWidget {
    Q_OBJECT

public:
    explicit KeyboardPad(QWidget *parent = nullptr);

    /**
     * @brief 设置是否显示控制键（Shift、Ctrl等）
     * @param show 是否显示
     */
    void setShowControlKeys(bool show);

signals:
    void keyPressed(const QString &key);

private:
    QGridLayout *m_layout;
    QPushButton *m_keys[96];  // 96个键位
    int m_currentRow;
    int m_currentCol;
};

#endif // KEYBOARD_PAD_H
