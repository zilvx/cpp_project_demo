#ifndef PARAM_ROW_H
#define PARAM_ROW_H

#include <QWidget>
#include <QLabel>

/**
 * @brief 参数行组件（标签 + 控件 + 单位）
 *
 * 用途：统一参数输入的UI展示，用于载波配置、波形参数等
 * 示例：
 *   auto *row = new ParamRow("频率", spinBox, "MHz", this);
 */
class ParamRow : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief 构造参数行
     * @param label 参数标签（如"频率"、"功率"）
     * @param control 控件（QDoubleSpinBox, QLineEdit, QComboBox等）
     * @param unit 单位显示（可选，如"MHz"、"dBm"）
     * @param parent 父控件
     */
    explicit ParamRow(const QString &label, QWidget *control,
                      const QString &unit = QString(), QWidget *parent = nullptr);

    /**
     * @brief 设置控件的可读性样式
     * @param readonly 是否只读
     * @param error 是否错误状态
     */
    void setControlStyle(bool readonly = false, bool error = false);

    QWidget *control() const { return m_control; }
    QLabel *label() const { return m_label; }
    QLabel *unit() const { return m_unit; }

private:
    QHBoxLayout *m_layout;
    QLabel *m_label;
    QWidget *m_control;
    QLabel *m_unit;
};

#endif // PARAM_ROW_H
