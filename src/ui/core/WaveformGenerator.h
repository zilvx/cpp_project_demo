#ifndef WAVEFORM_GENERATOR_H
#define WAVEFORM_GENERATOR_H

#include <QObject>
#include <QString>
#include "WaveformData.h"

/**
 * @brief 后台波形生成工作对象（多线程练习）
 *
 * 通过 moveToThread 挂到工作线程，槽 generate() 在后台线程完成波形计算，
 * 完成后以信号 waveReady() 把结果按值传回主线程（QueuedConnection）。
 *
 * 设计要点:
 *   - 不触碰任何 QWidget，只做纯数值计算，可安全跨线程
 *   - requestId：用户快速切换时后发请求可能先完成，主线程按 id 丢弃过期结果
 *   - simulateMs：模拟耗时，用于观察同步 vs 后台线程的 UI 卡顿差异
 *   - WaveParams 为自定义类型，构造时需 qRegisterMetaType 方可跨线程投递
 */
class WaveformGenerator : public QObject {
    Q_OBJECT

public:
    explicit WaveformGenerator(QObject *parent = nullptr);

    /// 供主线程同步模式复用的生成逻辑（不依赖 Qt 线程）
    static bool buildWaveform(const WaveParams &params, WaveformData &out);

public slots:
    /// 在后台线程执行：模拟耗时 + 生成波形 → waveReady
    void generate(int requestId, const WaveParams &params, int simulateMs);

signals:
    void waveReady(int requestId, const WaveformData &data);
    void waveFailed(int requestId, const QString &message);
};

#endif // WAVEFORM_GENERATOR_H