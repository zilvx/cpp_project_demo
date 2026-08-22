import QtQuick
import QtQuick.Layouts
import "../components"
import "../Theme.js" as Theme

// 文件管理页（对应设计稿 tab-file）
RowLayout {
    id: root
    Layout.fillWidth: true
    Layout.fillHeight: true
    spacing: 0

    property string fileName: "5G_NR_FR2_100MHz.stp"
    property string fileType: "配置文件 (.stp)"
    readonly property var types: ["配置文件 (.stp)", "波形文件 (.wfm)", "预设 (.prs)"]

    // ================= 左栏 =================
    Rectangle {
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.minimumWidth: 320
        color: Theme.colors.panelBg
        Rectangle {
            anchors.right: parent.right
            width: 1
            height: parent.height
            color: Theme.colors.panelBorder
        }
        Flickable {
            anchors.fill: parent
            clip: true
            contentWidth: width
            contentHeight: leftCol.height + 40
            Column {
                id: leftCol
                x: 24
                y: 20
                width: parent.width - 48
                spacing: 0

                Row {
                    spacing: 10
                    Text {
                        text: "文件管理"
                        color: Theme.colors.title
                        font.pixelSize: 16
                        font.weight: Font.DemiBold
                    }
                    Text {
                        text: "— 保存、加载和导出配置文件与波形"
                        color: Theme.colors.subtitle
                        font.pixelSize: 12
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Rectangle {
                        height: 18
                        width: badgeTxt.width + 20
                        radius: 12
                        color: Theme.colors.badgeBg
                        anchors.verticalCenter: parent.verticalCenter
                        Text {
                            id: badgeTxt
                            anchors.centerIn: parent
                            text: "File"
                            color: Theme.colors.badgeText
                            font.pixelSize: 10
                            font.weight: Font.Medium
                        }
                    }
                }
                Item { width: 1; height: 4 }
                Text {
                    text: "ⓘ  管理 Signal Studio 的配置文件 (.stp)、波形文件 (.wfm) 和预设参数。"
                    color: Theme.colors.desc
                    font.pixelSize: 13
                    font.italic: true
                    bottomPadding: 12
                }
                Rectangle { width: parent.width; height: 1; color: Theme.colors.navBg }
                Item { width: 1; height: 18 }

                SectionLabel { icon: "▣"; title: "当前文件" }
                Item { width: 1; height: 10 }
                Row {
                    spacing: 24
                    TextParam {
                        id: nameField
                        label: "文件名"
                        value: root.fileName
                        fieldWidth: 200
                        onEditingFinished: root.fileName = text
                    }
                    ComboField {
                        id: typeCombo
                        label: "类型"
                        model: root.types
                        currentIndex: 0
                        fieldWidth: 130
                        onActivated: root.fileType = currentText
                    }
                }
                Item { width: 1; height: 8 }
                TextParam {
                    id: pathField
                    label: "保存路径"
                    value: "C:/Users/Admin/Documents/Keysight/"
                    fieldWidth: 280
                }
                Item { width: 1; height: 16 }

                SectionLabel { icon: "☰"; title: "最近文件" }
                Item { width: 1; height: 10 }
                Column {
                    width: parent.width
                    spacing: 0
                    // 表头
                    Rectangle {
                        width: parent.width
                        height: 26
                        color: Theme.colors.chipBg
                        border.color: Theme.colors.panelBorder
                        border.width: 1
                        Row {
                            anchors.fill: parent
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                            Text { text: "名称"; width: parent.width - 180; color: Theme.colors.statLabel; font.pixelSize: 11; font.weight: Font.DemiBold; anchors.verticalCenter: parent.verticalCenter }
                            Text { text: "大小"; width: 100; color: Theme.colors.statLabel; font.pixelSize: 11; font.weight: Font.DemiBold; anchors.verticalCenter: parent.verticalCenter }
                            Text { text: "修改日期"; width: 80; color: Theme.colors.statLabel; font.pixelSize: 11; font.weight: Font.DemiBold; anchors.verticalCenter: parent.verticalCenter }
                        }
                    }
                    Repeater {
                        model: [
                            { n: "5G_NR_FR2_100MHz.stp", s: "24 KB", d: "2026-08-21" },
                            { n: "WLAN_11n_HT20.wfm", s: "1.2 MB", d: "2026-08-20" },
                            { n: "QAM64_50MHz.stp", s: "16 KB", d: "2026-08-19" },
                            { n: "LTE_10MHz_QPSK.wfm", s: "512 KB", d: "2026-08-18" },
                        ]
                        Rectangle {
                            width: parent.width
                            height: 26
                            color: rowArea.containsMouse ? Theme.colors.contentBg : "transparent"
                            border.color: "transparent"
                            Row {
                                anchors.fill: parent
                                anchors.leftMargin: 12
                                anchors.rightMargin: 12
                                Text {
                                    text: "▸  " + modelData.n
                                    width: parent.width - 180
                                    color: Theme.colors.title
                                    font.pixelSize: 12
                                    elide: Text.ElideRight
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                                Text { text: modelData.s; width: 100; color: Theme.colors.title; font.pixelSize: 12; anchors.verticalCenter: parent.verticalCenter }
                                Text { text: modelData.d; width: 80; color: Theme.colors.title; font.pixelSize: 12; anchors.verticalCenter: parent.verticalCenter }
                            }
                            MouseArea {
                                id: rowArea
                                anchors.fill: parent
                                hoverEnabled: true
                                onClicked: {
                                    root.fileName = modelData.n;
                                    ToastService.show("已加载 " + modelData.n);
                                }
                            }
                        }
                    }
                }
                Item { width: 1; height: 20 }

                Row {
                    spacing: 8
                    ActionButton { text: "保存 (Save)"; icon: "⤓"; primary: true; onClicked: ToastService.show("已保存 " + root.fileName) }
                    ActionButton { text: "加载 (Load)"; icon: "▣"; onClicked: ToastService.show("打开文件对话框…") }
                    ActionButton { text: "导出 (Export)"; icon: "⇗"; onClicked: ToastService.show("已导出 " + root.fileName) }
                    ActionButton { text: "导入 (Import)"; icon: "⇘"; onClicked: ToastService.show("打开导入对话框…") }
                    ActionButton { text: "删除"; icon: "✕"; onClicked: ToastService.show("删除功能未启用") }
                }
            }
        }
    }

    // ================= 右栏 =================
    Rectangle {
        Layout.preferredWidth: 380
        Layout.fillHeight: true
        color: Theme.colors.contentBg
        Flickable {
            anchors.fill: parent
            clip: true
            contentWidth: width
            contentHeight: rightCol.height + 32
            Column {
                id: rightCol
                x: 16
                y: 16
                width: parent.width - 32
                spacing: 14

                InfoCard {
                    title: "文件信息"
                    icon: "ⓘ"
                    StatRow { label: "文件名"; value: root.fileName }
                    StatRow { label: "大小"; value: "24 KB" }
                    StatRow { label: "创建日期"; value: "2026-08-21 14:30" }
                    StatRow { label: "修改日期"; value: "2026-08-21 15:20" }
                    StatRow { label: "包含波形"; value: "5G_NR_FR2_100MHz.wfm"; last: true }
                }
                InfoCard {
                    title: "快速操作"
                    icon: "⚡"
                    Row {
                        spacing: 6
                        ActionButton { text: "新建"; onClicked: ToastService.show("新建文件") }
                        ActionButton { text: "重命名"; onClicked: ToastService.show("重命名文件") }
                        ActionButton { text: "属性"; onClicked: ToastService.show("查看属性") }
                    }
                }
            }
        }
    }
}