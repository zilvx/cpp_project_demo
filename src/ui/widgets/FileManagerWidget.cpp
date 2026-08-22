#include "FileManagerWidget.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileIconProvider>
#include <QFileInfo>
#include <QFont>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSplitter>
#include <QStorageInfo>
#include <QStyle>
#include <QTreeWidget>
#include <QVBoxLayout>

#include <spdlog/spdlog.h>

namespace {

constexpr int kRolePath = Qt::UserRole;
constexpr int kRoleIsDir = Qt::UserRole + 1;

QString homePath() { return QDir::homePath(); }

QString downloadsPath() {
    const QString p = homePath() + QStringLiteral("/Downloads");
    return QDir(p).exists() ? p : homePath();
}

QString documentsPath() {
    const QString p = homePath() + QStringLiteral("/Documents");
    return QDir(p).exists() ? p : homePath();
}

QString picturesPath() {
    const QString p = homePath() + QStringLiteral("/Pictures");
    return QDir(p).exists() ? p : homePath();
}

QString desktopPath() {
    const QString p = homePath() + QStringLiteral("/Desktop");
    return QDir(p).exists() ? p : homePath();
}

} // namespace

FileManagerWidget::FileManagerWidget(QWidget *parent) : QWidget(parent) {
    setupUI();
    loadStyle();
    setupConnections();
    populateSidebar();
    navigateTo(documentsPath());
}

