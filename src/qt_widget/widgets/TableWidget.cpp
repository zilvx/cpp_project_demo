#include "TableWidget.h"
#include "../core/ScrollBar.h"
#include "../core/PenIconDelegate.h"
#include "../core/EditController.h"

#include <QApplication>
#include <QFile>
#include <QFont>

TableWidget::TableWidget(int rows, int cols, QWidget *parent)
    : QWidget(parent), m_table(nullptr), m_editCtrl(nullptr) {
    setupUI(rows, cols);
    populateSampleData();
}

void TableWidget::setupUI(int rows, int cols) {
    // 主布局
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    // 创建表格（数据行数 > 可视区域高度时自动出现垂直滚动条）
    m_table = new QTableWidget(rows, cols, this);

    // 从 Qt 资源文件加载表格样式表
    QFile styleFile(QStringLiteral(":/table.qss"));
    if (styleFile.open(QFile::ReadOnly | QFile::Text)) {
        m_table->setStyleSheet(QString::fromUtf8(styleFile.readAll()));
        styleFile.close();
    } else {
        qWarning("TableWidget: failed to load :/table.qss");
    }

    // 应用滚动条样式（独立于表格样式的 ScrollBar 组件）
    ScrollBarStyler::applyTo(m_table);

    // 安装钢笔图标委托：可编辑单元格右下角绘制 ✎ 图标
    m_table->setItemDelegate(new PenIconDelegate(m_table));

    // 设置水平表头
    QStringList headers;
    headers << QStringLiteral("姓名")
            << QStringLiteral("年龄")
            << QStringLiteral("部门")
            << QStringLiteral("职位")
            << QStringLiteral("入职日期")
            << QStringLiteral("备注");
    m_table->setHorizontalHeaderLabels(headers);

    // 设置垂直表头
    QStringList rowHeaders;
    for (int i = 1; i <= rows; ++i) {
        rowHeaders << QStringLiteral("第 %1 行").arg(i);
    }
    m_table->setVerticalHeaderLabels(rowHeaders);

    // 表头交互
    m_table->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->verticalHeader()->setDefaultAlignment(Qt::AlignCenter);

    // 表格属性
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    // 禁用默认行内编辑器，由虚拟键盘接管输入
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setShowGrid(true);

    // 编辑控制器（独立组件，管理编辑状态机）
    m_editCtrl = new EditController(m_table, this);
    connect(m_editCtrl, &EditController::editingStarted,
            this, &TableWidget::cellEditingStarted);
    connect(m_editCtrl, &EditController::editingFinished,
            this, &TableWidget::cellEditingFinished);

    layout->addWidget(m_table);

    // 窗口设置（高度限制使垂直滚动条生效）
    setWindowTitle(QStringLiteral("人员信息表 (%1行%2列)").arg(rows).arg(cols));
    resize(900, 360);
}

