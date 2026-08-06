#ifndef PEN_ICON_DELEGATE_H
#define PEN_ICON_DELEGATE_H

#include <QStyledItemDelegate>

/**
 * @brief 可编辑单元格右下角钢笔图标绘制委托
 *
 * 对拥有 Qt::ItemIsEditable 标志的单元格，在右下角绘制半透明钢笔图标，
 * 提示用户该单元格可双击编辑。不可编辑的单元格不绘制。
 *
 * 用法:
 *   m_table->setItemDelegate(new PenIconDelegate(m_table));
 */
class PenIconDelegate : public QStyledItemDelegate {
    Q_OBJECT

public:
    explicit PenIconDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;

private:
    QPixmap m_penPixmap;
};

#endif // PEN_ICON_DELEGATE_H
