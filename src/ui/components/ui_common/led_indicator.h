#ifndef LED_INDICATOR_H
#define LED_INDICATOR_H

#include <QWidget>
#include <QLabel>

/**
 * @brief LED指示灯组件
 *
 * 用途：显示设备状态（电源、锁定、过载等）
 * 颜色：
 *   - 绿色：正常/锁定
 *   - 红色：错误/警告/过载
 *   - 黄色：待机/警告
 */
class LedIndicator : public QWidget {
    Q_OBJECT

public:
    enum class Status {
        Off,           // 熄灭
        On,            // 点亮
        Blinking       // 闪烁
    };

    explicit LedIndicator(QWidget *parent = nullptr);

    /**
     * @brief 设置LED状态
     * @param status 状态（绿色、红色、黄色、熄灭、闪烁）
     * @param color 具体颜色（可选，默认根据状态颜色）
     */
    void setStatus(Status status, QColor color = QColor());

    /**
     * @brief 设置LED颜色
     * @param color 颜色值
     */
    void setColor(QColor color);

    /**
     * @brief 启动/停止闪烁
     * @param interval 闪烁间隔（毫秒）
     */
    void startBlinking(int interval = 500);
    void stopBlinking();

signals:
    void statusChanged(Status newStatus);

private slots:
    void onTimerTick();

private:
    QLabel *m_led;
    QTimer *m_blinkTimer;
    QColor m_color;
    Status m_status;
    bool m_isBlinking;
};

#endif // LED_INDICATOR_H
