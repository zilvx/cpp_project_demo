#include "EditController.h"

EditController::EditController(QTableWidget *table, QObject *parent)
    : QObject(parent), m_table(table) {
    Q_ASSERT(m_table);
    connect(m_table, &QTableWidget::cellClicked,
            this, &EditController::onCellClicked);
}

bool EditController::isEditing() const {
    return m_row >= 0 && m_col >= 0;
}

void EditController::onCellClicked(int row, int col) {
    auto *item = m_table->item(row, col);
    if (!item) return;

    if (!(item->flags() & Qt::ItemIsEditable)) {
        if (isEditing()) finishEdit(true);
        return;
    }

    if (row == m_row && col == m_col) return;

    if (isEditing()) finishEdit(true);

    m_row = row;
    m_col = col;
    m_originalText = item->text();

    m_table->setCurrentCell(row, col);
    emit editingStarted(row, col);
}

void EditController::handleKeyInput(const QString &key) {
    if (!isEditing()) return;

    auto *item = m_table->item(m_row, m_col);
    if (!item) return;

    if (key == QStringLiteral("Backspace")) {
        QString t = item->text();
        if (!t.isEmpty()) {
            t.chop(1);
            item->setText(t);
        }
    } else if (key == QStringLiteral("Space")) {
        item->setText(item->text() + QStringLiteral(" "));
    } else if (key == QStringLiteral("Enter") ||
               key == QStringLiteral("Return")) {
        finishEdit(true);
    } else if (key == QStringLiteral("Escape")) {
        finishEdit(false);
    } else if (key == QStringLiteral("Tab")) {
        const int totalCols = m_table->columnCount();
        int nextCol = m_col;
        for (int attempt = 0; attempt < totalCols; ++attempt) {
            nextCol = (nextCol + 1) % totalCols;
            auto *nextItem = m_table->item(m_row, nextCol);
            if (nextItem && (nextItem->flags() & Qt::ItemIsEditable)) {
                finishEdit(true);
                onCellClicked(m_row, nextCol);
                return;
            }
        }
        finishEdit(true);
    } else if (key.length() == 1) {
        item->setText(item->text() + key);
    }
}

void EditController::finishEdit(bool accept) {
    if (!isEditing()) return;

    auto *item = m_table->item(m_row, m_col);
    if (item) {
        if (accept) {
            if (item->text().isEmpty()) {
                item->setText(m_originalText);
            }
        } else {
            item->setText(m_originalText);
        }
    }

    const int oldRow = m_row;
    const int oldCol = m_col;
    m_row = -1;
    m_col = -1;
    m_originalText.clear();

    m_table->setCurrentCell(oldRow, oldCol);

    emit editingFinished();
}
