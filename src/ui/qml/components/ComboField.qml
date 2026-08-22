import QtQuick
import QtQuick.Controls
import "../Theme.js" as Theme

// 下拉选择参数（对应设计稿 select，弹出列表自绘以贴合扁平风格）
Item {
    id: root
    property string label: ""
    property var model: []
    property int currentIndex: 0
    property int fieldWidth: 120
    readonly property string currentText: currentIndex >= 0 && currentIndex < model.length ? model[currentIndex] : ""
    signal activated(int index)

    height: 24
    implicitWidth: labelW.width + 6 + field.width

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
            id: field
            width: root.fieldWidth
            height: 22
            radius: 4
            color: popup.opened ? "#ffffff" : Theme.colors.inputBg
            border.color: popup.opened ? Theme.colors.headerBorder : Theme.colors.inputBorder
            border.width: 1
            anchors.verticalCenter: parent.verticalCenter
            Text {
                anchors.left: parent.left
                anchors.leftMargin: 8
                anchors.right: parent.right
                anchors.rightMargin: 16
                anchors.verticalCenter: parent.verticalCenter
                text: root.currentText
                color: Theme.colors.inputText
                font.pixelSize: 13
                elide: Text.ElideRight
            }
            Text {
                anchors.right: parent.right
                anchors.rightMargin: 6
                anchors.verticalCenter: parent.verticalCenter
                text: "▾"
                color: Theme.colors.unit
                font.pixelSize: 9
            }
            MouseArea {
                anchors.fill: parent
                onClicked: popup.open()
            }
        }
    }

    Popup {
        id: popup
        width: root.fieldWidth
        height: Math.min(listView.contentHeight + 8, 200)
        y: 24
        padding: 0
        closePolicy: Popup.CloseOnPressOutside | Popup.CloseOnEscape
        background: Rectangle {
            color: "#ffffff"
            border.color: Theme.colors.inputBorder
            border.width: 1
            radius: 4
        }
        contentItem: ListView {
            id: listView
            clip: true
            implicitHeight: contentHeight
            model: root.model
            currentIndex: root.currentIndex
            delegate: Rectangle {
                width: listView.width
                height: 26
                color: index === root.currentIndex
                       ? Qt.rgba(42 / 255, 111 / 255, 156 / 255, 0.12)
                       : (mouseArea.containsMouse ? Theme.colors.navBg : "transparent")
                Text {
                    anchors.left: parent.left
                    anchors.leftMargin: 8
                    anchors.verticalCenter: parent.verticalCenter
                    text: modelData
                    color: Theme.colors.inputText
                    font.pixelSize: 13
                }
                MouseArea {
                    id: mouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: {
                        root.currentIndex = index
                        root.activated(index)
                        popup.close()
                    }
                }
            }
        }
    }
}