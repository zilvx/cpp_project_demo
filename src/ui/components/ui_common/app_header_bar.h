#ifndef APP_HEADER_BAR_H
#define APP_HEADER_BAR_H

#include <QWidget>

class QPushButton;

/**
 * @brief 全局应用标题栏（以 ArbWidget 标题栏为基准）
 *
 * 置于主 Tab 导航栏之上，各子界面不再各自绘制标题栏。
 */
class AppHeaderBar : public QWidget {
    Q_OBJECT

public:
    explicit AppHeaderBar(QWidget *parent = nullptr);

    QPushButton *rfToggleButton() const { return m_rfToggleBtn; }
    bool isRfEnabled() const;
    void setRfEnabled(bool enabled);

signals:
    void rfToggled(bool enabled);

private:
    void setupUI();

    QPushButton *m_rfToggleBtn = nullptr;
};

#endif // APP_HEADER_BAR_H
