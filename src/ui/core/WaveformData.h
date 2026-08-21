#ifndef WAVEFORM_DATA_H
#define WAVEFORM_DATA_H

#include <QVector>
#include <QString>
#include <QPointF>
#include <QMetaType>
#include <cmath>

/**
 * @brief 波形类型（函数发生器支持的波形）
 */
enum class WaveType { Sine, Square, Triangle, Sawtooth, Pulse, Sweep, Csv };

/**
 * @brief 波形生成参数 — 统一描述函数发生器的各项设置
 *
 * 由 UI 控件收集后经跨线程 QueuedConnection 传给后台工作对象，
 * 因此声明了 Q_DECLARE_METATYPE 并在 WaveformGenerator 构造里注册。
 */
struct WaveParams {
    WaveType type = WaveType::Sine;
    double frequency = 1.0;      // Hz；扫频时为起始频率
    double freqEnd   = 2.0;      // Hz；仅扫频使用
    double amplitude = 2.5;
    double offset    = 0.0;      // DC 偏置
    double phaseDeg  = 0.0;      // 相位（度）
    double dutyCycle = 0.5;      // 方波/脉冲占空比 [0,1]
    double sampleRate = 500.0;   // 采样率（点/秒）
    int    numCycles  = 3;       // 周期数；周期波使用
    double sweepDurationSec = 2.0; // 扫频时长（秒）
    QString csvPath;             // 仅 CSV 使用
};

Q_DECLARE_METATYPE(WaveParams)

/**
 * @brief 波形数据模型 — 从 CSV 加载或数学生成坐标点
 *
 * CSV 格式（两列，逗号分隔，无表头）:
 *   时间, 幅值
 *   0.0, 0.0
 *   0.1, 0.587
 */
class WaveformData {
public:
    struct Range {
        double min = 0.0, max = 1.0;
        double span() const { return max - min; }
    };

    using Point = QPointF;

    WaveformData() = default;

    /// 从 CSV 文件加载数据
    bool loadCSV(const QString &filePath);

    /// 将当前波形数据保存为 CSV 文件（与 loadCSV 格式一致，可往返）
    bool saveCSV(const QString &filePath) const;

    /// 生成正弦波
    void generateSine(double frequency = 1.0,
                      double amplitude = 1.0,
                      double sampleRate = 100.0,
                      int    numCycles = 2,
                      double offset = 0.0,
                      double phaseDeg = 0.0);

    /// 生成方波（含上升/下降沿过渡，避免无限斜率）
    void generateSquare(double frequency = 1.0,
                        double amplitude = 1.0,
                        double sampleRate = 100.0,
                        int    numCycles = 2,
                        double riseTimeRatio = 0.02);

    /// 生成锯齿波
    void generateSawtooth(double frequency = 1.0,
                          double amplitude = 1.0,
                          double sampleRate = 100.0,
                          int    numCycles = 2,
                          double offset = 0.0,
                          double phaseDeg = 0.0);

    /// 生成三角波（对称三角，-A~+A）
    void generateTriangle(double frequency = 1.0,
                          double amplitude = 1.0,
                          double sampleRate = 100.0,
                          int    numCycles = 2,
                          double offset = 0.0,
                          double phaseDeg = 0.0);

    /// 生成脉冲波（可调占空比）
    void generatePulse(double frequency = 1.0,
                       double amplitude = 1.0,
                       double sampleRate = 100.0,
                       int    numCycles = 2,
                       double dutyCycle = 0.5,
                       double offset = 0.0,
                       double phaseDeg = 0.0);

    /// 生成线性扫频波（chirp）：频率从 freqStart 线性变化到 freqEnd
    void generateSweep(double freqStart = 1.0,
                       double freqEnd = 2.0,
                       double amplitude = 1.0,
                       double sampleRate = 100.0,
                       double durationSec = 2.0,
                       double offset = 0.0,
                       double phaseDeg = 0.0);

    /// 按统一参数生成（函数发生器主入口），CSV 类型由调用方走 loadCSV
    bool generate(const WaveParams &params);

    const QVector<Point> &points() const { return m_points; }
    int  count() const { return m_points.size(); }
    bool isEmpty() const { return m_points.isEmpty(); }

    Range timeRange()  const { return m_timeRange; }
    Range ampRange()   const { return m_ampRange; }

private:
    void updateRanges();
    /// 方波/脉冲共用实现
    void generatePulseCore(double frequency, double amplitude,
                           double sampleRate, int numCycles,
                           double dutyCycle, double riseTimeRatio,
                           double offset, double phaseDeg);

    QVector<Point> m_points;
    Range m_timeRange;
    Range m_ampRange;
};

Q_DECLARE_METATYPE(WaveformData)

#endif // WAVEFORM_DATA_H