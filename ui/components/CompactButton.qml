import QtQuick
import QtQuick.Controls.Basic

Button {
    id: control
    implicitHeight: 26
    implicitWidth: Math.max(26, contentItem.implicitWidth + 16)
    leftPadding: 8
    rightPadding: 8
    topPadding: 3
    bottomPadding: 3
    font.pixelSize: 11
    hoverEnabled: true
    contentItem: Text {
        text: control.text
        font: control.font
        color: control.enabled ? "#dddddd" : "#606060"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
    background: Rectangle {
        radius: 3
        color: control.down ? "#303030" : control.hovered ? "#242424" : "#171717"
        border.color: control.activeFocus ? "#999999" : "#2a2a2a"
    }
}
