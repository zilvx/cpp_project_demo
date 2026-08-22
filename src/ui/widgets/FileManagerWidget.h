#ifndef FILE_MANAGER_WIDGET_H
#define FILE_MANAGER_WIDGET_H

#include <QWidget>
#include <QVector>

class QLabel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QPushButton;
class QSplitter;
class QTreeWidget;
class QTreeWidgetItem;

/**
 * @brief 仿 Windows 风格文件管理器组件（对照设计稿实现）
 *
 * 布局：标题栏 → 导航/地址栏 → 侧边栏 + 内容区 → 状态栏
 * 功能：前进/后退/上级、地址跳转、快速访问、图标/列表视图、双击进入目录
 */
class FileManagerWidget : public QWidget {
    Q_OBJECT

public:
    explicit FileManagerWidget(QWidget *parent = nullptr);

    /// 导航到指定目录（不存在则忽略）
    void navigateTo(const QString &path);

    QString currentPath() const { return m_currentPath; }

    /// 当前选中项：若为文件则返回其绝对路径，否则返回空串
    QString selectedFilePath() const;

signals:
    /// 用户双击普通文件时发出（绝对路径）
    void fileActivated(const QString &path);

private slots:
    void onBack();
    void onForward();
    void onUp();
    void onAddressReturn();
    void onSidebarClicked(QTreeWidgetItem *item, int column);
    void onItemDoubleClicked(QListWidgetItem *item);
    void onItemSelectionChanged();
    void onViewIcon();
    void onViewList();
    void onViewDetails();
    void onRefresh();

private:
    enum class ViewMode { Icon, List, Details };

    void setupUI();
    void setupConnections();
    void loadStyle();
    void populateSidebar();
    void refreshContent();
    void updateNavButtons();
    void updateAddressBar();
    void updateStatusBar();
    void setViewMode(ViewMode mode);
    void pushHistory(const QString &path);

    static QString formatSize(qint64 bytes);
    static QIcon fileTypeIcon(const QString &path, bool isDir);

    // ---- 标题栏 ----
    QLabel      *m_titleLabel = nullptr;

    // ---- 工具栏 ----
    QPushButton *m_backBtn = nullptr;
    QPushButton *m_forwardBtn = nullptr;
    QPushButton *m_upBtn = nullptr;
    QPushButton *m_refreshBtn = nullptr;
    QLineEdit   *m_addressEdit = nullptr;

    // ---- 侧边栏 / 内容 ----
    QTreeWidget *m_sidebar = nullptr;
    QListWidget *m_fileList = nullptr;
    QPushButton *m_viewIconBtn = nullptr;
    QPushButton *m_viewListBtn = nullptr;
    QPushButton *m_viewDetailsBtn = nullptr;
    QLabel      *m_sortLabel = nullptr;

    // ---- 状态栏 ----
    QLabel *m_statusLeft = nullptr;
    QLabel *m_statusRight = nullptr;

    // ---- 状态 ----
    QString          m_currentPath;
    QVector<QString> m_history;
    int              m_historyIndex = -1;
    ViewMode         m_viewMode = ViewMode::Icon;
    bool             m_navigating = false;  // 避免历史栈重入
};

#endif // FILE_MANAGER_WIDGET_H
