#include "WaveformData.h"

#include <QFile>
#include <QTextStream>
#include <QDebug>

bool WaveformData::loadCSV(const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning("WaveformData: cannot open %s", qPrintable(filePath));
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
        qWarning("WaveformData: no valid data in %s", qPrintable(filePath));
        return false;
    }

    updateRanges();
    return true;
}

void WaveformData::generateSine(double frequency, double amplitude,
                                 double sampleRate, int numCycles) {
    m_points.clear();
    const double duration = numCycles / frequency;
    const double dt = 1.0 / sampleRate;
    const int    total = static_cast<int>(duration / dt);

    for (int i = 0; i <= total; ++i) {
        const double t = i * dt;
        const double y = amplitude * std::sin(2.0 * M_PI * frequency * t);
        m_points.append(QPointF(t, y));
    }
    updateRanges();
}

void WaveformData::generateSquare(double frequency, double amplitude,
                                   double sampleRate, int numCycles,
                                   double riseTimeRatio) {
    m_points.clear();
    const double period    = 1.0 / frequency;
    const double halfT     = period / 2.0;
    const double riseTime  = period * riseTimeRatio;
    const double dt        = 1.0 / sampleRate;
    const double duration  = numCycles * period;
    const int    total     = static_cast<int>(duration / dt);

    for (int i = 0; i <= total; ++i) {
        const double t = i * dt;
        const double phase = std::fmod(t, period);
        double y = 0.0;

        if (phase < riseTime) {
            // 上升沿
            y = -amplitude + 2.0 * amplitude * (phase / riseTime);
        } else if (phase < halfT) {
            y = amplitude;
        } else if (phase < halfT + riseTime) {
            // 下降沿
            y = amplitude - 2.0 * amplitude * ((phase - halfT) / riseTime);
        } else {
            y = -amplitude;
        }
        m_points.append(QPointF(t, y));
    }
    updateRanges();
}

void WaveformData::generateSawtooth(double frequency, double amplitude,
                                     double sampleRate, int numCycles) {
    m_points.clear();
    const double period   = 1.0 / frequency;
    const double dt       = 1.0 / sampleRate;
    const double duration = numCycles * period;
    const int    total    = static_cast<int>(duration / dt);

    for (int i = 0; i <= total; ++i) {
        const double t = i * dt;
        const double phase = std::fmod(t, period);
        const double y = -amplitude + 2.0 * amplitude * (phase / period);
        m_points.append(QPointF(t, y));
    }
    updateRanges();
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
