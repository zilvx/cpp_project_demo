#include "SampleDataProvider.h"

SampleDataProvider::SampleDataProvider(int rowCount, QObject *parent)
    : IDataProvider(parent), m_rowCount(rowCount) {}

void SampleDataProvider::fetchData() {
    TableData data;
    data.columns = {
        QStringLiteral("姓名"), QStringLiteral("年龄"), QStringLiteral("部门"),
        QStringLiteral("职位"), QStringLiteral("入职日期"), QStringLiteral("备注"),
    };
    data.editable = {true, true, false, true, true, true};

    struct { QString name, age, dept, position, date, remark; } base[] = {
        {QStringLiteral("张三"), QStringLiteral("28"), QStringLiteral("研发部"),
         QStringLiteral("高级工程师"), QStringLiteral("2022-03-15"), QStringLiteral("技术骨干")},
        {QStringLiteral("李四"), QStringLiteral("35"), QStringLiteral("产品部"),
         QStringLiteral("产品经理"), QStringLiteral("2021-07-01"), QStringLiteral("")},
        {QStringLiteral("王五"), QStringLiteral("24"), QStringLiteral("设计部"),
         QStringLiteral("UI 设计师"), QStringLiteral("2023-01-10"), QStringLiteral("实习生转正")},
        {QStringLiteral("赵六"), QStringLiteral("42"), QStringLiteral("管理层"),
         QStringLiteral("技术总监"), QStringLiteral("2018-05-20"), QStringLiteral("部门负责人")},
        {QStringLiteral("孙七"), QStringLiteral("30"), QStringLiteral("测试部"),
         QStringLiteral("测试工程师"), QStringLiteral("2022-09-01"), QStringLiteral("")},
    };

    const QStringList surnames  = {QStringLiteral("刘"), QStringLiteral("陈"), QStringLiteral("杨"),
                                   QStringLiteral("黄"), QStringLiteral("周"), QStringLiteral("吴"),
                                   QStringLiteral("徐"), QStringLiteral("郑"), QStringLiteral("马"),
                                   QStringLiteral("朱"), QStringLiteral("胡"), QStringLiteral("林"),
                                   QStringLiteral("何"), QStringLiteral("郭"), QStringLiteral("高")};
    const QStringList depts     = {QStringLiteral("研发部"), QStringLiteral("产品部"), QStringLiteral("设计部"),
                                   QStringLiteral("测试部"), QStringLiteral("运维部"), QStringLiteral("市场部")};
    const QStringList positions = {QStringLiteral("工程师"), QStringLiteral("高级工程师"),
                                   QStringLiteral("产品经理"), QStringLiteral("设计师"),
                                   QStringLiteral("测试工程师"), QStringLiteral("运营专员")};

    for (int row = 0; row < m_rowCount; ++row) {
        QStringList cells;
        if (row < 5) {
            const auto &d = base[row];
            cells = {d.name, d.age, d.dept, d.position, d.date, d.remark};
        } else {
            cells = {
                surnames[row % 15] + QStringLiteral("某"),
                QString::number(22 + (row * 3) % 25),
                depts[row % 6], positions[row % 6],
                QStringLiteral("202%1-0%2-15").arg(row % 5).arg((row % 9) + 1, 2, 10, QChar('0')),
                (row % 3 == 0) ? QStringLiteral("") : QStringLiteral("扩展数据"),
            };
        }
        data.rows.append(cells);
    }
    emit dataReady(data);
}
