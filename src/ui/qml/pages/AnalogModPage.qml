import QtQuick
import QtQuick.Layouts
import "../components"
import "../Theme.js" as Theme

// 模拟调制页（对应设计稿 tab-analog）
RowLayout {
    id: root
    Layout.fillWidth: true
    Layout.fillHeight: true
    spacing: 0

    property string modMode: "AM (调幅)"
    property string modDepth: "80.0"
    property string modFreq: "1.0"
    property string modWave: "正弦 (Sine)"
    property string pulseWidth: "10.0"
    property string pulsePeriod: "100.0"
    readonly property var modModes: ["AM (调幅)", "FM (调频)", "PM (调相)", "Pulse (脉冲)", "None (关闭)"]
    readonly property var modWaves: ["正弦 (Sine)", "方波 (Square)", "三角波 (Triangle)"]
    readonly property var extMods: ["内部", "外部"]

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
                        text: "模拟调制 (Analog Modulation)"
                        color: Theme.colors.title
                        font.pixelSize: 16
                        font.weight: Font.DemiBold
                    }
                    Text {
                        text: "— AM / FM / PM / Pulse 调制配置"
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
                            text: "Analog"
                            color: Theme.colors.badgeText
                            font.pixelSize: 10
                            font.weight: Font.Medium
                        }
                    }
                }
                Item { width: 1; height: 4 }
                Text {
                    text: "ⓘ  配置 AM、FM、PM 和脉冲调制 (Pulse Modulation) 参数。"
                    color: Theme.colors.desc
                    font.pixelSize: 13
                    font.italic: true
                    bottomPadding: 12
                }
                Rectangle { width: parent.width; height: 1; color: Theme.colors.navBg }
                Item { width: 1; height: 18 }

                SectionLabel { icon: "∿"; title: "调制类型" }
                Item { width: 1; height: 10 }
                Row {
                    spacing: 24
                    ComboField { id: modeCombo; label: "调制模式"; model: root.modModes; currentIndex: 0; fieldWidth: 140; onActivated: root.modMode = currentText }
                    TextParam { id: depthField; label: "调制深度 / 频偏"; value: root.modDepth; unit: "% / kHz"; fieldWidth: 70; onEditingFinished: root.modDepth = text }
                }
                Item { width: 1; height: 16 }

                SectionLabel { icon: "≡"; title: "调制参数" }
                Item { width: 1; height: 10 }
                Row {
                    spacing: 24
                    TextParam { id: freqField; label: "调制频率"; value: root.modFreq; unit: "kHz"; fieldWidth: 70; onEditingFinished: root.modFreq = text }
                    ComboField { label: "外部调制"; model: root.extMods; currentIndex: 0; fieldWidth: 100 }
                    ComboField { id: waveCombo; label: "调制波形"; model: root.modWaves; currentIndex: 0; fieldWidth: 120; onActivated: root.modWave = currentText }
                }
                Item { width: 1; height: 16 }

                SectionLabel { icon: "⚡"; title: "脉冲调制 (Pulse Modulation)" }
                Item { width: 1; height: 10 }
                Row {
                    spacing: 24
                    TextParam { id: pwField; label: "脉冲宽度"; value: root.pulseWidth; unit: "μs"; fieldWidth: 70; onEditingFinished: root.pulseWidth = text }
                    TextParam { id: ppField; label: "脉冲周期"; value: root.pulsePeriod; unit: "μs"; fieldWidth: 70; onEditingFinished: root.pulsePeriod = text }
                    ValueChip {
                        label: "占空比"
                        value: Math.round(parseFloat(root.pulseWidth) / parseFloat(root.pulsePeriod) * 100) + " %"
                    }
                }
                Item { width: 1; height: 20 }

                Row {
                    spacing: 8
                    ActionButton { text: "启用调制"; icon: "▶"; primary: true; onClicked: ToastService.show("已启用 " + root.modMode) }
                    ActionButton { text: "停止"; icon: "■"; onClicked: ToastService.show("已停止调制") }
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
                    title: "模拟调制状态"
                    icon: "ⓘ"
                    StatRow { label: "调制类型"; value: root.modMode }
                    StatRow { label: "调制深度"; value: root.modDepth + " %" }
                    StatRow { label: "调制频率"; value: root.modFreq + " kHz" }
                    StatRow { label: "调制波形"; value: root.modWave }
                    StatRow { label: "状态"; value: "● 已开启"; last: true }
                }
                InfoCard {
                    title: "脉冲参数"
                    icon: "↗"
                    StatRow { label: "脉冲宽度"; value: root.pulseWidth + " μs" }
                    StatRow { label: "脉冲周期"; value: root.pulsePeriod + " μs" }
                    StatRow {
                        label: "占空比"
                        value: Math.round(parseFloat(root.pulseWidth) / parseFloat(root.pulsePeriod) * 100) + " %"
                        last: true
                    }
                }
            }
        }
    }
}