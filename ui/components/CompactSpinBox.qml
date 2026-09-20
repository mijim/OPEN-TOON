import QtQuick
import QtQuick.Controls.Basic

SpinBox {
    id: control
    implicitHeight: 26
    implicitWidth: 94
    leftPadding: 22
    rightPadding: 22
    topPadding: 3
    bottomPadding: 3
    font.pixelSize: 11
    background: Rectangle {
        radius: 2
        color: "#111111"
        border.color: control.activeFocus ? "#aaaaaa" : "#303030"
    }
    up.indicator: Rectangle {
        x: control.width - width
        width: 22
        height: control.height
        color: control.up.pressed ? "#333333" : "transparent"
        Text {
            anchors.centerIn: parent
            text: "+"
            font.pixelSize: 13
            color: control.up.enabled ? "#cccccc" : "#555555"
        }
    }
    down.indicator: Rectangle {
        width: 22
        height: control.height
        color: control.down.pressed ? "#333333" : "transparent"
        Text {
            anchors.centerIn: parent
            text: "−"
            font.pixelSize: 13
            color: control.down.enabled ? "#cccccc" : "#555555"
        }
    }
}
