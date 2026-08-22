import QtQuick
import "../Theme.js" as Theme

// 分区标题（对应设计稿 .section-label）
Row {
    id: root
    property string icon: "◈"
    property string title: ""
    spacing: 8

    Text {
        text: root.icon
        color: Theme.colors.headerBorder
        font.pixelSize: 12
        anchors.verticalCenter: parent.verticalCenter
    }
    Text {
        text: root.title
        color: Theme.colors.sectionLabel
        font.pixelSize: 12
        font.weight: Font.DemiBold
    }
}