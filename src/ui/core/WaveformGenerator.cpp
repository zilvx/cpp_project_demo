#include "WaveformGenerator.h"

#include <QThread>

#include <spdlog/spdlog.h>

WaveformGenerator::WaveformGenerator(QObject *parent) : QObject(parent) {
    // 跨线程 QueuedConnection 传递自定义类型前必须注册元类型
    qRegisterMetaType<WaveParams>("WaveParams");
    qRegisterMetaType<WaveformData>("WaveformData");
}

void WaveformGenerator::generate(int requestId, const WaveParams &params,
                                 int simulateMs) {
    // 模拟耗时：制造可感知的计算时间，便于对比 UI 卡顿
    if (simulateMs > 0) QThread::msleep(static_cast<unsigned long>(simulateMs));

    WaveformData data;
    if (!buildWaveform(params, data)) {
        spdlog::warn("WaveformGenerator: 波形生成失败, requestId={}", requestId);
        emit waveFailed(requestId, QStringLiteral("波形生成失败"));
        return;
    }
    emit waveReady(requestId, data);
}

bool WaveformGenerator::buildWaveform(const WaveParams &params, WaveformData &out) {
    return out.generate(params);
}