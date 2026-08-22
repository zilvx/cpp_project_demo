#include "FileManagerDialog.h"
#include "FileManagerWidget.h"

#include <QDialogButtonBox>
#include <QFile>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

FileManagerDialog::FileManagerDialog(QWidget *parent)
    : QDialog(parent) {
    setWindowTitle(QStringLiteral("文件管理器"));
    resize(780, 520);

    QFile styleFile(QStringLiteral(":/file_manager.qss"));
    if (styleFile.open(QFile::ReadOnly | QFile::Text))
        setStyleSheet(QString::fromUtf8(styleFile.readAll()));
    else
        qWarning("FileManagerDialog: failed to load :/file_manager.qss");

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // 内容区：复用文件管理器组件
    m_fileManager = new FileManagerWidget(this);
    root->addWidget(m_fileManager, 1);

    // 底部按钮栏
    auto *btnBar = new QWidget(this);
    btnBar->setObjectName(QStringLiteral("fmDlgBtnBar"));
    auto *btnLay = new QHBoxLayout(btnBar);
    btnLay->setContentsMargins(12, 8, 12, 8);
    btnLay->setSpacing(8);

    auto *hint = new QLabel(QStringLiteral("双击文件可直接选择"), btnBar);
    hint->setObjectName(QStringLiteral("fmDlgHint"));
    btnLay->addWidget(hint);
    btnLay->addStretch();

    auto *box = new QDialogButtonBox(QDialogButtonBox::Cancel, btnBar);
    m_okButton = box->addButton(QStringLiteral("选择"), QDialogButtonBox::AcceptRole);
    btnLay->addWidget(box);

    root->addWidget(btnBar);

    connect(box, &QDialogButtonBox::accepted, this, &FileManagerDialog::onAccept);
    connect(box, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_fileManager, &FileManagerWidget::fileActivated,
            this, &FileManagerDialog::onFileActivated);
}

void FileManagerDialog::setDialogTitle(const QString &title) {
    setWindowTitle(title);
}

void FileManagerDialog::setInitialPath(const QString &path) {
    if (!path.isEmpty())
        m_fileManager->navigateTo(path);
}

void FileManagerDialog::onFileActivated(const QString &path) {
    m_selectedFile = path;
    accept();
}

void FileManagerDialog::onAccept() {
    const QString path = m_fileManager->selectedFilePath();
    if (path.isEmpty())
        return;
    m_selectedFile = path;
    accept();
}

QString FileManagerDialog::selectFile(QWidget *parent, const QString &title) {
    FileManagerDialog dlg(parent);
    dlg.setDialogTitle(title);
    if (dlg.exec() == QDialog::Accepted)
        return dlg.selectedFile();
    return QString();
}