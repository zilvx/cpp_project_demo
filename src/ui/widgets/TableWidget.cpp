#include "TableWidget.h"
#include "../core/ScrollBar.h"
#include "../core/PenIconDelegate.h"
#include "../core/EditController.h"

#include <QApplication>
#include <QFile>
#include <QFont>
#include <spdlog/spdlog.h>
#include <QHeaderView>
#include <QVBoxLayout>

TableWidget::TableWidget(QWidget *parent)
    : QWidget(parent), m_table(nullptr), m_editCtrl(nullptr) {
    setupUI();
}

void TableWidget::setupUI() {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_table = new QTableWidget(0, 0, this);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    QFile styleFile(QStringLiteral(":/table.qss"));
    if (styleFile.open(QFile::ReadOnly | QFile::Text)) {
        m_table->setStyleSheet(QString::fromUtf8(styleFile.readAll()));
        styleFile.close();
    } else {
        spdlog::warn("TableWidget: failed to load :/table.qss");
    }

    ScrollBarStyler::applyTo(m_table);
    m_table->setItemDelegate(new PenIconDelegate(m_table));
    m_editCtrl = new EditController(m_table, this);
    connect(m_editCtrl, &EditController::editingStarted, this, &TableWidget::cellEditingStarted);
    connect(m_editCtrl, &EditController::editingFinished, this, &TableWidget::cellEditingFinished);
    layout->addWidget(m_table);
}

void TableWidget::loadData(const TableData &data) {
    m_table->clear();
    m_table->setSortingEnabled(false);
    m_table->setColumnCount(data.columnCount());
    m_table->setRowCount(data.rowCount());
    m_table->setHorizontalHeaderLabels(data.columns);

    for (int row = 0; row < data.rowCount(); ++row) {
        for (int col = 0; col < data.columnCount(); ++col) {
            auto *item = new QTableWidgetItem(data.rows[row][col]);
            if (col < data.editable.size() && !data.editable[col])
                item->setFlags(item->flags() & ~Qt::ItemIsEditable);
            else
                item->setFlags(item->flags() | Qt::ItemIsEditable);
            m_table->setItem(row, col, item);
        }
    }
    m_table->setSortingEnabled(true);
    m_table->resizeColumnsToContents();
}

void TableWidget::setCellText(int row, int col, const QString &text) {
    if (auto *item = m_table->item(row, col)) item->setText(text);
}

QString TableWidget::cellText(int row, int col) const {
    auto *item = m_table->item(row, col);
    return item ? item->text() : QString();
}

bool TableWidget::isEditing() const { return m_editCtrl && m_editCtrl->isEditing(); }
int  TableWidget::editingRow() const { return m_editCtrl ? m_editCtrl->editingRow() : -1; }
int  TableWidget::editingCol() const { return m_editCtrl ? m_editCtrl->editingCol() : -1; }

void TableWidget::handleKeyInput(const QString &key) {
    if (m_editCtrl) m_editCtrl->handleKeyInput(key);
}
