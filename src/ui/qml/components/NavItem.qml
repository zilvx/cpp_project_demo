import QtQuick
import "../Theme.js" as Theme

// 左侧导航项（垂直布局，激活项左侧高亮条）
Item {
    id: root
    property string text: ""
    property string icon: ""
    property bool active: false
    property bool hovered: false
    signal clicked()

    height: 44
    width: 150

    Rectangle {
        anchors.fill: parent
        color: root.active ? Theme.colors.navActiveBg
                           : (root.hovered ? Theme.colors.navHover : "transparent")
        Rectangle {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: 3
            color: Theme.colors.headerBorder
            visible: root.active
        }
    }
    Row {
        anchors.left: parent.left
        anchors.leftMargin: 16
        anchors.verticalCenter: parent.verticalCenter
        spacing: 8
        Text {
            id: iconTxt
            text: root.icon
            color: root.active ? Theme.colors.headerBorder : Theme.colors.navIcon
            font.pixelSize: 14
        }
        Text {
            id: textTxt
            text: root.text
            color: root.active ? Theme.colors.navActiveText : Theme.colors.navText
            font.pixelSize: 13
            font.weight: root.active ? Font.DemiBold : Font.Normal
        }
    }
    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        onEntered: root.hovered = true
        onExited: root.hovered = false
        onClicked: root.clicked()
    }
}