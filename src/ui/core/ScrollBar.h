#ifndef SCROLL_BAR_H
#define SCROLL_BAR_H

#include <QAbstractScrollArea>
#include <QString>

/**
 * @brief 深色主题滚动条样式管理器
 *
 * 为 QAbstractScrollArea（QTableWidget、QTreeWidget、QTextEdit 等）
 * 提供统一的深色滚动条样式，独立于具体组件，支持复用。
 *
 * 用法:
 *   ScrollBarStyler::applyTo(m_table);
 */
class ScrollBarStyler {
public:
    ScrollBarStyler() = delete;

    /// 将深色滚动条样式应用到指定滚动区域
    static void applyTo(QAbstractScrollArea *area);
};

#endif // SCROLL_BAR_H
