#include <gtest/gtest.h>
#include "WaveformChart.h"

TEST(NiceStepTest, NormalRange) {
    EXPECT_DOUBLE_EQ(WaveformChart::niceStep(9.5, 10), 1.0);
    EXPECT_DOUBLE_EQ(WaveformChart::niceStep(0.95, 10), 0.1);
    EXPECT_DOUBLE_EQ(WaveformChart::niceStep(950.0, 10), 100.0);
    EXPECT_DOUBLE_EQ(WaveformChart::niceStep(0.47, 8), 0.05);
    EXPECT_DOUBLE_EQ(WaveformChart::niceStep(47.0, 8), 5.0);
}

TEST(NiceStepTest, ZeroSpan) {
    // 零跨度的 niceStep 返回 0.0（无有效刻度间距）
    EXPECT_DOUBLE_EQ(WaveformChart::niceStep(0.0, 10), 0.0);
}

TEST(NiceStepTest, ManyDivs) {
    double step = WaveformChart::niceStep(100.0, 50);
    EXPECT_GT(step, 0.0);
    double steps = 100.0 / step;
    EXPECT_GE(steps, 20.0);
    EXPECT_LE(steps, 80.0);
}

TEST(NiceStepTest, SingleDiv) {
    double step = WaveformChart::niceStep(100.0, 1);
    EXPECT_GT(step, 0.0);
}
