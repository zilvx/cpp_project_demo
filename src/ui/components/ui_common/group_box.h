#ifndef GROUP_BOX_H
#define GROUP_BOX_H

#include <QGroupBox>

/**
 * @brief 带徽标的分组框
 *
 * 用途：组织和分类UI元素，类似于QGroupBox但支持badge
 * 示例：
 *   auto *gb = new GroupBox("载波配置", "1", this);
 */
class GroupBox : public QGroupBox {
    Q_OBJECT

public:
    explicit GroupBox(const QString &title, QWidget *parent = nullptr);

    /**
     * @brief 设置徽标文本
     * @param text 徽标内容（如步骤编号、警告标记等）
     */
    void setBadge(const QString &text);

    /**
     * @brief 设置徽标样式
     * @param color 徽标颜色
     */
    void setBadgeColor(const QColor &color = QColor("#2a6f9c"));

private:
    QLabel *m_badge;
};

#endif // GROUP_BOX_H
