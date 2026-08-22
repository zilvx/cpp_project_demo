import QtQuick

// 顶部开关按钮（对应设计稿 .rf-btn / .mod-btn）
Rectangle {
    id: root
    property string text: ""
    property bool on: true
    property color onColor: "#2ecc71"
    property color offColor: "#e74c3c"
    property bool pressed: false
    signal clicked()

    height: 24
    width: txt.width + 38
    radius: 4
    color: root.pressed ? Qt.darker(root.on ? root.onColor : root.offColor, 1.2)
                        : (root.on ? root.onColor : root.offColor)

    Row {
        anchors.centerIn: parent
        spacing: 6
        Text {
            text: "●"
            color: "#ffffff"
            font.pixelSize: 9
            anchors.verticalCenter: parent.verticalCenter
        }
        Text {
            id: txt
            text: root.text
            color: "#ffffff"
            font.pixelSize: 12
            font.weight: Font.DemiBold
        }
    }
    MouseArea {
        anchors.fill: parent
        onPressed: root.pressed = true
        onReleased: root.pressed = false
        onClicked: root.clicked()
    }
}