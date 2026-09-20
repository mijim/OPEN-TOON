import QtQuick
import QtQuick.Controls.Basic

TextField {
    id: control
    implicitHeight: 26
    implicitWidth: 100
    font.pixelSize: 11
    leftPadding: 6
    rightPadding: 6
    topPadding: 3
    bottomPadding: 3
    selectByMouse: true
    background: Rectangle {
        radius: 2
        color: "#111111"
        border.color: control.activeFocus ? "#aaaaaa" : "#303030"
    }
}
