#ifndef TABLE_WIDGET_H
#define TABLE_WIDGET_H

#include <QWidget>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QStringList>
#include "../core/TableData.h"

class EditController;

class TableWidget : public QWidget {
    Q_OBJECT

public:
    explicit TableWidget(QWidget *parent = nullptr);
    ~TableWidget() override = default;

    void loadData(const TableData &data);
    void setCellText(int row, int col, const QString &text);
    QString cellText(int row, int col) const;
    bool isEditing() const;
    int  editingRow() const;
    int  editingCol() const;

public slots:
    void handleKeyInput(const QString &key);

signals:
    void cellEditingStarted(int row, int col);
    void cellEditingFinished();

private:
    void setupUI();
    QTableWidget  *m_table;
    EditController *m_editCtrl;
};

#endif
