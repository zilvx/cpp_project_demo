import QtQuick
import QtQuick.Layouts
import "../components"
import "../Theme.js" as Theme

// 外部信号输入页（对应设计稿 tab-external）
RowLayout {
    id: root
    Layout.fillWidth: true
    Layout.fillHeight: true
    spacing: 0

    property string iqSource: "外部 I/Q"
    property string iqBandwidth: "100"
    property string triggerSource: "外部触发 1"
    property string triggerPolarity: "上升沿"
    property string triggerDelay: "0.0"
    readonly property var iqSources: ["外部 I/Q", "内部 I/Q"]
    readonly property var inputPorts: ["BNC I", "D-sub"]
    readonly property var impedances: ["50 Ω", "1 MΩ"]
    readonly property var triggerSources: ["外部触发 1", "外部触发 2", "GPIB"]
    readonly property var triggerPolarities: ["上升沿", "下降沿"]

    // ================= 左栏 =================
    Rectangle {
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.minimumWidth: 320
        color: Theme.colors.panelBg
        Rectangle { anchors.right: parent.right; width: 1; height: parent.height; color: Theme.colors.panelBorder }
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
                        text: "外部信号输入 (External)"
                        color: Theme.colors.title
                        font.pixelSize: 16
                        font.weight: Font.DemiBold
                    }
                    Text {
                        text: "— 配置外部 I/Q 或触发信号"
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
                            text: "External"
                            color: Theme.colors.badgeText
                            font.pixelSize: 10
                            font.weight: Font.Medium
                        }
                    }
                }
                Item { width: 1; height: 4 }
                Text {
                    text: "ⓘ  配置外部 I/Q 输入、外部触发和同步信号。"
                    color: Theme.colors.desc
                    font.pixelSize: 13
                    font.italic: true
                    bottomPadding: 12
                }
                Rectangle { width: parent.width; height: 1; color: Theme.colors.navBg }
                Item { width: 1; height: 18 }

                SectionLabel { icon: "∿"; title: "I/Q 输入" }
                Item { width: 1; height: 10 }
                Row {
                    spacing: 24
                    ComboField { id: iqCombo; label: "I/Q 源"; model: root.iqSources; currentIndex: 0; fieldWidth: 120; onActivated: root.iqSource = currentText }
                    TextParam { id: bwField; label: "I/Q 带宽"; value: root.iqBandwidth; unit: "MHz"; fieldWidth: 70; onEditingFinished: root.iqBandwidth = text }
                }
                Item { width: 1; height: 8 }
                Row {
                    spacing: 24
                    ComboField { label: "I 输入"; model: root.inputPorts; currentIndex: 0; fieldWidth: 100 }
                    ComboField { label: "Q 输入"; model: root.inputPorts; currentIndex: 1; fieldWidth: 100 }
                    ComboField { label: "阻抗"; model: root.impedances; currentIndex: 0; fieldWidth: 80 }
                }
                Item { width: 1; height: 16 }

                SectionLabel { icon: "⚡"; title: "外部触发" }
                Item { width: 1; height: 10 }
                Row {
                    spacing: 24
                    ComboField { id: trigCombo; label: "触发源"; model: root.triggerSources; currentIndex: 0; fieldWidth: 120; onActivated: root.triggerSource = currentText }
                    ComboField { id: polCombo; label: "触发极性"; model: root.triggerPolarities; currentIndex: 0; fieldWidth: 100; onActivated: root.triggerPolarity = currentText }
                    TextParam { id: delayField; label: "触发延迟"; value: root.triggerDelay; unit: "μs"; fieldWidth: 60; onEditingFinished: root.triggerDelay = text }
                }
                Item { width: 1; height: 20 }

                Row {
                    spacing: 8
                    ActionButton { text: "启用外部 I/Q"; icon: "✓"; primary: true; onClicked: ToastService.show("已启用外部 I/Q 输入") }
                    ActionButton { text: "校准"; icon: "⟳"; onClicked: ToastService.show("校准中…") }
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
                    title: "外部连接状态"
                    icon: "ⓘ"
                    StatRow { label: "I/Q 源"; value: root.iqSource }
                    StatRow { label: "I/Q 带宽"; value: root.iqBandwidth + " MHz" }
                    StatRow { label: "触发源"; value: root.triggerSource }
                    StatRow { label: "触发极性"; value: root.triggerPolarity }
                    StatRow { label: "状态"; value: "● 已连接"; last: true }
                }
                InfoCard {
                    title: "端口映射"
                    icon: "🔌"
                    StatRow { label: "I 输入"; value: "BNC I" }
                    StatRow { label: "Q 输入"; value: "BNC Q" }
                    StatRow { label: "触发输入"; value: "BNC Trig" }
                    StatRow { label: "同步时钟"; value: "BNC Sync"; last: true }
                }
            }
        }
    }
}