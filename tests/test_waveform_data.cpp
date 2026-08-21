#include <gtest/gtest.h>
#include <QApplication>
#include <QRect>

#include "WaveformData.h"
#include "WaveformChart.h"

// ====== WaveformData 波形生成测试 ======

TEST(WaveformDataTest, GenerateSine) {
    WaveformData data;
    data.generateSine(1.0, 2.5, 500.0, 3);
    EXPECT_GT(data.count(), 0);
    EXPECT_EQ(data.count(), static_cast<int>(500.0 * 3.0 / 1.0) + 1);
    EXPECT_FALSE(data.isEmpty());
}

TEST(WaveformDataTest, AmplitudeRange) {
    WaveformData data;
    data.generateSine(1.0, 2.5, 500.0, 3);
    auto ar = data.ampRange();
    EXPECT_GE(ar.max, 2.5);
    EXPECT_LE(ar.min, -2.5);
}

TEST(WaveformDataTest, GenerateSquare) {
    WaveformData data;
    data.generateSquare(1.0, 1.0, 100.0, 2);
    EXPECT_GT(data.count(), 0);
    EXPECT_EQ(data.count(), static_cast<int>(100.0 * 2.0 / 1.0) + 1);
}

TEST(WaveformDataTest, GenerateSawtooth) {
    WaveformData data;
    data.generateSawtooth(2.0, 1.5, 200.0, 1);
    EXPECT_GT(data.count(), 0);
    EXPECT_EQ(data.count(), static_cast<int>(200.0 * 1.0 / 2.0) + 1);
}

TEST(WaveformDataTest, TimeRange) {
    WaveformData data;
    data.generateSine(2.0, 1.0, 100.0, 4);
    auto tr = data.timeRange();
    EXPECT_NEAR(tr.min, 0.0, 1e-9);
    EXPECT_NEAR(tr.max, 2.0, 1e-9);  // 4 cycles / 2Hz = 2s
}

TEST(WaveformDataTest, EmptyByDefault) {
    WaveformData data;
    EXPECT_TRUE(data.isEmpty());
    EXPECT_EQ(data.count(), 0);
}

// ====== 扩展波形生成测试（函数发生器） ======

TEST(WaveformDataTest, GenerateTriangle) {
    WaveformData data;
    data.generateTriangle(2.0, 1.5, 200.0, 1);
    EXPECT_GT(data.count(), 0);
    auto ar = data.ampRange();
    EXPECT_GE(ar.max, 1.5);
    EXPECT_LE(ar.min, -1.5);
}

TEST(WaveformDataTest, GeneratePulseDutyCycle) {
    WaveformData data;
    data.generatePulse(1.0, 1.0, 1000.0, 1, 0.25);
    EXPECT_GT(data.count(), 0);
    // 统计高电平（y>0）与低电平（y<0）的占比，应约等于占空比 25%
    int high = 0, total = 0;
    for (const auto &p : data.points()) {
        ++total;
        if (p.y() > 0.0) ++high;
    }
    EXPECT_GT(total, 0);
    const double ratio = static_cast<double>(high) / total;
    EXPECT_NEAR(ratio, 0.25, 0.05);
}

TEST(WaveformDataTest, GenerateSweepFreqIncreases) {
    // 扫频瞬时频率随时间单调上升：相邻极值间距应越来越短。
    // 简化验证：首、中、末区间的过零点个数应递增。
    WaveformData data;
    data.generateSweep(1.0, 10.0, 1.0, 1000.0, 4.0);
    EXPECT_GT(data.count(), 0);

    // 将时间均分为三段，统计每段过零次数（采样点 y 符号变化）
    const auto &pts = data.points();
    auto zeroCrossings = [&](double t0, double t1) {
        int n = 0;
        bool prevPositive = pts.first().y() > 0.0;
        for (const auto &p : pts) {
            if (p.x() < t0 || p.x() > t1) continue;
            const bool cur = p.y() > 0.0;
            if (cur != prevPositive) ++n;
            prevPositive = cur;
        }
        return n;
    };
    const int first = zeroCrossings(0.0, 1.0);
    const int last  = zeroCrossings(3.0, 4.0);
    EXPECT_LT(first, last);   // 高频段过零更多
}

