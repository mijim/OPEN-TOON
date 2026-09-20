import QtQuick
import QtQuick.Controls.Basic

CheckBox {
    id: control
    implicitHeight: 24
    implicitWidth: contentItem.implicitWidth + 22
    padding: 2
    spacing: 6
    font.pixelSize: 11
    indicator: Rectangle {
        x: 2
        y: (control.height - height) / 2
        width: 14
        height: 14
        radius: 2
        color: control.checked ? "#dddddd" : "#111111"
        border.color: control.activeFocus ? "#ffffff" : "#666666"
        Text {
            anchors.centerIn: parent
            text: control.checked ? "✓" : ""
            font.pixelSize: 11
            color: "#111111"
        }
    }
    contentItem: Text {
        text: control.text
        font: control.font
        leftPadding: 20
        verticalAlignment: Text.AlignVCenter
        color: control.enabled ? "#dddddd" : "#777777"
    }
}
