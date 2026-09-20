import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ColumnLayout {
    id: root
    required property var drawingCanvas
    required property var controller
    property var selectionData: drawingCanvas.objectProperties
    spacing: 6
    visible: selectionData.vectorOnly || false
    RowLayout {
        spacing: 4
        CompactButton {
            text: "Align ▾"
            enabled: root.selectionData.count >= 2
            onClicked: alignMenu.open()
            Menu {
                id: alignMenu
                MenuItem { text: "Left edges"; onTriggered: root.drawingCanvas.editVectorSelection("arrange", 0) }
                MenuItem { text: "Horizontal centers"; onTriggered: root.drawingCanvas.editVectorSelection("arrange", 1) }
                MenuItem { text: "Right edges"; onTriggered: root.drawingCanvas.editVectorSelection("arrange", 2) }
                MenuItem { text: "Top edges"; onTriggered: root.drawingCanvas.editVectorSelection("arrange", 3) }
                MenuItem { text: "Vertical centers"; onTriggered: root.drawingCanvas.editVectorSelection("arrange", 4) }
                MenuItem { text: "Bottom edges"; onTriggered: root.drawingCanvas.editVectorSelection("arrange", 5) }
                MenuSeparator {}
                MenuItem { text: "Distribute centers horizontally"; enabled: root.selectionData.count >= 3; onTriggered: root.drawingCanvas.editVectorSelection("arrange", 6) }
                MenuItem { text: "Distribute centers vertically"; enabled: root.selectionData.count >= 3; onTriggered: root.drawingCanvas.editVectorSelection("arrange", 7) }
            }
        }
        CompactButton {
            text: "Order ▾"
            onClicked: orderMenu.open()
            Menu {
                id: orderMenu
                MenuItem { text: "Bring to front in art layer"; onTriggered: root.drawingCanvas.editVectorSelection("order", 3) }
                MenuItem { text: "Bring forward"; onTriggered: root.drawingCanvas.editVectorSelection("order", 2) }
                MenuItem { text: "Send backward"; onTriggered: root.drawingCanvas.editVectorSelection("order", 1) }
                MenuItem { text: "Send to back in art layer"; onTriggered: root.drawingCanvas.editVectorSelection("order", 0) }
            }
        }
    }
    ColumnLayout {
        visible: root.selectionData.count > 1
        spacing: 4
        Label { text: "Apply to all selected vectors"; color: "#999999"; font.pixelSize: 10 }
        RowLayout {
            Label { text: "Width"; color: "#999999" }
            CompactTextField {
                id: groupWidth
                Layout.fillWidth: true
                placeholderText: "Mixed"
                selectByMouse: true
                validator: DoubleValidator { bottom: .01; top: 4096; locale: "C" }
                Accessible.name: "Group stroke width"
                Binding {
                    target: groupWidth
                    property: "text"
                    value: root.selectionData.commonStrokeWidth === null || root.selectionData.commonStrokeWidth === undefined ? "" : Number(root.selectionData.commonStrokeWidth).toFixed(2)
                    when: !groupWidth.activeFocus
                }
                onEditingFinished: {
                    if (acceptableInput) root.drawingCanvas.editVectorSelection("strokeWidth", Number(text))
                    focus = false
                }
            }
        }
        CompactComboBox {
            Layout.fillWidth: true
            model: root.controller.palette
            textRole: "name"
            valueRole: "id"
            currentIndex: -1
            displayText: currentIndex < 0 ? "Apply color…" : currentText
            Accessible.name: "Apply color to selected vectors"
            onActivated: { root.drawingCanvas.editVectorSelection("swatch", currentValue); currentIndex = -1 }
        }
        CompactComboBox {
            Layout.fillWidth: true
            model: ["Underlay Art", "Color Art", "Line Art", "Overlay Art"]
            currentIndex: -1
            displayText: currentIndex < 0 ? "Move to art layer…" : currentText
            Accessible.name: "Art layer for selected vectors"
            onActivated: { root.drawingCanvas.editVectorSelection("artLayer", currentIndex); currentIndex = -1 }
        }
        RowLayout {
            CompactButton { text: "Fill shapes"; onClicked: root.drawingCanvas.editVectorSelection("filled", 1) }
            CompactButton { text: "Outline"; onClicked: root.drawingCanvas.editVectorSelection("filled", 0) }
        }
    }
    Label { text: "Pencil cleanup"; color: "#999999"; font.pixelSize: 10 }
    RowLayout {
        CompactComboBox {
            id: strength
            model: ["Light · 25%", "Medium · 50%", "Strong · 75%"]
            currentIndex: 1
            Layout.fillWidth: true
            Accessible.name: "Smoothing strength"
        }
        CompactButton { text: "Smooth"; onClicked: root.drawingCanvas.editVectorSelection("smooth", (strength.currentIndex + 1) * .25) }
    }
    RowLayout {
        CompactComboBox {
            id: tolerance
            model: ["0.25 px", "0.5 px", "1 px", "2 px", "4 px"]
            currentIndex: 2
            Layout.fillWidth: true
            Accessible.name: "Simplify centerline tolerance in local pixels"
        }
        CompactButton { text: "Simplify"; onClicked: root.drawingCanvas.editVectorSelection("simplify", [.25, .5, 1, 2, 4][tolerance.currentIndex]) }
    }
    Label {
        text: "Cleanup affects sampled pencil strokes. Endpoints and pressure are retained; smoothing protects sharp corners."
        wrapMode: Text.WordWrap
        Layout.fillWidth: true
        font.pixelSize: 10
        color: "#888888"
    }
}
