#ifndef WAVEFORM_DATA_H
#define WAVEFORM_DATA_H

#include <QVector>
#include <QString>
#include <QPointF>
#include <cmath>

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

    /// 生成正弦波
    void generateSine(double frequency = 1.0,
                      double amplitude = 1.0,
                      double sampleRate = 100.0,
                      int    numCycles = 2);

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
                          int    numCycles = 2);

    const QVector<Point> &points() const { return m_points; }
    int  count() const { return m_points.size(); }
    bool isEmpty() const { return m_points.isEmpty(); }

    Range timeRange()  const { return m_timeRange; }
    Range ampRange()   const { return m_ampRange; }

private:
    void updateRanges();

    QVector<Point> m_points;
    Range m_timeRange;
    Range m_ampRange;
};

#endif // WAVEFORM_DATA_H
