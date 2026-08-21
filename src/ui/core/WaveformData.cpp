#include "WaveformData.h"

#include <QFile>
#include <QTextStream>
#include <spdlog/spdlog.h>

namespace {
// 以指定相位（度）对时间轴做平移：t' = t - phase/(2πf)，等价于信号前移 phase 角
double phaseShift(double t, double frequency, double phaseDeg) {
    if (phaseDeg == 0.0 || frequency <= 0.0) return t;
    return t - phaseDeg / 360.0 / frequency;
}
} // namespace

bool WaveformData::loadCSV(const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        spdlog::warn("WaveformData: cannot open {}", filePath.toStdString());
        return false;
    }

    m_points.clear();
    QTextStream in(&file);
    while (!in.atEnd()) {
        const QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#')) continue;

        const QStringList cols = line.split(',');
        if (cols.size() < 2) continue;

        bool okX = false, okY = false;
        const double x = cols[0].trimmed().toDouble(&okX);
        const double y = cols[1].trimmed().toDouble(&okY);
        if (okX && okY) {
            m_points.append(QPointF(x, y));
        }
    }
    file.close();

    if (m_points.isEmpty()) {
        spdlog::warn("WaveformData: no valid data in {}", filePath.toStdString());
        return false;
    }

    updateRanges();
    return true;
}

bool WaveformData::saveCSV(const QString &filePath) const {
    if (m_points.isEmpty()) {
        spdlog::warn("WaveformData: 无数据可保存");
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        spdlog::warn("WaveformData: cannot open {} for writing", filePath.toStdString());
        return false;
    }

    QTextStream out(&file);
    // 与 loadCSV 一致：注释头 + 两列逗号分隔（时间,幅值），保证 save→load 往返可逆
    out << "# 自定义任意波形数据\n";
    out << "# 时间, 幅值\n";
    for (const auto &p : m_points)
        out << QString::number(p.x(), 'g', 12) << ','
            << QString::number(p.y(), 'g', 12) << '\n';
    file.close();
    return true;
}

void WaveformData::generateSine(double frequency, double amplitude,
                                 double sampleRate, int numCycles,
                                 double offset, double phaseDeg) {
    m_points.clear();
    const double duration = numCycles / frequency;
    const double dt = 1.0 / sampleRate;
    const int    total = static_cast<int>(duration / dt);

    for (int i = 0; i <= total; ++i) {
        const double t = i * dt;
        const double tp = phaseShift(t, frequency, phaseDeg);
        const double y = amplitude * std::sin(2.0 * M_PI * frequency * tp) + offset;
        m_points.append(QPointF(t, y));
    }
    updateRanges();
}

void WaveformData::generateSquare(double frequency, double amplitude,
                                   double sampleRate, int numCycles,
                                   double riseTimeRatio) {
    generatePulseCore(frequency, amplitude, sampleRate, numCycles,
                      0.5, riseTimeRatio, 0.0, 0.0);
}

void WaveformData::generateSawtooth(double frequency, double amplitude,
                                     double sampleRate, int numCycles,
                                     double offset, double phaseDeg) {
    m_points.clear();
    const double period   = 1.0 / frequency;
    const double dt       = 1.0 / sampleRate;
    const double duration = numCycles * period;
    const int    total    = static_cast<int>(duration / dt);

    for (int i = 0; i <= total; ++i) {
        const double t = i * dt;
        const double tp = phaseShift(t, frequency, phaseDeg);
        const double phase = std::fmod(tp, period);
        const double y = -amplitude + 2.0 * amplitude * (phase / period) + offset;
        m_points.append(QPointF(t, y));
    }
    updateRanges();
}

void WaveformData::generateTriangle(double frequency, double amplitude,
                                     double sampleRate, int numCycles,
                                     double offset, double phaseDeg) {
    m_points.clear();
    const double period   = 1.0 / frequency;
    const double dt       = 1.0 / sampleRate;
    const double duration = numCycles * period;
    const int    total    = static_cast<int>(duration / dt);

    for (int i = 0; i <= total; ++i) {
        const double t = i * dt;
        const double tp = phaseShift(t, frequency, phaseDeg);
        const double phase = std::fmod(tp, period);          // [0, period)
        const double frac = phase / period;                  // [0, 1)
        // 线性上升到 +A，再线性下降回 -A（对称三角）
        const double y = (frac < 0.5)
                             ? -amplitude + 4.0 * amplitude * frac
                             :  amplitude - 4.0 * amplitude * (frac - 0.5);
        m_points.append(QPointF(t, y + offset));
    }
    updateRanges();
}