void FileManagerWidget::setupUI() {
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // ---- 标题栏 ----
    auto *titleBar = new QWidget;
    titleBar->setObjectName(QStringLiteral("fmTitleBar"));
    auto *titleLay = new QHBoxLayout(titleBar);
    titleLay->setContentsMargins(12, 6, 8, 6);
    titleLay->setSpacing(8);

    m_titleLabel = new QLabel(QStringLiteral("📁  文件管理器"));
    m_titleLabel->setObjectName(QStringLiteral("fmTitleText"));
    titleLay->addWidget(m_titleLabel);
    titleLay->addStretch();

    auto makeWinBtn = [](const QString &text, const QString &obj) {
        auto *b = new QPushButton(text);
        b->setObjectName(obj);
        b->setFixedSize(30, 26);
        b->setFocusPolicy(Qt::NoFocus);
        b->setFlat(true);
        return b;
    };
    titleLay->addWidget(makeWinBtn(QStringLiteral("−"), QStringLiteral("fmWinMin")));
    titleLay->addWidget(makeWinBtn(QStringLiteral("□"), QStringLiteral("fmWinMax")));
    auto *closeBtn = makeWinBtn(QStringLiteral("×"), QStringLiteral("fmWinClose"));
    titleLay->addWidget(closeBtn);
    root->addWidget(titleBar);

    // ---- 工具栏 ----
    auto *toolBar = new QWidget;
    toolBar->setObjectName(QStringLiteral("fmToolBar"));
    auto *toolLay = new QHBoxLayout(toolBar);
    toolLay->setContentsMargins(12, 6, 12, 6);
    toolLay->setSpacing(6);

    auto makeNavBtn = [](const QString &text, const QString &tip) {
        auto *b = new QPushButton(text);
        b->setObjectName(QStringLiteral("fmNavBtn"));
        b->setFixedSize(32, 28);
        b->setToolTip(tip);
        b->setFocusPolicy(Qt::NoFocus);
        b->setFlat(true);
        return b;
    };
    m_backBtn = makeNavBtn(QStringLiteral("←"), QStringLiteral("后退"));
    m_forwardBtn = makeNavBtn(QStringLiteral("→"), QStringLiteral("前进"));
    m_upBtn = makeNavBtn(QStringLiteral("↑"), QStringLiteral("上级目录"));
    m_refreshBtn = makeNavBtn(QStringLiteral("↻"), QStringLiteral("刷新"));
    toolLay->addWidget(m_backBtn);
    toolLay->addWidget(m_forwardBtn);
    toolLay->addWidget(m_upBtn);
    toolLay->addWidget(m_refreshBtn);

    auto *addrWrap = new QWidget;
    addrWrap->setObjectName(QStringLiteral("fmAddressBar"));
    auto *addrLay = new QHBoxLayout(addrWrap);
    addrLay->setContentsMargins(10, 2, 10, 2);
    addrLay->setSpacing(8);
    auto *addrIcon = new QLabel(QStringLiteral("📂"));
    addrIcon->setObjectName(QStringLiteral("fmAddressIcon"));
    m_addressEdit = new QLineEdit;
    m_addressEdit->setObjectName(QStringLiteral("fmAddressEdit"));
    m_addressEdit->setPlaceholderText(QStringLiteral("输入路径..."));
    addrLay->addWidget(addrIcon);
    addrLay->addWidget(m_addressEdit, 1);
    toolLay->addWidget(addrWrap, 1);
    root->addWidget(toolBar);

    // ---- 主体：侧边栏 + 内容 ----
    auto *splitter = new QSplitter(Qt::Horizontal);
    splitter->setObjectName(QStringLiteral("fmSplitter"));
    splitter->setChildrenCollapsible(false);

    m_sidebar = new QTreeWidget;
    m_sidebar->setObjectName(QStringLiteral("fmSidebar"));
    m_sidebar->setHeaderHidden(true);
    m_sidebar->setRootIsDecorated(false);
    m_sidebar->setIndentation(0);
    m_sidebar->setFocusPolicy(Qt::NoFocus);
    m_sidebar->setMinimumWidth(160);
    m_sidebar->setMaximumWidth(280);
    splitter->addWidget(m_sidebar);

    auto *content = new QWidget;
    content->setObjectName(QStringLiteral("fmContent"));
    auto *contentLay = new QVBoxLayout(content);
    contentLay->setContentsMargins(12, 10, 12, 8);
    contentLay->setSpacing(8);

    auto *contentHeader = new QHBoxLayout;
    contentHeader->setSpacing(4);
    auto makeViewBtn = [](const QString &text, const QString &tip) {
        auto *b = new QPushButton(text);
        b->setObjectName(QStringLiteral("fmViewBtn"));
        b->setFixedSize(30, 26);
        b->setToolTip(tip);
        b->setFocusPolicy(Qt::NoFocus);
        b->setFlat(true);
        b->setCheckable(true);
        return b;
    };
    m_viewIconBtn = makeViewBtn(QStringLiteral("▦"), QStringLiteral("大图标"));
    m_viewListBtn = makeViewBtn(QStringLiteral("☰"), QStringLiteral("列表"));
    m_viewDetailsBtn = makeViewBtn(QStringLiteral("≡"), QStringLiteral("详细信息"));
    contentHeader->addWidget(m_viewIconBtn);
    contentHeader->addWidget(m_viewListBtn);
    contentHeader->addWidget(m_viewDetailsBtn);
    contentHeader->addStretch();
    m_sortLabel = new QLabel(QStringLiteral("⇅  按名称排序"));
    m_sortLabel->setObjectName(QStringLiteral("fmSortLabel"));
    contentHeader->addWidget(m_sortLabel);
    contentLay->addLayout(contentHeader);

    m_fileList = new QListWidget;
    m_fileList->setObjectName(QStringLiteral("fmFileList"));
    m_fileList->setMovement(QListView::Static);
    m_fileList->setResizeMode(QListView::Adjust);
    m_fileList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_fileList->setWordWrap(true);
    m_fileList->setUniformItemSizes(false);
    contentLay->addWidget(m_fileList, 1);
    splitter->addWidget(content);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({220, 780});
    root->addWidget(splitter, 1);

    // ---- 状态栏 ----
    auto *statusBar = new QWidget;
    statusBar->setObjectName(QStringLiteral("fmStatusBar"));
    auto *statusLay = new QHBoxLayout(statusBar);
    statusLay->setContentsMargins(16, 4, 16, 4);
    m_statusLeft = new QLabel;
    m_statusLeft->setObjectName(QStringLiteral("fmStatusLeft"));
    m_statusRight = new QLabel(QStringLiteral("文件管理器"));
    m_statusRight->setObjectName(QStringLiteral("fmStatusRight"));
    statusLay->addWidget(m_statusLeft);
    statusLay->addStretch();
    statusLay->addWidget(m_statusRight);
    root->addWidget(statusBar);

    setViewMode(ViewMode::Icon);
    setMinimumSize(720, 480);
}

void FileManagerWidget::loadStyle() {
    QFile f(QStringLiteral(":/file_manager.qss"));
    if (f.open(QFile::ReadOnly | QFile::Text)) {
        setStyleSheet(QString::fromUtf8(f.readAll()));
    } else {
        spdlog::warn("FileManagerWidget: failed to load :/file_manager.qss");
    }
}

