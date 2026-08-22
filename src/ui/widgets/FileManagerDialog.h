#ifndef FILE_MANAGER_DIALOG_H
#define FILE_MANAGER_DIALOG_H

#include <QDialog>

class FileManagerWidget;
class QPushButton;

/**
 * @brief 文件管理器弹框（复用 FileManagerWidget 作为内容）
 *
 * 交互：双击文件即选中并关闭；或先单击选中再点“选择”按钮确认。
 * 可用静态方法 selectFile() 一键弹出并返回所选文件绝对路径。
 */
class FileManagerDialog : public QDialog {
    Q_OBJECT

public:
    explicit FileManagerDialog(QWidget *parent = nullptr);

    /// 设置弹框标题
    void setDialogTitle(const QString &title);

    /// 设置初始目录（不存在则忽略，由 FileManagerWidget 内部校验）
    void setInitialPath(const QString &path);

    /// 返回本次选择的文件绝对路径；未选择则为空串
    QString selectedFile() const { return m_selectedFile; }

    /// 便捷静态方法：弹出弹框，选择成功返回路径，取消返回空串
    static QString selectFile(QWidget *parent = nullptr,
                              const QString &title = QStringLiteral("选择文件"));

private slots:
    void onFileActivated(const QString &path);
    void onAccept();

private:
    FileManagerWidget *m_fileManager = nullptr;
    QPushButton       *m_okButton = nullptr;
    QString            m_selectedFile;
};

#endif // FILE_MANAGER_DIALOG_H