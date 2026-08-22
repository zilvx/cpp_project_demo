import QtQuick
import QtQuick.Layouts
import "../components"
import "../constellation.js" as Cst
import "../Theme.js" as Theme

// 数字调制页（对应设计稿 tab-digital）
RowLayout {
    id: root
    Layout.fillWidth: true
    Layout.fillHeight: true
    spacing: 0

    // ---- 状态 ----
    property string formatText: "QAM 64"
    property string symbolRate: "50.0"
    property string oversampling: "4"
    property string filterType: "升余弦 (Raised Cosine)"
    property string rolloff: "0.35"
    property string dataSource: "PN 序列"
    property string patternLen: "4096"
    property double evm: 0.8
    property bool previewing: false
    readonly property var formats: ["QPSK", "QAM 64", "QAM 256", "16QAM", "FSK", "ASK"]
    readonly property var filters: ["升余弦 (Raised Cosine)", "根升余弦 (Root Nyquist)", "高斯 (Gaussian)"]
    readonly property var dataSources: ["PN 序列", "全0", "全1", "自定义文件"]

    Component.onCompleted: recompute()

    function recompute() {
        var pts = Cst.pointsForFormat(root.formatText);
        pts = Cst.applyEVM(pts, root.evm);
        constellation.points = pts;
        constellation.pointRadius = Cst.pointRadius(pts.length);
    }

    function calculate() {
        root.evm = Math.round((0.5 + Math.random() * 1.0) * 10) / 10;
        recompute();
        ToastService.show("已计算 EVM(RMS) = " + root.evm + " %");
    }

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
            id: flick
            anchors.fill: parent
            clip: true
            contentWidth: width
            contentHeight: leftCol.height + 40
            Column {
                id: leftCol
                x: 24
                y: 20
                width: flick.width - 48
                spacing: 0

                // 标题
                Row {
                    spacing: 10
                    Text {
                        text: "数字调制 (Digital Modulation)"
                        color: Theme.colors.title
                        font.pixelSize: 16
                        font.weight: Font.DemiBold
                    }
                    Text {
                        text: "— QPSK / QAM / FSK 等调制配置"
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
                            text: "Digital"
                            color: Theme.colors.badgeText
                            font.pixelSize: 10
                            font.weight: Font.Medium
                        }
                    }
                }
                Item { width: 1; height: 4 }
                Text {
                    text: "ⓘ  设置数字调制格式、符号率、滤波器等参数。"
                    color: Theme.colors.desc
                    font.pixelSize: 13
                    font.italic: true
                    bottomPadding: 12
                }
                Rectangle {
                    width: parent.width
                    height: 1
                    color: Theme.colors.navBg
                }
                Item { width: 1; height: 18 }

                // 基本参数
                SectionLabel { icon: "⚙"; title: "基本参数" }
                Item { width: 1; height: 10 }
                Row {
                    spacing: 24
                    ComboField {
                        id: formatCombo
                        label: "调制格式"
                        model: root.formats
                        currentIndex: 1
                        fieldWidth: 120
                        onActivated: {
                            root.formatText = currentText;
                            recompute();
                        }
                    }
                    TextParam {
                        id: rateField
                        label: "符号率"
                        value: root.symbolRate
                        unit: "MHz"
                        fieldWidth: 70
                        onEditingFinished: root.symbolRate = text
                    }
                    TextParam {
                        id: overField
                        label: "过采样"
                        value: root.oversampling
                        unit: "x"
                        fieldWidth: 50
                        numeric: true
                        onEditingFinished: root.oversampling = text
                    }
                }
                Item { width: 1; height: 16 }

                // 滤波器设置
                SectionLabel { icon: "⋔"; title: "滤波器设置" }
                Item { width: 1; height: 10 }
                Row {
                    spacing: 24
                    ComboField {
                        id: filterCombo
                        label: "滤波器类型"
                        model: root.filters
                        currentIndex: 0
                        fieldWidth: 190
                        onActivated: root.filterType = currentText
                    }
                    TextParam {
                        id: rolloffField
                        label: "滚降系数"
                        value: root.rolloff
                        fieldWidth: 50
                        onEditingFinished: root.rolloff = text
                    }
                }
                Item { width: 1; height: 16 }

                // 数据源
                SectionLabel { icon: "▤"; title: "数据源" }
                Item { width: 1; height: 10 }
                Row {
                    spacing: 24
                    ComboField {
                        id: sourceCombo
                        label: "数据源"
                        model: root.dataSources
                        currentIndex: 0
                        fieldWidth: 130
                        onActivated: root.dataSource = currentText
                    }
                    TextParam {
                        id: patternField
                        label: "Pattern 长度"
                        value: root.patternLen
                        unit: "符号"
                        fieldWidth: 70
                        numeric: true
                        onEditingFinished: root.patternLen = text
                    }
                }
                Item { width: 1; height: 20 }

                // 操作按钮
                Row {
                    spacing: 8
                    ActionButton {
                        text: "计算 (Calculate)"
                        icon: "="
                        primary: true
                        onClicked: root.calculate()
                    }
                    ActionButton {
                        text: root.previewing ? "停止预览" : "预览"
                        icon: "▶"
                        onClicked: {
                            root.previewing = !root.previewing;
                            ToastService.show(root.previewing ? "开始预览调制波形" : "已停止预览");
                        }
                    }
                    ActionButton {
                        text: "保存"
                        icon: "⤓"
                        onClicked: ToastService.show("已保存配置文件 QAM64_50MHz.stp")
                    }
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

                // I/Q 星座图
                InfoCard {
                    title: "I/Q 星座图"
                    icon: "◎"
                    Constellation {
                        id: constellation
                        width: rightCol.width
                        height: 150
                        label: root.formatText
                        rateText: root.symbolRate + " MHz"
                        previewing: root.previewing
                    }
                }

                // 数字调制状态
                InfoCard {
                    title: "数字调制状态"
                    icon: "ⓘ"
                    StatRow { label: "调制格式"; value: root.formatText }
                    StatRow { label: "符号率"; value: root.symbolRate + " MHz" }
                    StatRow { label: "滤波器"; value: root.filterType + " (α=" + root.rolloff + ")" }
                    StatRow { label: "EVM (RMS)"; value: root.evm + " %"; last: true }
                }
            }
        }
    }
}