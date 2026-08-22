import QtQuick
import "../Theme.js" as Theme

// 统计行（对应设计稿 .stat-row）
Item {
    id: root
    property string label: ""
    property string value: ""
    property bool last: false
    height: 26
    width: parent ? parent.width : 200

    Text {
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        text: root.label
        color: Theme.colors.statLabel
        font.pixelSize: 12
    }
    Text {
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        text: root.value
        color: Theme.colors.statValue
        font.pixelSize: 12
        font.weight: Font.Medium
    }
    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 1
        color: Theme.colors.divider
        visible: !root.last
    }
}