TEST(WaveformDataTest, OffsetShiftsRange) {
    WaveformData data;
    data.generateSine(1.0, 2.0, 200.0, 2, 0.5, 0.0);
    auto ar = data.ampRange();
    // 偏置使整个范围上移 0.5：中心从 0 移到 +0.5（padding 对称，不改变中心）
    EXPECT_NEAR((ar.max + ar.min) / 2.0, 0.5, 1e-6);
    // 幅值跨度含 padding：rawSpan=2A，pad=0.1*rawSpan+0.1 各加两侧
    const double rawSpan = 4.0;
    const double pad = 0.1 * rawSpan + 0.1;
    EXPECT_NEAR(ar.span(), rawSpan + 2.0 * pad, 1e-6);
}

TEST(WaveformDataTest, GenerateViaParams) {
    WaveParams p;
    p.type       = WaveType::Pulse;
    p.frequency  = 1.0;
    p.amplitude  = 2.0;
    p.sampleRate = 500.0;
    p.numCycles  = 2;
    p.dutyCycle  = 0.5;
    p.offset     = 1.0;

    WaveformData data;
    ASSERT_TRUE(data.generate(p));
    EXPECT_GT(data.count(), 0);
    auto ar = data.ampRange();
    EXPECT_NEAR((ar.max + ar.min) / 2.0, 1.0, 1e-6);  // 中心 = offset
    const double rawSpan = 4.0;
    const double pad = 0.1 * rawSpan + 0.1;
    EXPECT_NEAR(ar.span(), rawSpan + 2.0 * pad, 1e-6);
}

TEST(WaveformDataTest, GenerateViaParamsUnknownType) {
    WaveParams p;
    p.type = static_cast<WaveType>(999);
    WaveformData data;
    EXPECT_FALSE(data.generate(p));
}

// ====== WaveformChart 坐标转换测试 ======

class WaveformChartTest : public ::testing::Test {
protected:
    void SetUp() override {
        chart = new WaveformChart();
        chart->resize(600, 400);

        WaveformData d;
        d.generateSine(1.0, 2.5, 500.0, 3);
        chart->setData(d);
    }
    void TearDown() override { delete chart; }
    WaveformChart *chart;
};

TEST_F(WaveformChartTest, NiceStepIsStatic) {
    EXPECT_DOUBLE_EQ(WaveformChart::niceStep(9.5, 10), 1.0);
    EXPECT_DOUBLE_EQ(WaveformChart::niceStep(0.95, 10), 0.1);
}

TEST_F(WaveformChartTest, DataToPixelNoCrash) {
    QPointF dp(1.0, 1.0);
    QRect r(50, 30, 500, 340);
    QPointF px = chart->dataToPixel(dp, r);
    EXPECT_GE(px.x(), r.left());
    EXPECT_LE(px.x(), r.right());
}

TEST_F(WaveformChartTest, PixelToDataRoundTrip) {
    QRect r(50, 30, 500, 340);
    // 选择数据范围中间的值以提高精度
    QPointF original(1.5, 0.0);
    QPointF pixel = chart->dataToPixel(original, r);
    QPointF back  = chart->pixelToData(pixel, r);
    EXPECT_NEAR(original.x(), back.x(), 1e-3);
    EXPECT_NEAR(original.y(), back.y(), 0.1);  // Y 轴精度受数据范围影响
}

TEST_F(WaveformChartTest, InterpolateYAtKnownPoint) {
    // 正弦波在 t=0 处 y=0，t=0.25 处 y=2.5
    double y0 = chart->interpolateY(0.0);
    EXPECT_NEAR(y0, 0.0, 1e-2);
}