void FileManagerWidget::setupConnections() {
    connect(m_backBtn, &QPushButton::clicked, this, &FileManagerWidget::onBack);
    connect(m_forwardBtn, &QPushButton::clicked, this, &FileManagerWidget::onForward);
    connect(m_upBtn, &QPushButton::clicked, this, &FileManagerWidget::onUp);
    connect(m_refreshBtn, &QPushButton::clicked, this, &FileManagerWidget::onRefresh);
    connect(m_addressEdit, &QLineEdit::returnPressed, this, &FileManagerWidget::onAddressReturn);
    connect(m_sidebar, &QTreeWidget::itemClicked, this, &FileManagerWidget::onSidebarClicked);
    connect(m_fileList, &QListWidget::itemDoubleClicked, this, &FileManagerWidget::onItemDoubleClicked);
    connect(m_fileList, &QListWidget::itemSelectionChanged, this, &FileManagerWidget::onItemSelectionChanged);
    connect(m_viewIconBtn, &QPushButton::clicked, this, &FileManagerWidget::onViewIcon);
    connect(m_viewListBtn, &QPushButton::clicked, this, &FileManagerWidget::onViewList);
    connect(m_viewDetailsBtn, &QPushButton::clicked, this, &FileManagerWidget::onViewDetails);
}

void FileManagerWidget::populateSidebar() {
    m_sidebar->clear();

    auto addSection = [this](const QString &title) -> QTreeWidgetItem * {
        auto *sec = new QTreeWidgetItem(m_sidebar);
        sec->setText(0, title);
        sec->setFlags(Qt::ItemIsEnabled);
        sec->setData(0, kRolePath, QString());
        QFont font = sec->font(0);
        font.setPointSize(10);
        font.setBold(true);
        sec->setFont(0, font);
        sec->setForeground(0, QColor(QStringLiteral("#5a5a5a")));
        sec->setExpanded(true);
        return sec;
    };

    auto addItem = [](QTreeWidgetItem *parent, const QString &text,
                      const QString &path, const QString &iconEmoji) {
        auto *item = new QTreeWidgetItem(parent);
        item->setText(0, iconEmoji + QStringLiteral("  ") + text);
        item->setData(0, kRolePath, path);
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        item->setSizeHint(0, QSize(0, 28));
        return item;
    };

    auto *quick = addSection(QStringLiteral("快速访问"));
    addItem(quick, QStringLiteral("下载"), downloadsPath(), QStringLiteral("⬇"));
    addItem(quick, QStringLiteral("文档"), documentsPath(), QStringLiteral("📄"));
    addItem(quick, QStringLiteral("图片"), picturesPath(), QStringLiteral("🖼"));
    addItem(quick, QStringLiteral("桌面"), desktopPath(), QStringLiteral("🖥"));
    addItem(quick, QStringLiteral("主目录"), homePath(), QStringLiteral("🏠"));

    auto *sep1 = new QTreeWidgetItem(m_sidebar);
    sep1->setFlags(Qt::NoItemFlags);
    sep1->setText(0, QStringLiteral("────────────"));
    sep1->setForeground(0, QColor(QStringLiteral("#d9dce1")));

    auto *computer = addSection(QStringLiteral("此电脑"));
    const auto volumes = QStorageInfo::mountedVolumes();
    bool anyVol = false;
    for (const QStorageInfo &vol : volumes) {
        if (!vol.isValid() || !vol.isReady())
            continue;
        // 跳过只读系统卷中的常见伪挂载
        const QString root = QDir::cleanPath(vol.rootPath());
        if (root.startsWith(QStringLiteral("/System")) ||
            root.startsWith(QStringLiteral("/private/var/vm")) ||
            root == QStringLiteral("/dev"))
            continue;
        QString name = vol.displayName();
        if (name.isEmpty())
            name = root;
        if (root == QStringLiteral("/"))
            name = QStringLiteral("本地磁盘 (/)");
        addItem(computer, name, root, QStringLiteral("💾"));
        anyVol = true;
    }
    if (!anyVol)
        addItem(computer, QStringLiteral("本地磁盘"), QStringLiteral("/"), QStringLiteral("💾"));

    auto *sep2 = new QTreeWidgetItem(m_sidebar);
    sep2->setFlags(Qt::NoItemFlags);
    sep2->setText(0, QStringLiteral("────────────"));
    sep2->setForeground(0, QColor(QStringLiteral("#d9dce1")));

    auto *net = addSection(QStringLiteral("网络"));
    addItem(net, QStringLiteral("网络邻居"), QString(), QStringLiteral("🌐"));
    // macOS iCloud Drive 常见路径
    const QString iCloud = homePath() + QStringLiteral("/Library/Mobile Documents/com~apple~CloudDocs");
    if (QDir(iCloud).exists())
        addItem(net, QStringLiteral("iCloud Drive"), iCloud, QStringLiteral("☁"));
}

