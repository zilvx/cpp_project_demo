#ifndef EDIT_CONTROLLER_H
#define EDIT_CONTROLLER_H

#include <QObject>
#include <QString>
#include <QTableWidget>

/**
 * @brief 表格单元格的虚拟键盘编辑控制器
 *
 * 将编辑状态机从 TableWidget 中分离，职责单一，可独立测试。
 * - 单元格点击 → 开始编辑
 * - 键盘按键 → 修改内容
 * - Enter 确认 / Esc 取消
 * - Tab 跳转下一可编辑列
 *
 * 用法:
 *   auto *ctrl = new EditController(table, this);
 *   connect(ctrl, &EditController::editingStarted, ...);
 *   connect(ctrl, &EditController::editingFinished, ...);
 *   // 键盘输入:
 *   ctrl->handleKeyInput(key);
 */
class EditController : public QObject {
    Q_OBJECT

public:
    explicit EditController(QTableWidget *table, QObject *parent = nullptr);

    bool isEditing() const;
    int  editingRow() const { return m_row; }
    int  editingCol() const { return m_col; }

public slots:
    void onCellClicked(int row, int col);
    void handleKeyInput(const QString &key);

signals:
    void editingStarted(int row, int col);
    void editingFinished();

private:
    void finishEdit(bool accept);

    QTableWidget *m_table;
    int    m_row = -1;
    int    m_col = -1;
    QString m_originalText;
};

#endif // EDIT_CONTROLLER_H
