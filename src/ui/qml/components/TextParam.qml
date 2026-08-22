import QtQuick
import "../Theme.js" as Theme

// 文本输入参数（对应设计稿 .param-item label + input + unit）
Item {
    id: root
    property string label: ""
    property string value: ""
    property string unit: ""
    property int fieldWidth: 80
    property bool numeric: false
    readonly property string text: field.text
    signal editingFinished(string text)

    height: 24
    implicitWidth: labelW.width + 6 + field.width + (root.unit === "" ? 0 : 6 + unitW.width)

    Component.onCompleted: field.text = root.value
    onValueChanged: if (field.text !== root.value) field.text = root.value

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
            width: root.fieldWidth
            height: 22
            radius: 4
            color: field.activeFocus ? "#ffffff" : Theme.colors.inputBg
            border.color: field.activeFocus ? Theme.colors.headerBorder : Theme.colors.inputBorder
            border.width: 1
            anchors.verticalCenter: parent.verticalCenter
            TextInput {
                id: field
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 4
                verticalAlignment: TextInput.AlignVCenter
                font.pixelSize: 13
                color: Theme.colors.inputText
                selectByMouse: true
                validator: root.numeric ? doubleValidator : null
                onEditingFinished: root.editingFinished(text)
            }
        }
        Text {
            id: unitW
            text: root.unit
            color: Theme.colors.unit
            font.pixelSize: 12
            anchors.verticalCenter: parent.verticalCenter
        }
    }
    DoubleValidator {
        id: doubleValidator
        bottom: 0
        top: 999999
        decimals: 6
        notation: DoubleValidator.StandardNotation
    }
}