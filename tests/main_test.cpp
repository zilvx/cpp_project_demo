#include <QApplication>
#include <gtest/gtest.h>

int main(int argc, char **argv) {
    QApplication app(argc, argv);  // Widget 类需要 QApplication
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
