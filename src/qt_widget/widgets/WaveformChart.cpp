#include "WaveformChart.h"

#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QtMath>

// ============================================================
//  构造
// ============================================================

WaveformChart::WaveformChart(QWidget *parent)
    : QWidget(parent) {
    setMinimumSize(400, 250);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMouseTracking(true);

    // 缓存字体，避免每帧重复创建
    m_axisFont  = QFont(QStringLiteral("Monospace"), 9);
    m_titleFont = QFont(QStringLiteral("Monospace"), 12, QFont::Bold);
    m_labelFont = QFont(QStringLiteral("Monospace"), 10, QFont::Bold);
}

void WaveformChart::setData(const WaveformData &data) {
    m_data = data;
    m_markerVisible = false;
    update();
}

void WaveformChart::setTitle(const QString &t) { m_title = t; update(); }
void WaveformChart::setXLabel(const QString &l) { m_xLabel = l; update(); }
void WaveformChart::setYLabel(const QString &l) { m_yLabel = l; update(); }

// ============================================================
//  布局工具
// ============================================================

QRect WaveformChart::chartRect() const {
    return QRect(kMarginLeft, kMarginTop,
                 width()  - kMarginLeft - kMarginRight,
                 height() - kMarginTop  - kMarginBottom);
}

double WaveformChart::niceStep(double span, int targetDivs) {
    const double rough = span / targetDivs;
    const double exp   = std::pow(10.0, std::floor(std::log10(rough)));
    const double mant  = rough / exp;
    if      (mant < 1.5) return 1.0 * exp;
    else if (mant < 3.0) return 2.0 * exp;
    else if (mant < 7.0) return 5.0 * exp;
    else                 return 10.0 * exp;
}

// ============================================================
//  曲线插值
// ============================================================

double WaveformChart::interpolateY(double dataX) const {
    const auto &pts = m_data.points();
    if (pts.isEmpty()) return 0.0;
    if (pts.size() == 1) return pts[0].y();

    int lo = 0, hi = pts.size() - 1;
    if (dataX <= pts[lo].x()) return pts[lo].y();
    if (dataX >= pts[hi].x()) return pts[hi].y();

    while (hi - lo > 1) {
        const int mid = (lo + hi) / 2;
        if (pts[mid].x() <= dataX) lo = mid;
        else                       hi = mid;
    }
    const double t = (dataX - pts[lo].x()) / (pts[hi].x() - pts[lo].x());
    return pts[lo].y() + t * (pts[hi].y() - pts[lo].y());
}

// ============================================================
//  绘制入口
// ============================================================

void WaveformChart::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const QRect cr = chartRect();

    drawBackground(p);
    drawGrid(p, cr);
    drawCurve(p, cr);
    drawMarker(p, cr);
    drawAxisLabels(p, cr);
}

// ============================================================
//  背景
// ============================================================

void WaveformChart::drawBackground(QPainter &p) {
    p.fillRect(rect(), QColor(kBgR, kBgG, kBgB));
}

// ============================================================
//  精细网格
// ============================================================

void WaveformChart::drawGrid(QPainter &p, const QRect &r) {
    if (m_data.isEmpty()) return;

    const auto tRange = m_data.timeRange();
    const auto aRange = m_data.ampRange();

    const double xMajorStep = niceStep(tRange.span(), 10);
    const double yMajorStep = niceStep(aRange.span(), 8);
    const double xMinorStep = xMajorStep / kGridMajor;
    const double yMinorStep = yMajorStep / kGridMajor;

    const double xStart = std::floor(tRange.min / xMinorStep) * xMinorStep;
    const double yStart = std::floor(aRange.min / yMinorStep) * yMinorStep;

    const QPen minorPen(QColor(255, 255, 255, kGridMinorAlpha), 1);
    const QPen majorPen(QColor(255, 255, 255, kGridMajorAlpha), 1);

    // 垂直网格线
    for (double x = xStart; x <= tRange.max + xMinorStep * 0.5; x += xMinorStep) {
        const int px = static_cast<int>(r.left() +
            (x - tRange.min) / tRange.span() * r.width());
        if (px < r.left() || px > r.right()) continue;

        const double mod = std::fmod(std::abs(x - xStart), xMajorStep);
        const bool isMajor = (mod < xMinorStep * 0.01 || mod > xMajorStep - xMinorStep * 0.01);
        p.setPen(isMajor ? majorPen : minorPen);
        p.drawLine(px, r.top(), px, r.bottom());
    }

    // 水平网格线
    for (double y = yStart; y <= aRange.max + yMinorStep * 0.5; y += yMinorStep) {
        const int py = static_cast<int>(r.bottom() -
            (y - aRange.min) / aRange.span() * r.height());
        if (py < r.top() || py > r.bottom()) continue;

        const double mod = std::fmod(std::abs(y - yStart), yMajorStep);
        const bool isMajor = (mod < yMinorStep * 0.01 || mod > yMajorStep - yMinorStep * 0.01);
        p.setPen(isMajor ? majorPen : minorPen);
        p.drawLine(r.left(), py, r.right(), py);
    }

    // 边框
    p.setPen(QPen(QColor(255, 255, 255, kBorderAlpha), 1));
    p.drawRect(r);
}

