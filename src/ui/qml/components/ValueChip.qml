import QtQuick
import "../Theme.js" as Theme

// 只读值显示（对应设计稿 .value-display）
Item {
    id: root
    property string label: ""
    property string value: ""
    height: 24
    implicitWidth: labelW.width + 6 + chip.width

    Row {
        anchors.fill: parent
        spacing: 6
        Text {
            id: labelW
            text: root.label
            color: Theme.colors.label
            font.pixelSize: 12
            anchors.verticalCenter: parent.verticalCenter
        }
        Rectangle {
            id: chip
            width: Math.max(50, valueW.width + 24)
            height: 22
            radius: 4
            color: Theme.colors.chipBg
            anchors.verticalCenter: parent.verticalCenter
            Text {
                id: valueW
                anchors.centerIn: parent
                text: root.value
                color: Theme.colors.inputText
                font.pixelSize: 13
                font.weight: Font.Medium
            }
        }
    }
}