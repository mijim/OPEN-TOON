import QtQuick
import QtQuick.Controls.Basic

ComboBox {
    id: control
    implicitHeight: 26
    implicitWidth: 140
    leftPadding: 8
    rightPadding: 23
    topPadding: 3
    bottomPadding: 3
    font.pixelSize: 11
    contentItem: Text {
        text: control.displayText
        font: control.font
        color: control.enabled ? "#dddddd" : "#666666"
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
    indicator: Text {
        x: control.width - width - 8
        y: (control.height - height) / 2
        text: "⌄"
        color: "#999999"
        font.pixelSize: 12
    }
    background: Rectangle {
        radius: 3
        color: "#151515"
        border.color: control.activeFocus ? "#999999" : "#303030"
    }
    delegate: ItemDelegate {
        required property int index
        width: control.width
        height: 26
        text: control.textAt(index)
        font.pixelSize: 11
        highlighted: control.highlightedIndex === index
        padding: 6
    }
}
