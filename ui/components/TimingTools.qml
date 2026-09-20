import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

Flow {
    id: root
    required property var controller
    spacing: 6
    property alias contentMode: content.currentIndex
    Label {
        text: "Frames " + (root.controller.rangeStart + 1) + "–" + root.controller.rangeEnd
        height: 28
        verticalAlignment: Text.AlignVCenter
    }
    CompactComboBox {
        id: content
        model: ["Exposures", "Independent drawings", "Keys only", "Exposures + keys"]
        implicitWidth: 145
        implicitHeight: 28
        Accessible.name: "Timing edit content"
    }
    CompactButton {
        text: "Copy"
        implicitHeight: 28
        onClicked: root.controller.copyTimelineRange()
    }
    CompactButton {
        text: "Paste"
        implicitHeight: 28
        enabled: root.controller.hasClipboard
        onClicked: root.controller.pasteTimelineRange(content.currentIndex, false)
    }
    CompactButton {
        text: "Insert"
        implicitHeight: 28
        enabled: root.controller.hasClipboard
        onClicked: root.controller.pasteTimelineRange(content.currentIndex, true)
    }
    CompactComboBox {
        id: step
        implicitWidth: 95
        implicitHeight: 28
        model: ["On ones", "On twos", "On threes"]
        onActivated: root.controller.timeSelectedDrawings(currentIndex + 1)
        Accessible.name: "Drawing timing"
    }
    CompactSpinBox {
        id: repeats
        from: 1
        to: 1000
        value: 2
        editable: true
        implicitWidth: 105
        implicitHeight: 28
        Accessible.name: "Repeat count"
    }
    CompactButton {
        text: "Repeat"
        implicitHeight: 28
        onClicked: root.controller.repeatTimelineRange(repeats.value)
    }
    CompactButton {
        text: "Clear"
        implicitHeight: 28
        onClicked: root.controller.clearTimelineRange(content.currentIndex >= 2)
    }
    Label {
        text: "Drag the range end to stretch · Alt-drag to move"
        color: "#999999"
        height: 28
        verticalAlignment: Text.AlignVCenter
    }
}
