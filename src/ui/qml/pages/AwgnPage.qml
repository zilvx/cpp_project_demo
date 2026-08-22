import QtQuick
import QtQuick.Layouts
import "../components"
import "../Theme.js" as Theme

// AWGN 噪声页（对应设计稿 tab-awgn）
RowLayout {
    id: root
    Layout.fillWidth: true
    Layout.fillHeight: true
    spacing: 0

    property string noisePower: "-60.0"
    property string snr: "30.0"
    property string noiseBandwidth: "50.0"
    property string noiseType: "高斯白噪声 (AWGN)"
    property string noiseSeed: "12345"
    property string spectrumShape: "平坦 (Flat)"
    property string peakFactor: "3.0"
    readonly property var noiseTypes: ["高斯白噪声 (AWGN)", "带限噪声", "脉冲噪声"]
    readonly property var spectrumShapes: ["平坦 (Flat)", "高斯 (Gaussian)"]

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
                        text: "加性高斯白噪声 (AWGN)"
                        color: Theme.colors.title
                        font.pixelSize: 16
                        font.weight: Font.DemiBold
                    }
                    Text {
                        text: "— 配置噪声功率与频谱"
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
                            text: "AWGN"
                            color: Theme.colors.badgeText
                            font.pixelSize: 10
                            font.weight: Font.Medium
                        }
                    }
                }
                Item { width: 1; height: 4 }
                Text {
                    text: "ⓘ  在输出信号上叠加高斯白噪声，用于模拟真实信道环境。"
                    color: Theme.colors.desc
                    font.pixelSize: 13
                    font.italic: true
                    bottomPadding: 12
                }
                Rectangle { width: parent.width; height: 1; color: Theme.colors.navBg }
                Item { width: 1; height: 18 }

                SectionLabel { icon: "≡"; title: "噪声参数" }
                Item { width: 1; height: 10 }
                Row {
                    spacing: 24
                    TextParam { id: npField; label: "噪声功率"; value: root.noisePower; unit: "dBm"; fieldWidth: 70; onEditingFinished: root.noisePower = text }
                    TextParam { id: snrField; label: "信噪比 (SNR)"; value: root.snr; unit: "dB"; fieldWidth: 60; onEditingFinished: root.snr = text }
                    TextParam { id: nbField; label: "噪声带宽"; value: root.noiseBandwidth; unit: "MHz"; fieldWidth: 70; onEditingFinished: root.noiseBandwidth = text }
                }
                Item { width: 1; height: 16 }

                SectionLabel { icon: "⋔"; title: "噪声特性" }
                Item { width: 1; height: 10 }
                Row {
                    spacing: 24
                    ComboField { id: typeCombo; label: "噪声类型"; model: root.noiseTypes; currentIndex: 0; fieldWidth: 160; onActivated: root.noiseType = currentText }
                    TextParam { id: seedField; label: "噪声种子"; value: root.noiseSeed; fieldWidth: 80; numeric: true; onEditingFinished: root.noiseSeed = text }
                }
                Item { width: 1; height: 8 }
                Row {
                    spacing: 24
                    ComboField { id: shapeCombo; label: "频谱形状"; model: root.spectrumShapes; currentIndex: 0; fieldWidth: 130; onActivated: root.spectrumShape = currentText }
                    TextParam { id: peakField; label: "峰值因数"; value: root.peakFactor; unit: "dB"; fieldWidth: 60; onEditingFinished: root.peakFactor = text }
                }
                Item { width: 1; height: 20 }

                Row {
                    spacing: 8
                    ActionButton { text: "启用噪声"; icon: "＋"; primary: true; onClicked: ToastService.show("已启用 AWGN 噪声") }
                    ActionButton { text: "禁用"; icon: "✕"; onClicked: ToastService.show("已禁用噪声") }
                    ActionButton { text: "保存"; icon: "⤓"; onClicked: ToastService.show("已保存噪声配置") }
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
                    title: "AWGN 状态"
                    icon: "ⓘ"
                    StatRow { label: "噪声功率"; value: root.noisePower + " dBm" }
                    StatRow { label: "信噪比 (SNR)"; value: root.snr + " dB" }
                    StatRow { label: "噪声带宽"; value: root.noiseBandwidth + " MHz" }
                    StatRow { label: "峰值因数"; value: root.peakFactor + " dB" }
                    StatRow { label: "状态"; value: "● 已开启"; last: true }
                }
                InfoCard {
                    title: "噪声特性"
                    icon: "↗"
                    StatRow { label: "噪声类型"; value: root.noiseType.replace(" (AWGN)", "") }
                    StatRow { label: "频谱形状"; value: root.spectrumShape.replace(" (Flat)", "").replace(" (Gaussian)", "") }
                    StatRow { label: "噪声种子"; value: root.noiseSeed }
                    StatRow { label: "均方根 (RMS)"; value: "0.15 V"; last: true }
                }
            }
        }
    }
}