import QtQuick
import "../Theme.js" as Theme

// 信息卡（对应设计稿 .info-card，子元素为 StatRow）
Rectangle {
    id: card
    property string title: ""
    property string icon: "ⓘ"
    default property alias content: body.data

    color: Theme.colors.cardBg
    radius: 8
    border.color: Theme.colors.panelBorder
    border.width: 1
    width: parent ? parent.width : 320
    height: body.height + 28

    Column {
        id: body
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.topMargin: 14
        anchors.leftMargin: 14
        anchors.rightMargin: 14
        spacing: 0

        Row {
            spacing: 6
            Text {
                text: card.icon
                color: Theme.colors.cardTitle
                font.pixelSize: 11
                anchors.verticalCenter: parent.verticalCenter
            }
            Text {
                text: card.title
                color: Theme.colors.cardTitle
                font.pixelSize: 11
                font.weight: Font.DemiBold
                font.capitalization: Font.AllUppercase
            }
        }
        Item { width: 1; height: 10 }
    }
}