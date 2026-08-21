#include <gtest/gtest.h>
#include <QTemporaryDir>
#include <QFile>
#include <QFileInfo>

#include "WaveformData.h"

// ====== WaveformData CSV 保存 / 往返测试 ======

TEST(ArbCsvTest, SaveCsvRoundTrip) {
    WaveformData src;
    src.generateSine(1.0, 2.5, 500.0, 3);

    QTemporaryDir tmp;
    ASSERT_TRUE(tmp.isValid());
    const QString path = tmp.filePath(QStringLiteral("wave.csv"));

    ASSERT_TRUE(src.saveCSV(path));

    WaveformData dst;
    ASSERT_TRUE(dst.loadCSV(path));

    EXPECT_EQ(dst.count(), src.count());
    const auto &a = src.points();
    const auto &b = dst.points();
    ASSERT_EQ(a.size(), b.size());
    for (int i = 0; i < a.size(); ++i) {
        EXPECT_NEAR(a[i].x(), b[i].x(), 1e-9);
        EXPECT_NEAR(a[i].y(), b[i].y(), 1e-9);
    }
}

TEST(ArbCsvTest, SaveCsvEmptyDataFails) {
    WaveformData empty;
    QTemporaryDir tmp;
    ASSERT_TRUE(tmp.isValid());
    const QString path = tmp.filePath(QStringLiteral("empty.csv"));
    EXPECT_FALSE(empty.saveCSV(path));
    EXPECT_FALSE(QFileInfo::exists(path));
}

TEST(ArbCsvTest, SaveCsvUnwritablePathFails) {
    WaveformData data;
    data.generateSine(1.0, 1.0, 100.0, 2);
    // 指向不存在目录下的文件路径，open 必然失败
    EXPECT_FALSE(data.saveCSV(QStringLiteral("/nonexistent_dir_xyz/out.csv")));
}

TEST(ArbCsvTest, SaveCsvProducesHeaderAndTwoColumns) {
    WaveformData data;
    data.generateTriangle(1.0, 1.0, 100.0, 2);

    QTemporaryDir tmp;
    ASSERT_TRUE(tmp.isValid());
    const QString path = tmp.filePath(QStringLiteral("hdr.csv"));
    ASSERT_TRUE(data.saveCSV(path));

    // 读回后 loadCSV 可解析且行数 = 注释 2 行 + 点数行
    QFile f(path);
    ASSERT_TRUE(f.open(QIODevice::ReadOnly | QIODevice::Text));
    int dataLines = 0;
    while (!f.atEnd()) {
        const QString line = f.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#')) continue;
        EXPECT_EQ(line.split(',').size(), 2);
        ++dataLines;
    }
    EXPECT_EQ(dataLines, data.count());
}
