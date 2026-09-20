import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

Dialog {
    id: root
    required property var drawingCanvas
    required property var controller
    title: "Drawing selection"
    anchors.centerIn: parent
    width: 510
    modal: true
    standardButtons: Dialog.Close
    contentItem: ColumnLayout {
        spacing: 14
        Label {
            text: root.drawingCanvas.regionInfo
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
        Label {
            text: "Vectors remain editable. Raster pixels use integer translation and lossless flips/90° rotation. Overlapping pixels composite source over destination. Moves outside the raster canvas are rejected. Imported images are excluded."
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
            color: "#aaaaaa"
        }
        RowLayout {
            Label {
                text: "Horizontal · px"
            }
            SpinBox {
                id: dx
                from: -8192
                to: 8192
                value: 32
                editable: true
                Accessible.name: "Selection horizontal offset"
            }
            Label {
                text: "Vertical · px"
            }
            SpinBox {
                id: dy
                from: -8192
                to: 8192
                value: 0
                editable: true
                Accessible.name: "Selection vertical offset"
            }
        }
        RowLayout {
            Button {
                text: "Move"
                enabled: root.drawingCanvas.hasRegion
                onClicked: root.drawingCanvas.transformRegion(0, dx.value, dy.value)
            }
            Button {
                text: "Duplicate"
                enabled: root.drawingCanvas.hasRegion
                onClicked: root.drawingCanvas.transformRegion(1, dx.value, dy.value)
            }
            Button {
                text: "Delete"
                enabled: root.drawingCanvas.hasRegion
                onClicked: root.drawingCanvas.transformRegion(2)
            }
        }
        RowLayout {
            Button {
                text: "Flip horizontal"
                enabled: root.drawingCanvas.hasRegion
                onClicked: root.drawingCanvas.transformRegion(3)
            }
            Button {
                text: "Flip vertical"
                enabled: root.drawingCanvas.hasRegion
                onClicked: root.drawingCanvas.transformRegion(4)
            }
            Button {
                text: "Rotate 90°"
                enabled: root.drawingCanvas.hasRegion
                onClicked: root.drawingCanvas.transformRegion(5)
            }
            Button {
                text: "Deselect"
                onClicked: root.drawingCanvas.clearRegion()
            }
        }
        Label {
            text: root.controller.status
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
    }
}