// ============================================================
//  平滑曲线 (Catmull-Rom → Cubic Bezier)
// ============================================================

QPainterPath WaveformChart::buildSmoothPath(const QVector<QPointF> &pts) {
    QPainterPath path;
    const int n = pts.size();
    if (n < 2) {
        if (n == 1) { path.moveTo(pts[0]); path.addEllipse(pts[0], 1, 1); }
        return path;
    }
    if (n == 2) { path.moveTo(pts[0]); path.lineTo(pts[1]); return path; }

    path.moveTo(pts[0]);

    for (int i = 0; i < n - 1; ++i) {
        const QPointF p0 = pts[qMax(0, i - 1)];
        const QPointF p1 = pts[i];
        const QPointF p2 = pts[i + 1];
        const QPointF p3 = pts[qMin(n - 1, i + 2)];

        constexpr double kTension = 0.5;
        const QPointF cp1(
            p1.x() + (p2.x() - p0.x()) * kTension / 3.0,
            p1.y() + (p2.y() - p0.y()) * kTension / 3.0);
        const QPointF cp2(
            p2.x() - (p3.x() - p1.x()) * kTension / 3.0,
            p2.y() - (p3.y() - p1.y()) * kTension / 3.0);

        path.cubicTo(cp1, cp2, p2);
    }
    return path;
}

