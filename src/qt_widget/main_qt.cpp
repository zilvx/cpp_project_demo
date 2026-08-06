#include <QApplication>
#include <QVBoxLayout>
#include <QWidget>
#include "TableWidget.h"
#include "VirtualKeyboard.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    auto *window = new QWidget;
    window->setWindowTitle(QStringLiteral("Qt Demo — 表格 + 虚拟键盘"));
    window->resize(1050, 720);

    auto *layout = new QVBoxLayout(window);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // 表格（上方）
    auto *table = new TableWidget(20, 6, window);
    layout->addWidget(table);

    // 98 键虚拟键盘（下方，初始隐藏，点击可编辑单元格时弹出）
    auto *keyboard = new VirtualKeyboard(window);
    keyboard->setVisible(false);
    layout->addWidget(keyboard);

    // 点击可编辑单元格 → 弹出键盘
    QObject::connect(table, &TableWidget::cellEditingStarted,
                     keyboard, [keyboard](int, int) {
                         keyboard->setVisible(true);
                     });

    // 编辑结束 → 隐藏键盘
    QObject::connect(table, &TableWidget::cellEditingFinished,
                     keyboard, [keyboard]() {
                         keyboard->setVisible(false);
                     });

    // 键盘输入 → 更新单元格内容
    QObject::connect(keyboard, &VirtualKeyboard::keyPressed,
                     table, &TableWidget::handleKeyInput);

    window->show();
    return app.exec();
}
