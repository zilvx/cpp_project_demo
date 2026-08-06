#include "PenIconDelegate.h"

#include <QPainter>
#include <QTableWidget>

PenIconDelegate::PenIconDelegate(QObject *parent)
    : QStyledItemDelegate(parent) {
    // 预渲染钢笔图标 pixmap
    constexpr int kIconSize = 16;
    m_penPixmap = QPixmap(kIconSize, kIconSize);
    m_penPixmap.fill(Qt::transparent);

    QPainter p(&m_penPixmap);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QColor(255, 255, 255, 140));
    QFont font;
    font.setPixelSize(12);
    p.setFont(font);
    p.drawText(m_penPixmap.rect(), Qt::AlignCenter, QStringLiteral("✎"));
    p.end();
}

void PenIconDelegate::paint(QPainter *painter,
                             const QStyleOptionViewItem &option,
                             const QModelIndex &index) const {
    // 1. 先绘制默认样式（文字、背景、选中高亮等）
    QStyledItemDelegate::paint(painter, option, index);

    // 2. 仅在拥有可编辑标志的单元格右下角绘制钢笔图标
    const auto flags = index.flags();
    if (!(flags & Qt::ItemIsEditable)) return;

    constexpr int kMargin = 4;
    const QRect cellRect = option.rect;
    const int iconW = m_penPixmap.width();
    const int iconH = m_penPixmap.height();

    const QPoint topLeft(
        cellRect.right()  - iconW - kMargin,
        cellRect.bottom() - iconH - kMargin
    );
    painter->drawPixmap(QRect(topLeft, m_penPixmap.size()), m_penPixmap);
}
