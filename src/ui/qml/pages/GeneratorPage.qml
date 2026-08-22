import QtQuick
import QtQuick.Layouts
import "../components"
import "../Theme.js" as Theme

// 信号发生器页（对应设计稿 tab-generator）
RowLayout {
    id: root
    Layout.fillWidth: true
    Layout.fillHeight: true
    spacing: 0

    property string frequency: "2.4000"
    property string power: "-10.0"
    property string phase: "0.0"
    property string refSource: "内部 (Internal)"
    property string refFreq: "10 MHz"
    property string impedance: "50 Ω"
    readonly property var refSources: ["内部 (Internal)", "外部 (External)"]
    readonly property var refFreqs: ["10 MHz", "100 MHz"]
    readonly property var impedances: ["50 Ω", "75 Ω"]

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
                        text: "信号发生器 (Generator)"
                        color: Theme.colors.title
                        font.pixelSize: 16
                        font.weight: Font.DemiBold
                    }
                    Text {
                        text: "— 配置射频载波与输出参数"
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
                            text: "Generator"
                            color: Theme.colors.badgeText
                            font.pixelSize: 10
                            font.weight: Font.Medium
                        }
                    }
                }
                Item { width: 1; height: 4 }
                Text {
                    text: "ⓘ  设置射频载波频率、功率、参考源和输出阻抗等核心发生器参数。"
                    color: Theme.colors.desc
                    font.pixelSize: 13
                    font.italic: true
                    bottomPadding: 12
                }
                Rectangle { width: parent.width; height: 1; color: Theme.colors.navBg }
                Item { width: 1; height: 18 }

                SectionLabel { icon: "≡"; title: "射频载波 (RF Carrier)" }
                Item { width: 1; height: 10 }
                Row {
                    spacing: 24
                    TextParam { id: freqField; label: "频率"; value: root.frequency; unit: "GHz"; fieldWidth: 90; onEditingFinished: root.frequency = text }
                    TextParam { id: powField; label: "功率"; value: root.power; unit: "dBm"; fieldWidth: 70; onEditingFinished: root.power = text }
                    TextParam { id: phaseField; label: "相位"; value: root.phase; unit: "°"; fieldWidth: 60; onEditingFinished: root.phase = text }
                }
                Item { width: 1; height: 8 }
                Row {
                    spacing: 24
                    TextParam { label: "频率偏移"; value: "0.0"; unit: "Hz"; fieldWidth: 70 }
                    TextParam { label: "功率偏移"; value: "0.0"; unit: "dB"; fieldWidth: 60 }
                    TextParam { label: "相位偏移"; value: "0.0"; unit: "°"; fieldWidth: 60 }
                }
                Item { width: 1; height: 16 }

                SectionLabel { icon: "◷"; title: "参考与同步" }
                Item { width: 1; height: 10 }
                Row {
                    spacing: 24
                    ComboField { id: refCombo; label: "参考源"; model: root.refSources; currentIndex: 0; fieldWidth: 140; onActivated: root.refSource = currentText }
                    ComboField { id: refFreqCombo; label: "参考频率"; model: root.refFreqs; currentIndex: 0; fieldWidth: 100; onActivated: root.refFreq = currentText }
                }
                Item { width: 1; height: 8 }
                Row {
                    spacing: 24
                    ComboField { id: impCombo; label: "输出阻抗"; model: root.impedances; currentIndex: 0; fieldWidth: 80; onActivated: root.impedance = currentText }
                    TextParam { label: "输出衰减"; value: "0.0"; unit: "dB"; fieldWidth: 60 }
                }
                Item { width: 1; height: 20 }

                Row {
                    spacing: 8
                    ActionButton { text: "应用 (Apply)"; icon: "▶"; primary: true; onClicked: ToastService.show("已应用发生器参数") }
                    ActionButton { text: "重置 (Reset)"; icon: "↺"; onClicked: ToastService.show("已重置发生器参数") }
                    ActionButton { text: "保存"; icon: "⤓"; onClicked: ToastService.show("已保存发生器配置") }
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
                    title: "发生器状态"
                    icon: "ⓘ"
                    StatRow { label: "载波频率"; value: root.frequency + " GHz" }
                    StatRow { label: "输出功率"; value: root.power + " dBm" }
                    StatRow { label: "参考源"; value: root.refSource + " " + root.refFreq }
                    StatRow { label: "输出阻抗"; value: root.impedance }
                    StatRow { label: "状态"; value: "● 已锁定"; last: true }
                }
                InfoCard {
                    title: "频谱参数"
                    icon: "↗"
                    StatRow { label: "谐波"; value: "-30 dBc" }
                    StatRow { label: "非谐波"; value: "-65 dBc" }
                    StatRow { label: "相位噪声"; value: "-110 dBc/Hz @ 10kHz"; last: true }
                }
            }
        }
    }
}