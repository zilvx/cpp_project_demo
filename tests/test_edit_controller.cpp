#include <gtest/gtest.h>
#include <QApplication>
#include <QTableWidget>
#include <QSignalSpy>

#include "EditController.h"

class EditControllerTest : public ::testing::Test {
protected:
    void SetUp() override {
        table = new QTableWidget(3, 4);
        table->setHorizontalHeaderLabels({"A", "B", "C", "D"});
        // C 列只读
        for (int r = 0; r < 3; ++r) {
            for (int c = 0; c < 4; ++c) {
                auto *item = new QTableWidgetItem(
                    QString("R%1C%2").arg(r).arg(c));
                if (c == 2) item->setFlags(item->flags() & ~Qt::ItemIsEditable);
                else        item->setFlags(item->flags() | Qt::ItemIsEditable);
                table->setItem(r, c, item);
            }
        }
        ctrl = new EditController(table);
    }
    void TearDown() override { delete table; }

    QTableWidget *table = nullptr;
    EditController *ctrl = nullptr;
};

TEST_F(EditControllerTest, InitiallyNotEditing) {
    EXPECT_FALSE(ctrl->isEditing());
    EXPECT_EQ(ctrl->editingRow(), -1);
    EXPECT_EQ(ctrl->editingCol(), -1);
}

TEST_F(EditControllerTest, ClickEditableCellStartsEditing) {
    QSignalSpy spy(ctrl, &EditController::editingStarted);
    ctrl->onCellClicked(0, 0);  // Column A is editable
    EXPECT_TRUE(ctrl->isEditing());
    EXPECT_EQ(spy.count(), 1);
}

TEST_F(EditControllerTest, ClickReadOnlyCellDoesNotStartEditing) {
    QSignalSpy spy(ctrl, &EditController::editingStarted);
    ctrl->onCellClicked(0, 2);  // Column C is read-only
    EXPECT_FALSE(ctrl->isEditing());
    EXPECT_EQ(spy.count(), 0);
}

TEST_F(EditControllerTest, SingleCharAppended) {
    ctrl->onCellClicked(0, 0);
    ctrl->handleKeyInput("X");
    EXPECT_EQ(table->item(0, 0)->text(), "R0C0X");
}

TEST_F(EditControllerTest, BackspaceRemovesLastChar) {
    ctrl->onCellClicked(0, 0);
    ctrl->handleKeyInput("Backspace");
    EXPECT_EQ(table->item(0, 0)->text(), "R0C");
}

TEST_F(EditControllerTest, EnterCommitsEdit) {
    QSignalSpy spy(ctrl, &EditController::editingFinished);
    ctrl->onCellClicked(0, 0);
    ctrl->handleKeyInput("Z");
    ctrl->handleKeyInput("Enter");
    EXPECT_FALSE(ctrl->isEditing());
    EXPECT_EQ(spy.count(), 1);
    EXPECT_EQ(table->item(0, 0)->text(), "R0C0Z");
}

TEST_F(EditControllerTest, EscapeCancelsEdit) {
    ctrl->onCellClicked(0, 0);
    QString original = table->item(0, 0)->text();
    ctrl->handleKeyInput("X");
    ctrl->handleKeyInput("Y");
    QSignalSpy spy(ctrl, &EditController::editingFinished);
    ctrl->handleKeyInput("Escape");
    EXPECT_FALSE(ctrl->isEditing());
    EXPECT_EQ(spy.count(), 1);
    EXPECT_EQ(table->item(0, 0)->text(), original);
}

TEST_F(EditControllerTest, EmptyTextRestoredOnCommit) {
    ctrl->onCellClicked(0, 0);
    table->item(0, 0)->setText("");  // clear the cell
    ctrl->handleKeyInput("Enter");
    EXPECT_EQ(table->item(0, 0)->text(), "R0C0");  // original restored
}