void WaveformChart::drawCurve(QPainter &p, const QRect &r) {
    if (m_data.count() < 1) return;

    QVector<QPointF> pxPts;
    pxPts.reserve(m_data.count());
    for (const auto &dp : m_data.points())
        pxPts.append(dataToPixel(dp, r));

    p.setPen(QPen(QColor(kWaveR, kWaveG, kWaveB, kWaveAlpha),
                  2.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(Qt::NoBrush);
    p.drawPath(buildSmoothPath(pxPts));
}

// ============================================================
//  坐标变换
// ============================================================

QPointF WaveformChart::dataToPixel(const QPointF &dp, const QRect &r) const {
    const auto tR = m_data.timeRange();
    const auto aR = m_data.ampRange();
    return QPointF(
        r.left()   + (dp.x() - tR.min) / tR.span() * r.width(),
        r.bottom() - (dp.y() - aR.min) / aR.span() * r.height()
    );
}

QPointF WaveformChart::pixelToData(const QPointF &px, const QRect &r) const {
    const auto tR = m_data.timeRange();
    const auto aR = m_data.ampRange();
    return QPointF(
        (px.x() - r.left()) / r.width() * tR.span() + tR.min,
        aR.max - (px.y() - r.top()) / r.height() * aR.span()
    );
}

// ============================================================
//  坐标轴标签
// ============================================================

void WaveformChart::drawAxisLabels(QPainter &p, const QRect &r) {
    if (m_data.isEmpty()) return;

    const auto tRange = m_data.timeRange();
    const auto aRange = m_data.ampRange();

    p.setPen(QColor(255, 255, 255, kAxisTextAlpha));
    p.setFont(m_axisFont);

    // ---- X 轴刻度 ----
    const double xStep  = niceStep(tRange.span(), 10);
    const double xStart = std::ceil(tRange.min / xStep) * xStep;

    for (double x = xStart; x <= tRange.max + xStep * 0.5; x += xStep) {
        const int px = static_cast<int>(
            r.left() + (x - tRange.min) / tRange.span() * r.width());
        if (px < r.left() || px > r.right()) continue;
        p.drawLine(px, r.bottom(), px, r.bottom() + 5);
        p.drawText(QRect(px - 30, r.bottom() + 5, 60, 15),
                   Qt::AlignHCenter | Qt::AlignTop,
                   QString::number(x, 'f', 2));
    }

    // ---- Y 轴刻度 ----
    const double yStep  = niceStep(aRange.span(), 8);
    const double yStart = std::ceil(aRange.min / yStep) * yStep;

    for (double y = yStart; y <= aRange.max + yStep * 0.5; y += yStep) {
        const int py = static_cast<int>(
            r.bottom() - (y - aRange.min) / aRange.span() * r.height());
        if (py < r.top() || py > r.bottom()) continue;
        p.drawLine(r.left() - 5, py, r.left(), py);
        p.drawText(QRect(r.left() - 65, py - 8, 60, 16),
                   Qt::AlignRight | Qt::AlignVCenter,
                   QString::number(y, 'f', 2));
    }

    // ---- 标题 ----
    if (!m_title.isEmpty()) {
        p.setFont(m_titleFont);
        p.setPen(QColor(255, 255, 255, 200));
        p.drawText(QRect(r.left(), 0, r.width(), kMarginTop),
                   Qt::AlignCenter, m_title);
    }

    // ---- 轴标签 ----
    if (!m_xLabel.isEmpty()) {
        p.setFont(m_axisFont);
        p.setPen(QColor(255, 255, 255, kAxisTextAlpha));
        p.drawText(QRect(r.left(), r.bottom() + 35, r.width(), 15),
                   Qt::AlignCenter, m_xLabel);
    }
    if (!m_yLabel.isEmpty()) {
        p.save();
        p.setFont(m_axisFont);
        p.setPen(QColor(255, 255, 255, kAxisTextAlpha));
        p.translate(10, r.center().y());
        p.rotate(-90);
        p.drawText(QRect(-40, -10, 80, 20), Qt::AlignCenter, m_yLabel);
        p.restore();
    }
}

// ============================================================
//  滑动标记（三角形 + 坐标标签，自动防溢出）
// ============================================================

void WaveformChart::drawMarker(QPainter &p, const QRect &r) {
    if (!m_markerVisible || m_data.isEmpty()) return;

    const QPointF px = dataToPixel(QPointF(m_markerX, m_markerY), r);
    if (px.x() < r.left() || px.x() > r.right() ||
        px.y() < r.top()  || px.y() > r.bottom()) return;

    // --- 三角形 ---
    constexpr int kTriH = 12, kTriHalfW = 7;
    QPolygonF tri;
    tri << QPointF(px.x(),              px.y())
        << QPointF(px.x() - kTriHalfW, px.y() - kTriH)
        << QPointF(px.x() + kTriHalfW, px.y() - kTriH);

    p.setPen(Qt::NoPen);
    p.setBrush(QColor(kMarkerR, kMarkerG, kMarkerB, kMarkerGlowAlpha));
    p.drawPolygon(tri.translated(-1, 1));

    p.setBrush(QColor(kMarkerR, kMarkerG, kMarkerB, kMarkerBodyAlpha));
    p.setPen(QPen(QColor(255, 220, 100), 1));
    p.drawPolygon(tri);

    // --- 坐标文本 ---
    const QString coordText = QStringLiteral("(%1, %2)")
        .arg(m_markerX, 0, 'f', 3).arg(m_markerY, 0, 'f', 3);

    p.setFont(m_labelFont);
    const QFontMetrics fm(m_labelFont);
    const int textW = fm.horizontalAdvance(coordText) + 12;
    const int textH = fm.height() + 6;

    // 默认标签在右侧；若超出右边界则翻转到左侧
    const bool overflowRight = (px.x() + kTriHalfW + 6 + textW > r.right());
    const int labelX = overflowRight
        ? static_cast<int>(px.x()) - kTriHalfW - 6 - textW
        : static_cast<int>(px.x()) + kTriHalfW + 6;
    const int labelY = static_cast<int>(px.y()) - kTriH - textH;

    const QRect labelRect(labelX, labelY, textW, textH);

    // 标签背景
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(kLabelBgR, kLabelBgG, kLabelBgB, kLabelBgAlpha));
    p.drawRoundedRect(labelRect, 5, 5);

    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(QColor(kLabelBorderR, kLabelBorderG, kLabelBorderB, kLabelBorderAlpha), 1));
    p.drawRoundedRect(labelRect, 5, 5);

    // 标签文字
    p.setPen(QColor(kLabelTextR, kLabelTextG, kLabelTextB));
    p.drawText(labelRect, Qt::AlignCenter, coordText);
}

// ============================================================
//  鼠标交互
// ============================================================

void WaveformChart::mousePressEvent(QMouseEvent *event) {
    if (event->button() != Qt::LeftButton || m_data.isEmpty()) {
        QWidget::mousePressEvent(event);
        return;
    }
    const QRect cr = chartRect();
    if (!cr.contains(event->pos())) return;

    m_dragging      = true;
    m_markerVisible = true;
    const QPointF d = pixelToData(QPointF(event->pos()), cr);
    m_markerX = d.x();
    m_markerY = interpolateY(m_markerX);
    update();
}

void WaveformChart::mouseMoveEvent(QMouseEvent *event) {
    if (!m_dragging || m_data.isEmpty()) return;

    const QRect cr = chartRect();
    const QPointF d = pixelToData(QPointF(event->pos()), cr);
    m_markerX = qBound(m_data.timeRange().min, d.x(), m_data.timeRange().max);
    m_markerY = interpolateY(m_markerX);
    update();
}

void WaveformChart::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton)
        m_dragging = false;
    QWidget::mouseReleaseEvent(event);
}