void TableWidget::populateSampleData() {
    const int totalRows = m_table->rowCount();
    const int totalCols = m_table->columnCount();

    // 预定义 5 行核心数据
    struct RowData {
        QString name, age, dept, position, date, remark;
    };
    const RowData base[] = {
        {QStringLiteral("张三"), QStringLiteral("28"), QStringLiteral("研发部"),
         QStringLiteral("高级工程师"), QStringLiteral("2022-03-15"), QStringLiteral("技术骨干")},
        {QStringLiteral("李四"), QStringLiteral("35"), QStringLiteral("产品部"),
         QStringLiteral("产品经理"),   QStringLiteral("2021-07-01"), QStringLiteral("")},
        {QStringLiteral("王五"), QStringLiteral("24"), QStringLiteral("设计部"),
         QStringLiteral("UI 设计师"),  QStringLiteral("2023-01-10"), QStringLiteral("实习生转正")},
        {QStringLiteral("赵六"), QStringLiteral("42"), QStringLiteral("管理层"),
         QStringLiteral("技术总监"),   QStringLiteral("2018-05-20"), QStringLiteral("部门负责人")},
        {QStringLiteral("孙七"), QStringLiteral("30"), QStringLiteral("测试部"),
         QStringLiteral("测试工程师"), QStringLiteral("2022-09-01"), QStringLiteral("")},
    };

    // 用中国常见姓氏生成扩充数据
    const QString surnames[] = {
        QStringLiteral("刘"), QStringLiteral("陈"), QStringLiteral("杨"),
        QStringLiteral("黄"), QStringLiteral("周"),  QStringLiteral("吴"),
        QStringLiteral("徐"), QStringLiteral("郑"), QStringLiteral("马"),
        QStringLiteral("朱"), QStringLiteral("胡"),  QStringLiteral("林"),
        QStringLiteral("何"), QStringLiteral("郭"),  QStringLiteral("高"),
    };
    const QString depts[] = {
        QStringLiteral("研发部"), QStringLiteral("产品部"), QStringLiteral("设计部"),
        QStringLiteral("测试部"), QStringLiteral("运维部"), QStringLiteral("市场部"),
    };
    const QString positions[] = {
        QStringLiteral("工程师"),     QStringLiteral("高级工程师"),
        QStringLiteral("产品经理"),   QStringLiteral("设计师"),
        QStringLiteral("测试工程师"), QStringLiteral("运营专员"),
    };

    for (int row = 0; row < totalRows; ++row) {
        QString name, age, dept, position, date, remark;

        if (row < 5) {
            // 使用预定义数据
            const auto &d = base[row];
            name     = d.name;
            age      = d.age;
            dept     = d.dept;
            position = d.position;
            date     = d.date;
            remark   = d.remark;
        } else {
            // 自动生成扩展数据
            name     = surnames[row % 15] + QStringLiteral("某");
            age      = QString::number(22 + (row * 3) % 25);
            dept     = depts[row % 6];
            position = positions[row % 6];
            date     = QStringLiteral("202%1-0%2-15")
                           .arg((row % 5))
                           .arg((row % 9) + 1, 2, 10, QChar('0'));
            remark   = (row % 3 == 0) ? QStringLiteral("") : QStringLiteral("扩展数据");
        }

        m_table->setItem(row, 0, new QTableWidgetItem(name));
        m_table->setItem(row, 1, new QTableWidgetItem(age));
        m_table->setItem(row, 2, new QTableWidgetItem(dept));
        m_table->setItem(row, 3, new QTableWidgetItem(position));
        m_table->setItem(row, 4, new QTableWidgetItem(date));
        m_table->setItem(row, 5, new QTableWidgetItem(remark));

        // 逐列设置可编辑性 + 钢笔图标
        for (int col = 0; col < totalCols; ++col) {
            QTableWidgetItem *item = m_table->item(row, col);
            item->setTextAlignment(Qt::AlignCenter);

            constexpr int kDeptCol = 2;
            if (col == kDeptCol) {
                // 部门列：移除可编辑标记，禁止双击编辑
                item->setFlags(item->flags() & ~Qt::ItemIsEditable);
            } else {
                // 其他列：可编辑；右下角钢笔图标由 PenIconDelegate 绘制
                item->setFlags(item->flags() | Qt::ItemIsEditable);
            }
        }
    }

    // 数据全部填充完成后再启用排序，避免插入过程中排序导致行号/数据错乱
    m_table->setSortingEnabled(true);
}

void TableWidget::setCellText(int row, int col, const QString &text) {
    auto *item = m_table->item(row, col);
    if (!item) return;

    item->setText(text);

    // 保持编辑标记：部门列只读，其他列可编辑
    constexpr int kDeptCol = 2;
    if (col == kDeptCol) {
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    } else {
        item->setFlags(item->flags() | Qt::ItemIsEditable);
    }
}

QString TableWidget::cellText(int row, int col) const {
    if (auto *item = m_table->item(row, col)) {
        return item->text();
    }
    return {};
}

// ====== 编辑功能 → 委托给 EditController ======

bool TableWidget::isEditing() const {
    return m_editCtrl->isEditing();
}

int TableWidget::editingRow() const {
    return m_editCtrl->editingRow();
}

int TableWidget::editingCol() const {
    return m_editCtrl->editingCol();
}

void TableWidget::handleKeyInput(const QString &key) {
    m_editCtrl->handleKeyInput(key);
}
