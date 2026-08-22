#ifndef LCD_DISPLAY_H
#define LCD_DISPLAY_H

#include <QWidget>
#include <QLabel>
#include <QTimer>
#include <QHBoxLayout>
#include <functional>

/**
 * @brief LCD显示屏组件（模拟设备面板显示）
 *
 * 用途：显示数值（频率、功率、时间等）
 * 特性：滚动动画、固定小数位、单位显示
 * 示例：
 *   auto *lcd = new LCDDisplay("2.4", "GHz", this);
 *   lcd->setValue("2.45");
 */
class LCDDisplay : public QWidget {
    Q_OBJECT

public:
    explicit LCDDisplay(const QString &defaultValue, const QString &unit = "",
                       QWidget *parent = nullptr);

    /**
     * @brief 设置数值
     * @param value 数值
     * @param animate 是否播放滚动动画
     */
    void setValue(const QString &value, bool animate = true);

    /**
     * @brief 设置单位
     * @param unit 单位字符串
     */
    void setUnit(const QString &unit);

    /**
     * @brief 设置固定小数位数
     * @param decimals 小数位数（0-10）
     */
    void setDecimals(int decimals);

    /**
     * @brief 设置数值格式化回调
     * @param formatter 格式化函数
     */
    using ValueFormatter = std::function<QString(const QString&)>;
    void setValueFormatter(ValueFormatter formatter);

    QLabel *displayLabel() const { return m_display; }

signals:
    void valueChanged(const QString &value);

private slots:
    void onTimerTick();

private:
    QHBoxLayout *m_layout;
    QLabel *m_display;
    QLabel *m_unit;
    QTimer *m_animationTimer;
    QString m_currentValue;
    int m_decimalPlaces;
    ValueFormatter m_formatter;
};

#endif // LCD_DISPLAY_H
