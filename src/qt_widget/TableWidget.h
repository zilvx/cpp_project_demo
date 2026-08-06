#ifndef TABLE_WIDGET_H
#define TABLE_WIDGET_H

#include <QWidget>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QStringList>

class EditController;

class TableWidget : public QWidget {
    Q_OBJECT

public:
    explicit TableWidget(int rows = 5, int cols = 6, QWidget *parent = nullptr);
    ~TableWidget() override = default;

    void setCellText(int row, int col, const QString &text);
    QString cellText(int row, int col) const;

    bool isEditing() const;
    int  editingRow() const;
    int  editingCol() const;

public slots:
    /// 虚拟键盘按键处理（委托给 EditController）
    void handleKeyInput(const QString &key);

signals:
    void cellEditingStarted(int row, int col);
    void cellEditingFinished();

private:
    void setupUI(int rows, int cols);
    void populateSampleData();

    QTableWidget  *m_table;
    EditController *m_editCtrl;
};

#endif // TABLE_WIDGET_H
