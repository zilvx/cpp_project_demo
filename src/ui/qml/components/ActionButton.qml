import QtQuick
import "../Theme.js" as Theme

// 操作按钮（对应设计稿 .action-btns button / .primary）
Rectangle {
    id: root
    property string text: ""
    property string icon: ""
    property bool primary: false
    property bool pressed: false
    signal clicked()

    height: 28
    width: txt.width + (root.icon === "" ? 24 : 30) + (root.icon === "" ? 0 : iconTxt.width)
    radius: 4
    color: root.pressed ? (root.primary ? Theme.colors.primaryDark : Theme.colors.navBg)
                        : (root.primary ? Theme.colors.primary : "#ffffff")
    border.color: root.primary ? Theme.colors.primary : Theme.colors.inputBorder
    border.width: 1

    Row {
        anchors.centerIn: parent
        spacing: 6
        Text {
            id: iconTxt
            text: root.icon
            color: root.primary ? "#ffffff" : Theme.colors.navText
            font.pixelSize: 12
            anchors.verticalCenter: parent.verticalCenter
        }
        Text {
            id: txt
            text: root.text
            color: root.primary ? "#ffffff" : Theme.colors.navText
            font.pixelSize: 12
            font.weight: Font.Medium
        }
    }
    MouseArea {
        anchors.fill: parent
        onPressed: root.pressed = true
        onReleased: root.pressed = false
        onClicked: root.clicked()
    }
}