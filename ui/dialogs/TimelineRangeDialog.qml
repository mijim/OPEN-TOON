import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

Dialog {
    id: root
    required property var controller
    title: "Edit timeline range"
    modal: true
    width: 470
    anchors.centerIn: parent
    standardButtons: Dialog.Close
    contentItem: ColumnLayout {
        spacing: 14
        Label {
            text: "Frames " + (root.controller.rangeStart + 1) + "–" + root.controller.rangeEnd + " · " + root.controller.selectedLayers.length + " layers"
        }
        Label {
            text: "Drag across timeline or Xsheet cells to select a range. Paste starts at the playhead. Exposure and key operations stay separate."
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            color: "#aaaaaa"
        }
        RowLayout {
            Button {
                text: "Copy range"
                onClicked: root.controller.copyTimelineRange()
            }
            ComboBox {
                id: content
                model: ["Exposures", "Independent drawings", "Keys only", "Exposures + keys"]
                Layout.fillWidth: true
            }
        }
        RowLayout {
            Button {
                text: "Paste / overwrite"
                enabled: root.controller.hasClipboard
                onClicked: root.controller.pasteTimelineRange(content.currentIndex, false)
            }
            Button {
                text: "Paste / insert time"
                enabled: root.controller.hasClipboard
                onClicked: root.controller.pasteTimelineRange(content.currentIndex, true)
            }
        }
        RowLayout {
            SpinBox {
                id: repeats
                from: 1
                to: 1000
                value: 2
                editable: true
                Accessible.name: "Repeat count"
            }
            Button {
                text: "Repeat exposures"
                onClicked: root.controller.repeatTimelineRange(repeats.value)
            }
        }
        RowLayout {
            SpinBox {
                id: length
                from: 1
                to: 1000000
                value: root.controller.rangeEnd - root.controller.rangeStart
                editable: true
                Accessible.name: "Retimed frame count"
            }
            Button {
                text: "Stretch to frames"
                onClicked: root.controller.retimeTimelineRange(length.value)
            }
        }
        Label {
            text: "Overwrite and stretch replace the destination range. Undo restores the original. Compression that would discard a drawing or merge keys is rejected."
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            color: "#aaaaaa"
        }
        RowLayout {
            Button {
                text: "On ones"
                onClicked: root.controller.timeSelectedDrawings(1)
            }
            Button {
                text: "On twos"
                onClicked: root.controller.timeSelectedDrawings(2)
            }
            Button {
                text: "On threes"
                onClicked: root.controller.timeSelectedDrawings(3)
            }
        }
        RowLayout {
            Button {
                text: "Clear exposures"
                onClicked: root.controller.clearTimelineRange(false)
            }
            Button {
                text: "Clear exposures + keys"
                onClicked: root.controller.clearTimelineRange(true)
            }
        }
    }
}
