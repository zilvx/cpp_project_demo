#include <gtest/gtest.h>
#include <QStringList>
#include <QVector>

#include "TableData.h"

TEST(TableDataTest, EmptyIsInvalid) {
    TableData d;
    EXPECT_FALSE(d.isValid());
    EXPECT_EQ(d.rowCount(), 0);
    EXPECT_EQ(d.columnCount(), 0);
}

TEST(TableDataTest, ValidWhenColumnsExist) {
    TableData d;
    d.columns << "Name" << "Age";
    EXPECT_TRUE(d.isValid());
    EXPECT_EQ(d.columnCount(), 2);
}

TEST(TableDataTest, RowCount) {
    TableData d;
    d.columns << "A" << "B";
    d.rows.append(QStringList() << "x" << "y");
    d.rows.append(QStringList() << "1" << "2");
    d.rows.append(QStringList() << "p" << "q");
    EXPECT_EQ(d.rowCount(), 3);
    EXPECT_EQ(d.columnCount(), 2);
}

TEST(TableDataTest, EditableDefaultsEmpty) {
    TableData d;
    EXPECT_TRUE(d.editable.isEmpty());
}

TEST(TableDataTest, EditableExplicit) {
    TableData d;
    d.columns << "A" << "B" << "C";
    d.editable = {true, false, true};
    EXPECT_EQ(d.editable.size(), 3);
    EXPECT_TRUE(d.editable[0]);
    EXPECT_FALSE(d.editable[1]);
}

#ifdef USE_PROTO_DATA
TEST(TableDataTest, FromProtoRoundTrip) {
    table_data::TableData msg;
    msg.add_columns("Name");
    msg.add_columns("Age");
    msg.add_editable(true);
    msg.add_editable(false);
    auto *row = msg.add_rows();
    row->add_cells("Alice");
    row->add_cells("30");

    TableData d = TableData::fromProto(msg);
    EXPECT_TRUE(d.isValid());
    EXPECT_EQ(d.columnCount(), 2);
    EXPECT_EQ(d.rowCount(), 1);
    EXPECT_EQ(d.rows[0][0], "Alice");
    EXPECT_EQ(d.rows[0][1], "30");
    EXPECT_TRUE(d.editable[0]);
    EXPECT_FALSE(d.editable[1]);
}
#endif
