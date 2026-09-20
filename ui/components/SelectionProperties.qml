import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ColumnLayout {
    id: root
    required property var drawingCanvas
    required property var controller
    property var objectData: drawingCanvas.objectProperties
    spacing: 10
    Label {
        text: root.objectData.kind === "vector" ? "Vector object" : root.objectData.kind === "raster" ? "Raster selection" : "Drawing selection · " + (root.objectData.count || 0) + " vectors"
        font.bold: true
    }
    Label {
        text: "Drag corners to scale. Drag the circle to rotate. Hold Shift after starting a drag to constrain proportions or snap rotation."
        wrapMode: Text.WordWrap
        Layout.fillWidth: true
        color: "#999999"
    }
    GridLayout {
        columns: 2
        Layout.fillWidth: true
        Repeater {
            model: [
                {
                    key: "x",
                    name: "X · px"
                },
                {
                    key: "y",
                    name: "Y · px"
                },
                {
                    key: "width",
                    name: "Width · px"
                },
                {
                    key: "height",
                    name: "Height · px"
                },
                {
                    key: "rotation",
                    name: "Rotate by · °"
                }
            ]
            ColumnLayout {
                required property var modelData
                Layout.fillWidth: true
                Label {
                    text: modelData.name
                    color: "#999999"
                    font.pixelSize: 10
                }
                PropertyNumber {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 90
                    number: root.objectData[modelData.key] || 0
                    label: modelData.name
                    enabled: !root.objectData.locked
                    onCommitted: value => root.drawingCanvas.setObjectProperty(modelData.key, value)
                }
            }
        }
    }
    ColumnLayout {
        visible: root.objectData.kind === "vector"
        Layout.fillWidth: true
        Label {
            text: "Stroke width"
            color: "#999999"
        }
        PropertyNumber {
            Layout.fillWidth: true
            number: root.objectData.strokeWidth || 1
            label: "Selected stroke width"
            onCommitted: value => root.drawingCanvas.setObjectProperty("strokeWidth", value)
        }
        CompactCheckBox {
            text: "Filled"
            checked: root.objectData.filled || false
            onToggled: root.drawingCanvas.setObjectProperty("filled", checked ? 1 : 0)
        }
        CompactComboBox {
            Layout.fillWidth: true
            model: ["Underlay Art", "Color Art", "Line Art", "Overlay Art"]
            currentIndex: root.objectData.artLayer || 0
            onActivated: root.drawingCanvas.setObjectProperty("artLayer", currentIndex)
            Accessible.name: "Selected object art layer"
        }
        CompactComboBox {
            Layout.fillWidth: true
            model: root.controller.palette
            textRole: "name"
            valueRole: "id"
            currentIndex: model.findIndex(s => s.id === root.objectData.swatch)
            onActivated: root.drawingCanvas.setObjectProperty("swatch", currentValue)
            Accessible.name: "Selected object color"
        }
    }
    VectorTools {
        Layout.fillWidth: true
        drawingCanvas: root.drawingCanvas
        controller: root.controller
        enabled: !root.objectData.locked
    }
    Label {
        visible: root.objectData.locked || false
        text: "Layer locked"
        color: "#bbbbbb"
    }
}
