#ifndef STAT_ROW_H
#define STAT_ROW_H

#include <QWidget>
#include <QLabel>

/**
 * @brief 统计行组件（数值 + 单位 + 标签）
 *
 * 用途：显示状态信息（CPU、内存、频率、功率等）
 * 示例：
 *   auto *row = new StatRow("CPU", "45", "%", this);
 */
class StatRow : public QWidget {
    Q_OBJECT

public:
    explicit StatRow(const QString &label, const QString &value,
                     const QString &unit = QString(), QWidget *parent = nullptr);

    /**
     * @brief 更新数值
     * @param value 新数值
     */
    void setValue(const QString &value);

    /**
     * @brief 设置数值样式（正常/警告/错误）
     * @param style 样式类型
     */
    enum class Style {
        Normal,      // 正常
        Warning,     // 警告
        Error        // 错误
    };
    void setStyle(Style style);

    QLabel *valueLabel() const { return m_value; }

private:
    QHBoxLayout *m_layout;
    QLabel *m_label;
    QLabel *m_value;
    QLabel *m_unit;
};

#endif // STAT_ROW_H
