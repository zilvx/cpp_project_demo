import QtQuick
import QtQuick.Layouts
import "components"
import "pages"
import "Theme.js" as Theme

// 数字调制界面（对应设计稿 vsg_digital modulation.html 完整界面）
// 以 Item 为根，可嵌入 QQuickWidget（main_ui.cpp 的"数字调制"标签页）
Item {
    id: root
    property int currentTab: 3
    property bool modOn: true
    property bool rfOn: true

    Rectangle {
        anchors.fill: parent
        color: Theme.colors.pageBg

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            // ===== 主体区：左侧导航栏 + 内容区 =====
            // （顶部标题栏由 main_ui 全局 AppHeaderBar 提供）
            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 0

                // 左侧导航栏
                Rectangle {
                    Layout.fillHeight: true
                    Layout.preferredWidth: 150
                    color: Theme.colors.navBg
                    Rectangle {
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        anchors.right: parent.right
                        width: 1
                        color: Theme.colors.navBorder
                    }
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.topMargin: 10
                        spacing: 2

                        NavItem { icon: "🗀"; text: "File"; active: root.currentTab === 0; onClicked: root.currentTab = 0 }
                        NavItem { icon: "⚙"; text: "Generator"; active: root.currentTab === 1; onClicked: root.currentTab = 1 }
                        NavItem { icon: "∿"; text: "Analog"; active: root.currentTab === 2; onClicked: root.currentTab = 2 }
                        NavItem { icon: "⟨⟩"; text: "Digital Mod"; active: root.currentTab === 3; onClicked: root.currentTab = 3 }
                        NavItem { icon: "⏚"; text: "External"; active: root.currentTab === 4; onClicked: root.currentTab = 4 }
                        NavItem { icon: "☁"; text: "AWGN"; active: root.currentTab === 5; onClicked: root.currentTab = 5 }

                        Item { Layout.fillHeight: true }

                        // 底部开关区
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 1
                            color: Theme.colors.navBorder
                            Layout.leftMargin: 12
                            Layout.rightMargin: 12
                        }
                        Item { Layout.preferredHeight: 10 }
                        Row {
                            Layout.leftMargin: 16
                            spacing: 4
                            Text { text: "调制"; color: Theme.colors.label; font.pixelSize: 11; anchors.verticalCenter: parent.verticalCenter }
                            ToggleBtn {
                                text: root.modOn ? "开" : "关"
                                on: root.modOn
                                onColor: Theme.colors.modOn
                                offColor: Theme.colors.modOff
                                onClicked: root.modOn = !root.modOn
                            }
                        }
                        Item { Layout.preferredHeight: 8 }
                        Row {
                            Layout.leftMargin: 16
                            spacing: 4
                            Text { text: "RF"; color: Theme.colors.label; font.pixelSize: 11; anchors.verticalCenter: parent.verticalCenter }
                            ToggleBtn {
                                text: root.rfOn ? "开" : "关"
                                on: root.rfOn
                                onColor: Theme.colors.rfOn
                                offColor: Theme.colors.rfOff
                                onClicked: root.rfOn = !root.rfOn
                            }
                        }
                        Item { Layout.preferredHeight: 14 }
                    }
                }

                // 内容区（6 个标签页）
                StackLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    currentIndex: root.currentTab

                    FilePage {}
                    GeneratorPage {}
                    AnalogModPage {}
                    DigitalModPage {}
                    ExternalPage {}
                    AwgnPage {}
                }
            }

            // ===== 底部状态栏 =====
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 28
                color: Theme.colors.footerBg
                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    height: 1
                    color: Theme.colors.navBorder
                }
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 24
                    anchors.rightMargin: 24
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 20
                        Row {
                            spacing: 6
                            Rectangle { width: 6; height: 6; radius: 3; color: Theme.colors.dotGreen; anchors.verticalCenter: parent.verticalCenter }
                            Text { text: root.rfOn && root.modOn ? "就绪" : "待机"; color: Theme.colors.footerText; font.pixelSize: 11 }
                        }
                        Text { text: "|"; color: Theme.colors.footerText; font.pixelSize: 11 }
                        Text { text: "波形: QAM64_50MHz"; color: Theme.colors.footerText; font.pixelSize: 11 }
                        Text { text: "|"; color: Theme.colors.footerText; font.pixelSize: 11 }
                        Text { text: "内存: 5% 已用"; color: Theme.colors.footerText; font.pixelSize: 11 }
                    }
                    RowLayout {
                        spacing: 16
                        Text { text: "AP5041A G3"; color: Theme.colors.footerText; font.pixelSize: 11 }
                        Text { text: "固件 v3.2.1"; color: Theme.colors.footerText; font.pixelSize: 11 }
                        Text { text: "2026-08-21 16:30"; color: Theme.colors.footerText; font.pixelSize: 11 }
                    }
                }
            }
        }
    }

    // ===== 全局轻提示 Toast（由 ToastService.show 触发） =====
    Rectangle {
        id: toastBox
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 46
        width: toastText.width + 32
        height: 30
        radius: 15
        color: Qt.rgba(13 / 255, 27 / 255, 42 / 255, 0.9)
        opacity: 0
        visible: false
        Text {
            id: toastText
            anchors.centerIn: parent
            color: "#ffffff"
            font.pixelSize: 12
        }
        Behavior on opacity { NumberAnimation { duration: 150 } }
    }
    Timer { id: toastTimer; interval: 2000; onTriggered: toastBox.opacity = 0 }

    Connections {
        target: ToastService
        function onShowRequested(msg) {
            toastText.text = msg;
            toastBox.visible = true;
            toastBox.opacity = 1;
            toastTimer.restart();
        }
    }
}