void FileManagerWidget::navigateTo(const QString &path) {
    const QString cleaned = QDir::cleanPath(path);
    QFileInfo info(cleaned);
    if (!info.exists() || !info.isDir()) {
        spdlog::warn("FileManagerWidget: 无法打开目录 {}", cleaned.toStdString());
        return;
    }
    if (!m_navigating)
        pushHistory(cleaned);
    m_currentPath = cleaned;
    refreshContent();
    updateAddressBar();
    updateNavButtons();
    updateStatusBar();
}

void FileManagerWidget::pushHistory(const QString &path) {
    // 截断前进分支
    if (m_historyIndex >= 0 && m_historyIndex < m_history.size() - 1)
        m_history.resize(m_historyIndex + 1);
    if (!m_history.isEmpty() && m_history.last() == path) {
        m_historyIndex = m_history.size() - 1;
        return;
    }
    m_history.push_back(path);
    m_historyIndex = m_history.size() - 1;
}

void FileManagerWidget::refreshContent() {
    m_fileList->clear();

    QDir dir(m_currentPath);
    dir.setFilter(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::AllDirs);
    dir.setSorting(QDir::DirsFirst | QDir::Name | QDir::IgnoreCase);

    const QFileInfoList entries = dir.entryInfoList();
    for (const QFileInfo &fi : entries) {
        auto *item = new QListWidgetItem;
        const bool isDir = fi.isDir();
        QString name = fi.fileName();
        if (m_viewMode == ViewMode::Details) {
            const QString size = isDir ? QStringLiteral("—") : formatSize(fi.size());
            const QString mtime = fi.lastModified().toString(QStringLiteral("yyyy-MM-dd hh:mm"));
            name = QStringLiteral("%1\n%2    %3").arg(fi.fileName(), size, mtime);
        }
        item->setText(name);
        item->setToolTip(fi.absoluteFilePath());
        item->setData(kRolePath, fi.absoluteFilePath());
        item->setData(kRoleIsDir, isDir);
        item->setIcon(fileTypeIcon(fi.absoluteFilePath(), isDir));
        if (m_viewMode == ViewMode::Icon)
            item->setSizeHint(QSize(110, 96));
        else if (m_viewMode == ViewMode::List)
            item->setSizeHint(QSize(200, 36));
        else
            item->setSizeHint(QSize(280, 48));
        m_fileList->addItem(item);
    }
    updateStatusBar();
}

void FileManagerWidget::updateNavButtons() {
    m_backBtn->setEnabled(m_historyIndex > 0);
    m_forwardBtn->setEnabled(m_historyIndex >= 0 && m_historyIndex < m_history.size() - 1);
    QDir dir(m_currentPath);
    m_upBtn->setEnabled(dir.cdUp());  // 能上移则启用
}

void FileManagerWidget::updateAddressBar() {
    // 显示更友好的路径：用 > 分隔
    QString display = m_currentPath;
#ifdef Q_OS_UNIX
    if (display.startsWith(homePath()))
        display.replace(0, homePath().size(), QStringLiteral("~"));
#endif
    display.replace(QLatin1Char('/'), QStringLiteral(" > "));
    if (display.startsWith(QStringLiteral(" > ")))
        display = QStringLiteral("/") + display.mid(2);
    m_addressEdit->setText(display);
    m_addressEdit->setCursorPosition(0);
    // 实际路径存 tooltip，便于复制
    m_addressEdit->setToolTip(m_currentPath);
}

void FileManagerWidget::updateStatusBar() {
    const int total = m_fileList->count();
    const int selected = m_fileList->selectedItems().size();
    QString left;
    if (selected > 0)
        left = QStringLiteral("%1 个项目已选择  |  共 %2 项").arg(selected).arg(total);
    else
        left = QStringLiteral("%1 个项目").arg(total);

    // 可用空间：取当前路径所在卷
    QStorageInfo storage(m_currentPath);
    if (storage.isValid() && storage.isReady()) {
        left += QStringLiteral("  |  可用空间: %1").arg(formatSize(storage.bytesAvailable()));
    }
    m_statusLeft->setText(left);
}

