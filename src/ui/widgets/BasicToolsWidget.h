#ifndef BASIC_TOOLS_WIDGET_H
#define BASIC_TOOLS_WIDGET_H

#include <QList>
#include <QWidget>

class QLabel;
class QPushButton;
class QStackedWidget;

/**
 * @brief 基础工具子界面：左侧导航 + 堆叠内容区
 *
 * 容纳人员信息表、波形示波器、文件管理器，通过左侧导航切换。
 */
class BasicToolsWidget : public QWidget {
    Q_OBJECT

public:
    /**
     * @param tablePage 人员信息表页面（所有权转移）
     * @param wavePage  波形示波器页面（所有权转移）
     * @param filePage  文件管理器页面（所有权转移）
     */
    BasicToolsWidget(QWidget *tablePage, QWidget *wavePage, QWidget *filePage,
                     QWidget *parent = nullptr);
    ~BasicToolsWidget() override;

private:
    void loadStyleSheet();
    void setupUI(QWidget *tablePage, QWidget *wavePage, QWidget *filePage);
    QWidget *createNavPanel();
    void setCurrentIndex(int index);

    QStackedWidget *m_stack = nullptr;
    QList<QPushButton *> m_navBtns;
    QList<QLabel *> m_navBadges;
};

#endif // BASIC_TOOLS_WIDGET_H
