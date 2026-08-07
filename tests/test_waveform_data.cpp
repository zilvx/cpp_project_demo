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
