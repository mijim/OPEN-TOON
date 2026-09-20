import QtQuick
import QtQuick.Controls.Basic

Button {
    id: control
    property bool active: false
    property string hint: ""
    implicitHeight: 30
    implicitWidth: Math.max(30, contentItem.implicitWidth + 18)
    padding: 7
    font.pixelSize: 12
    hoverEnabled: true
    contentItem: Text {
        text: control.text
        font: control.font
        color: control.enabled ? (control.active ? "#111111" : "#d5d5d5") : "#555555"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
    background: Rectangle {
        color: control.active ? "#f5f5f5" : control.down ? "#333333" : control.hovered ? "#252525" : "transparent"
        radius: 4
        border.color: control.activeFocus ? "#dddddd" : "transparent"
    }
    ToolTip.visible: hovered && hint.length > 0
    ToolTip.delay: 650
    ToolTip.text: hint
    Accessible.name: hint.length > 0 ? hint : text
}
