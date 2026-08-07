#ifndef WAVEFORM_CHART_H
#define WAVEFORM_CHART_H

#include <QWidget>
#include <QFont>
#include "../core/WaveformData.h"

/**
 * @brief 波形图表组件 — 示波器风格，精细刻度 + 平滑曲线 + 滑动标记
 *
 * 支持:
 *   - 精细的网格与刻度线（主刻度 + 次刻度）
 *   - Catmull-Rom 平滑曲线连接数据点
 *   - 数学生成的正弦波 / 方波 / 锯齿波
 *   - CSV 文件导入的自定义波形
 *   - 可拖动三角形标记 + 实时坐标显示
 */
class WaveformChart : public QWidget {
    Q_OBJECT

public:
    explicit WaveformChart(QWidget *parent = nullptr);

    void setData(const WaveformData &data);
    void setTitle(const QString &title);
    void setXLabel(const QString &label);
    void setYLabel(const QString &label);

    // 工具方法（公开用于单元测试）
    QPointF dataToPixel(const QPointF &dp, const QRect &r) const;
    QPointF pixelToData(const QPointF &px, const QRect &r) const;
    double  interpolateY(double dataX) const;
    static double niceStep(double span, int targetDivs);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    // 绘图子步骤
    void drawBackground(QPainter &p);
    void drawGrid(QPainter &p, const QRect &r);
    void drawCurve(QPainter &p, const QRect &r);
    void drawAxisLabels(QPainter &p, const QRect &r);
    void drawMarker(QPainter &p, const QRect &r);

    QRect   chartRect() const;
    static QPainterPath buildSmoothPath(const QVector<QPointF> &points);

    // ---- 数据 ----
    WaveformData m_data;
    QString m_title;
    QString m_xLabel;
    QString m_yLabel;

    // ---- 滑动标记 ----
    bool     m_markerVisible = false;
    double   m_markerX       = 0.0;
    double   m_markerY       = 0.0;
    bool     m_dragging      = false;

    // ---- 缓存字体 ----
    QFont m_axisFont;
    QFont m_titleFont;
    QFont m_labelFont;

    // ---- 布局常量 ----
    static constexpr int kMarginLeft   = 70;
    static constexpr int kMarginRight  = 100;
    static constexpr int kMarginTop    = 30;
    static constexpr int kMarginBottom = 50;
    static constexpr int kGridMajor    = 5;

    // ---- 主题颜色 ----
    static constexpr int kBgR = 0x1a, kBgG = 0x1d, kBgB = 0x2e;         // 背景
    static constexpr int kWaveR = 0, kWaveG = 210, kWaveB = 210;        // 波形线 (#00D2D2)
    static constexpr int kWaveAlpha = 180;

    static constexpr int kGridMinorAlpha = 25;
    static constexpr int kGridMajorAlpha = 50;
    static constexpr int kBorderAlpha    = 70;
    static constexpr int kAxisTextAlpha  = 160;

    static constexpr int kMarkerR = 255, kMarkerG = 180, kMarkerB = 30;  // 三角形 (#FFB41E)
    static constexpr int kMarkerGlowAlpha = 60;
    static constexpr int kMarkerBodyAlpha = 240;
    static constexpr int kLabelBgR = 30, kLabelBgG = 33, kLabelBgB = 50; // 标签背景
    static constexpr int kLabelBgAlpha   = 230;
    static constexpr int kLabelBorderR = 255, kLabelBorderG = 200, kLabelBorderB = 50;
    static constexpr int kLabelBorderAlpha = 180;
    static constexpr int kLabelTextR = 255, kLabelTextG = 220, kLabelTextB = 100;
};

#endif // WAVEFORM_CHART_H