void FileManagerWidget::setViewMode(ViewMode mode) {
    m_viewMode = mode;
    m_viewIconBtn->setChecked(mode == ViewMode::Icon);
    m_viewListBtn->setChecked(mode == ViewMode::List);
    m_viewDetailsBtn->setChecked(mode == ViewMode::Details);

    switch (mode) {
    case ViewMode::Icon:
        m_fileList->setViewMode(QListView::IconMode);
        m_fileList->setIconSize(QSize(48, 48));
        m_fileList->setSpacing(8);
        m_fileList->setGridSize(QSize(110, 100));
        m_fileList->setWrapping(true);
        m_fileList->setFlow(QListView::LeftToRight);
        break;
    case ViewMode::List:
        m_fileList->setViewMode(QListView::ListMode);
        m_fileList->setIconSize(QSize(22, 22));
        m_fileList->setSpacing(2);
        m_fileList->setGridSize(QSize());
        m_fileList->setWrapping(false);
        m_fileList->setFlow(QListView::TopToBottom);
        break;
    case ViewMode::Details:
        m_fileList->setViewMode(QListView::ListMode);
        m_fileList->setIconSize(QSize(22, 22));
        m_fileList->setSpacing(2);
        m_fileList->setGridSize(QSize());
        m_fileList->setWrapping(false);
        m_fileList->setFlow(QListView::TopToBottom);
        break;
    }
    if (!m_currentPath.isEmpty())
        refreshContent();
}

void FileManagerWidget::onBack() {
    if (m_historyIndex <= 0) return;
    m_navigating = true;
    --m_historyIndex;
    navigateTo(m_history.at(m_historyIndex));
    m_navigating = false;
    updateNavButtons();
}

void FileManagerWidget::onForward() {
    if (m_historyIndex < 0 || m_historyIndex >= m_history.size() - 1) return;
    m_navigating = true;
    ++m_historyIndex;
    navigateTo(m_history.at(m_historyIndex));
    m_navigating = false;
    updateNavButtons();
}

void FileManagerWidget::onUp() {
    QDir dir(m_currentPath);
    if (!dir.cdUp()) return;
    navigateTo(dir.absolutePath());
}

void FileManagerWidget::onAddressReturn() {
    QString text = m_addressEdit->text().trimmed();
    // 支持 "a > b > c" 与 ~ 展开
    text.replace(QStringLiteral(" > "), QStringLiteral("/"));
    text.replace(QStringLiteral(">"), QStringLiteral("/"));
    if (text.startsWith(QLatin1Char('~')))
        text = homePath() + text.mid(1);
    if (text.isEmpty())
        text = QStringLiteral("/");
    navigateTo(text);
}

void FileManagerWidget::onSidebarClicked(QTreeWidgetItem *item, int) {
    if (!item) return;
    const QString path = item->data(0, kRolePath).toString();
    if (path.isEmpty()) return;
    navigateTo(path);
}

void FileManagerWidget::onItemDoubleClicked(QListWidgetItem *item) {
    if (!item) return;
    const QString path = item->data(kRolePath).toString();
    const bool isDir = item->data(kRoleIsDir).toBool();
    if (isDir) {
        navigateTo(path);
    } else {
        emit fileActivated(path);
        spdlog::info("FileManagerWidget: 激活文件 {}", path.toStdString());
    }
}

void FileManagerWidget::onItemSelectionChanged() {
    updateStatusBar();
}

QString FileManagerWidget::selectedFilePath() const {
    auto *item = m_fileList->currentItem();
    if (!item)
        return QString();
    if (item->data(kRoleIsDir).toBool())
        return QString();
    return item->data(kRolePath).toString();
}

void FileManagerWidget::onViewIcon() { setViewMode(ViewMode::Icon); }
void FileManagerWidget::onViewList() { setViewMode(ViewMode::List); }
void FileManagerWidget::onViewDetails() { setViewMode(ViewMode::Details); }
void FileManagerWidget::onRefresh() { refreshContent(); }

QString FileManagerWidget::formatSize(qint64 bytes) {
    const double kb = 1024.0;
    const double mb = kb * 1024.0;
    const double gb = mb * 1024.0;
    if (bytes >= static_cast<qint64>(gb))
        return QString::number(bytes / gb, 'f', 1) + QStringLiteral(" GB");
    if (bytes >= static_cast<qint64>(mb))
        return QString::number(bytes / mb, 'f', 1) + QStringLiteral(" MB");
    if (bytes >= static_cast<qint64>(kb))
        return QString::number(bytes / kb, 'f', 1) + QStringLiteral(" KB");
    return QString::number(bytes) + QStringLiteral(" B");
}

QIcon FileManagerWidget::fileTypeIcon(const QString &path, bool isDir) {
    static QFileIconProvider provider;
    QFileInfo fi(path);
    if (isDir)
        return provider.icon(QFileIconProvider::Folder);
    return provider.icon(fi);
}
