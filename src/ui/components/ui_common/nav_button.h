#ifndef NAV_BUTTON_H
#define NAV_BUTTON_H

#include <QPushButton>

/**
 * @brief 导航按钮组件（带激活状态）
 *
 * 用途：显示导航步骤，支持激活/非激活状态
 * 示例：
 *   auto *btn = new NavButton("载波", this);
 *   btn->setActive(true);
 */
class NavButton : public QPushButton {
    Q_OBJECT

public:
    explicit NavButton(const QString &text, QWidget *parent = nullptr);

    /**
     * @brief 设置激活状态
     * @param active 是否激活
     */
    void setActive(bool active);

    /**
     * @brief 获取激活状态
     */
    bool isActive() const { return m_active; }

private:
    bool m_active;
};

#endif // NAV_BUTTON_H