void WaveformData::generatePulse(double frequency, double amplitude,
                                  double sampleRate, int numCycles,
                                  double dutyCycle, double offset, double phaseDeg) {
    generatePulseCore(frequency, amplitude, sampleRate, numCycles,
                      dutyCycle, 0.0, offset, phaseDeg);
}

void WaveformData::generatePulseCore(double frequency, double amplitude,
                                     double sampleRate, int numCycles,
                                     double dutyCycle, double riseTimeRatio,
                                     double offset, double phaseDeg) {
    m_points.clear();
    const double period   = 1.0 / frequency;
    const double dt       = 1.0 / sampleRate;
    const double duration = numCycles * period;
    const int    total    = static_cast<int>(duration / dt);

    // 占空比钳位到 (0,1)，避免除零 / 恒高
    double dc = dutyCycle;
    if (dc <= 0.0) dc = 0.0;
    if (dc >= 1.0) dc = 1.0;

    for (int i = 0; i <= total; ++i) {
        const double t = i * dt;
        const double tp = phaseShift(t, frequency, phaseDeg);
        const double phase = std::fmod(tp, period);
        const double frac = phase / period;   // [0,1)
        double y;
        if (riseTimeRatio > 0.0 && frac < riseTimeRatio) {
            y = -amplitude + 2.0 * amplitude * (frac / riseTimeRatio);
        } else if (frac < dc) {
            y = amplitude;
        } else {
            y = -amplitude;
        }
        m_points.append(QPointF(t, y + offset));
    }
    updateRanges();
}

void WaveformData::generateSweep(double freqStart, double freqEnd,
                                  double amplitude, double sampleRate,
                                  double durationSec, double offset, double phaseDeg) {
    m_points.clear();
    const double dt  = 1.0 / sampleRate;
    const int    total = static_cast<int>(durationSec / dt);

    // 线性扫频：瞬时频率 f(t) = f0 + (f1-f0)/T * t
    // 相位 φ(t) = 2π( f0*t + (f1-f0)/(2T) * t² ) + φ0
    const double k = (freqEnd - freqStart) / durationSec;
    const double phi0 = phaseDeg * M_PI / 180.0;

    for (int i = 0; i <= total; ++i) {
        const double t = i * dt;
        const double phase = 2.0 * M_PI * (freqStart * t + 0.5 * k * t * t) + phi0;
        const double y = amplitude * std::sin(phase) + offset;
        m_points.append(QPointF(t, y));
    }
    updateRanges();
}

bool WaveformData::generate(const WaveParams &params) {
    switch (params.type) {
    case WaveType::Sine:
        generateSine(params.frequency, params.amplitude, params.sampleRate,
                     params.numCycles, params.offset, params.phaseDeg);
        return true;
    case WaveType::Square:
        generateSquare(params.frequency, params.amplitude, params.sampleRate,
                       params.numCycles, 0.02);
        break; // 占空比/偏置/相位对标准方波不生效（方波用 generatePulseCore 且占空比固定 0.5）
    case WaveType::Triangle:
        generateTriangle(params.frequency, params.amplitude, params.sampleRate,
                         params.numCycles, params.offset, params.phaseDeg);
        return true;
    case WaveType::Sawtooth:
        generateSawtooth(params.frequency, params.amplitude, params.sampleRate,
                         params.numCycles, params.offset, params.phaseDeg);
        return true;
    case WaveType::Pulse:
        generatePulse(params.frequency, params.amplitude, params.sampleRate,
                      params.numCycles, params.dutyCycle, params.offset, params.phaseDeg);
        return true;
    case WaveType::Sweep:
        generateSweep(params.frequency, params.freqEnd, params.amplitude,
                      params.sampleRate, params.sweepDurationSec, params.offset,
                      params.phaseDeg);
        return true;
    case WaveType::Csv:
        return loadCSV(params.csvPath);
    default:
        return false;
    }
    return true;
}

void WaveformData::updateRanges() {
    if (m_points.isEmpty()) return;

    m_timeRange.min = m_points.first().x();
    m_timeRange.max = m_points.first().x();
    m_ampRange.min  = m_points.first().y();
    m_ampRange.max  = m_points.first().y();

    for (const auto &p : m_points) {
        if (p.x() < m_timeRange.min) m_timeRange.min = p.x();
        if (p.x() > m_timeRange.max) m_timeRange.max = p.x();
        if (p.y() < m_ampRange.min)  m_ampRange.min  = p.y();
        if (p.y() > m_ampRange.max)  m_ampRange.max  = p.y();
    }

    // 略扩展幅值范围，避免曲线贴边
    const double ampPad = m_ampRange.span() * 0.1 + 0.1;
    m_ampRange.min -= ampPad;
    m_ampRange.max += ampPad;
}