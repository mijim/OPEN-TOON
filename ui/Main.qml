import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Dialogs
import OpenToon.Native
import "components" as C

ApplicationWindow {
    id: root
    width: 1440
    height: 920
    minimumWidth: 1080
    minimumHeight: 720
    visible: true
    title: editor.sceneName + (editor.modified ? " •" : "") + " — OPEN-TOON"
    color: "#0a0a0a"
    font.family: "Helvetica Neue"
    font.pixelSize: 12
    palette.window: "#111111"
    palette.windowText: "#ededed"
    palette.base: "#191919"
    palette.text: "#ededed"
    palette.button: "#202020"
    palette.buttonText: "#ededed"
    palette.highlight: "#454545"
    palette.highlightedText: "#ffffff"
    palette.mid: "#393939"
    Shortcut {
        sequence: "A"
        enabled: !root.textEditing
        onActivated: editor.tool = "Animate"
    }
    property bool showCurves: false
    property bool showTimingTools: false
    property string inspectorMode: "none"
    property real bottomHeight: 280
    readonly property real maximumBottomHeight: Math.max(140, workspace.height - appHeader.height - canvasToolbar.height - bottomSplitter.height - bottomTabs.height - statusBar.height - (timingTools.visible ? timingTools.height : 0) - canvasWorkspace.Layout.minimumHeight - 2)
    readonly property real effectiveBottomHeight: Math.max(140, Math.min(bottomHeight, maximumBottomHeight))
    Connections {
        target: canvas
        function onRegionChanged() {
            if (canvas.hasRegion)
                root.inspectorMode = editor.tool === "Animate" ? "layer" : "object";
        }
    }
    property var backend: editor
    property string pendingAction: ""
    property bool allowClose: false
    property bool textEditing: activeFocusItem instanceof TextInput || activeFocusItem instanceof TextEdit
    property bool xsheet: false
    property bool keyEditing: editor.tool === "Animate"
    property int timelineCell: 22
    property int timelineRow: 34
    property int colorEditId: 0
    function requestAction(action) {
        if (editor.modified) {
            pendingAction = action;
            discardDialog.open();
        } else
            performAction(action);
    }
    function performAction(action) {
        if (action === "new")
            editor.newScene();
        else if (action === "demo")
            editor.loadDemo();
        else if (action === "open")
            openDialog.open();
        else if (action === "quit") {
            allowClose = true;
            root.close();
        }
    }
    function save() {
        if (editor.projectPath.length)
            editor.saveProject();
        else
            saveDialog.open();
    }
    onClosing: function (close) {
        if (editor.exporting) {
            close.accepted = false;
            editor.report("Cancel or finish the export before closing.");
            return;
        }
        if (editor.modified && !allowClose) {
            close.accepted = false;
            requestAction("quit");
        }
    }

    Shortcut {
        sequences: [StandardKey.Copy]
        enabled: !root.textEditing && timeline.activeFocus
        onActivated: editor.copyTimelineRange()
    }
    Shortcut {
        sequences: [StandardKey.Paste]
        enabled: !root.textEditing && timeline.activeFocus
        onActivated: editor.pasteTimelineRange(0, false)
    }
    Dialog {
        id: markerDialog
        title: "Scene marker"
        modal: true
        anchors.centerIn: parent
        standardButtons: Dialog.Ok | Dialog.Cancel
        onAccepted: editor.setSceneMarker(markerName.text)
        C.CompactTextField {
            id: markerName
            placeholderText: "Marker label (empty removes marker)"
            width: 330
            selectByMouse: true
        }
    }
    FileDialog {
        id: xsheetDialog
        title: "Export Xsheet PDF"
        fileMode: FileDialog.SaveFile
        defaultSuffix: "pdf"
        nameFilters: ["PDF files (*.pdf)"]
        onAccepted: editor.exportXsheet(selectedFile)
    }
    Shortcut {
        sequences: [StandardKey.New]
        onActivated: root.requestAction("new")
    }
    Shortcut {
        sequences: [StandardKey.Open]
        onActivated: root.requestAction("open")
    }
    Shortcut {
        sequences: [StandardKey.Save]
        onActivated: root.save()
    }
    Shortcut {
        sequences: [StandardKey.SaveAs]
        onActivated: saveDialog.open()
    }
    Shortcut {
        sequences: [StandardKey.Undo]
        enabled: !root.textEditing
        onActivated: editor.undo()
    }
    Shortcut {
        sequences: [StandardKey.Redo]
        enabled: !root.textEditing
        onActivated: editor.redo()
    }
    Shortcut {
        sequence: "B"
        enabled: !root.textEditing
        onActivated: editor.tool = "Pencil"
    }
    Shortcut {
        sequence: "E"
        enabled: !root.textEditing
        onActivated: editor.tool = "Eraser"
    }
    Shortcut {
        sequence: "M"
        enabled: !root.textEditing
        onActivated: editor.tool = "Marquee"
    }
    Shortcut {
        sequence: "V"
        enabled: !root.textEditing
        onActivated: editor.tool = "Select"
    }
    Shortcut {
        sequence: "Space"
        enabled: !root.textEditing
        onActivated: editor.togglePlayback()
    }
    Shortcut {
        sequence: "Right"
        enabled: !root.textEditing
        onActivated: editor.frame++
    }
    Shortcut {
        sequence: "Left"
        enabled: !root.textEditing
        onActivated: editor.frame--
    }
    Shortcut {
        sequence: "F"
        enabled: !root.textEditing
        onActivated: canvas.fit()
    }
    Shortcut {
        sequences: ["Delete", "Backspace"]
        enabled: !root.textEditing
        onActivated: canvas.deleteSelection()
    }
    Shortcut {
        sequence: "O"
        enabled: !root.textEditing
        onActivated: editor.onionSkin = !editor.onionSkin
    }

    menuBar: MenuBar {
        background: Rectangle {
            color: "#0e0e0e"
        }
        Menu {
            title: "Scene"
            Action {
                text: "New scene"
                onTriggered: root.requestAction("new")
            }
            Action {
                text: "Open project…"
                onTriggered: root.requestAction("open")
            }
            Action {
                text: "Save"
                onTriggered: root.save()
            }
            Action {
                text: "Save As…"
                onTriggered: saveDialog.open()
            }
            MenuSeparator {}
            Action {
                text: "Import image…"
                onTriggered: imageDialog.open()
            }
            Action {
                text: "Export PNG sequence…"
                enabled: !editor.exporting
                onTriggered: exportDialog.open()
            }
            Action {
                text: "Scene settings…"
                onTriggered: settingsDialog.open()
            }
            MenuSeparator {}
            Action {
                text: "Open bouncing ball example"
                onTriggered: root.requestAction("demo")
            }
            Action {
                text: editor.savingRecovery ? "Saving recovery snapshot…" : "Save recovery snapshot"
                enabled: !editor.savingRecovery
                onTriggered: editor.autosave()
            }
            Action {
                text: "Compact project history…"
                enabled: editor.projectPath.length > 0 && !editor.modified
                onTriggered: compactDialog.open()
            }
            Action {
                text: "Quit"
                onTriggered: root.requestAction("quit")
            }
        }
        Menu {
            title: "Edit"
            Action {
                text: "Timeline range…"
                onTriggered: root.showTimingTools = !root.showTimingTools
            }
            Action {
                text: "Copy timeline range"
                onTriggered: editor.copyTimelineRange()
            }
            Action {
                text: "Paste exposures"
                enabled: editor.hasClipboard
                onTriggered: editor.pasteTimelineRange(0, false)
            }
            Action {
                text: "Scene marker…"
                onTriggered: markerDialog.open()
            }
            Action {
                text: "Export Xsheet PDF…"
                onTriggered: xsheetDialog.open()
            }
            MenuSeparator {}

            Action {
                text: "Undo"
                enabled: editor.canUndo
                onTriggered: editor.undo()
            }
            Action {
                text: "Redo"
                enabled: editor.canRedo
                onTriggered: editor.redo()
            }
            MenuSeparator {}
            Action {
                text: "New drawing on twos"
                onTriggered: editor.newDrawing(false)
            }
            Action {
                text: "Duplicate drawing"
                onTriggered: editor.newDrawing(true)
            }
            Action {
                text: "Hold for two frames"
                onTriggered: editor.holdDrawing(2)
            }
            Action {
                text: "Hold for four frames"
                onTriggered: editor.holdDrawing(4)
            }
            Action {
                text: "Clear current frame and key"
                onTriggered: editor.clearExposure()
            }
            Action {
                text: "Insert two frames"
                onTriggered: editor.insertFrames(2)
            }
            Action {
                text: "Remove two frames"
                onTriggered: editor.removeFrames(2)
            }
        }
        Menu {
            title: "View"
            Action {
                text: "Fit canvas"
                onTriggered: canvas.fit()
            }
            Action {
                text: "Mirror canvas"
                checkable: true
                checked: canvas.mirrored
                onTriggered: canvas.mirrored = !canvas.mirrored
            }
            Action {
                text: "Rotate view 15°"
                onTriggered: canvas.rotationAngle += 15
            }
            Action {
                text: "Onion skin"
                checkable: true
                checked: editor.onionSkin
                onTriggered: editor.onionSkin = !editor.onionSkin
            }
            Action {
                text: "Revision history"
                onTriggered: historyDialog.open()
            }
        }
        Menu {
            title: "Help"
            Action {
                text: "Keyboard and mouse controls"
                onTriggered: helpDialog.open()
            }
            Action {
                text: "Implementation status"
                onTriggered: Qt.openUrlExternally("https://github.com/mijim/OPEN-TOON/blob/main/docs/implementation/STATUS.md")
            }
        }
    }

    ColumnLayout {
        id: workspace
        anchors.fill: parent
        spacing: 0
        Rectangle {
            id: appHeader
            Layout.fillWidth: true
            Layout.preferredHeight: 44
            Layout.minimumHeight: 44
            Layout.maximumHeight: 44
            color: "#0a0a0a"
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 20
                anchors.rightMargin: 18
                spacing: 14
                Text {
                    text: "◩"
                    color: "white"
                    font.pixelSize: 25
                }
                Text {
                    text: "OPEN-TOON"
                    color: "#fafafa"
                    font.pixelSize: 14
                    font.letterSpacing: 1.3
                    font.bold: true
                }
                Rectangle {
                    width: 1
                    height: 20
                    color: "#303030"
                }
                Text {
                    text: editor.sceneName
                    color: "#b5b5b5"
                    elide: Text.ElideRight
                    Layout.preferredWidth: 220
                    Layout.maximumWidth: 350
                }
                Rectangle {
                    Layout.preferredWidth: stateLabel.implicitWidth + 16
                    height: 22
                    radius: 4
                    color: "#191919"
                    border.color: "#333333"
                    Text {
                        id: stateLabel
                        anchors.centerIn: parent
                        text: editor.modified ? "Unsaved changes" : "Saved"
                        color: "#999999"
                        font.pixelSize: 10
                    }
                }
                Item {
                    Layout.fillWidth: true
                }
                Text {
                    text: Qt.application.version
                    color: "#737373"
                    font.pixelSize: 10
                    font.letterSpacing: 0.8
                }
                C.ToolButton {
                    text: "Save"
                    hint: "Save project · Cmd/Ctrl+S"
                    onClicked: root.save()
                }
                C.ToolButton {
                    text: editor.exporting ? "Exporting…" : "Export ↗"
                    active: true
                    enabled: !editor.exporting
                    onClicked: exportDialog.open()
                }
            }
        }
        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#282828"
        }
        Rectangle {
            id: canvasToolbar
            Layout.fillWidth: true
            Layout.preferredHeight: 34
            Layout.minimumHeight: 34
            Layout.maximumHeight: 34
            color: "#111111"
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 16
                spacing: 10
                Text {
                    text: editor.tool
                    color: "#e5e5e5"
                    Layout.preferredWidth: 76
                }
                C.CompactComboBox {
                    visible: editor.tool.startsWith("Raster ")
                    model: ["Raster ink", "Raster soft", "Raster dry", "Raster smudge", "Raster eraser"]
                    currentIndex: Math.max(0, model.indexOf(editor.tool))
                    implicitHeight: 28
                    implicitWidth: 138
                    onActivated: editor.tool = currentText
                    Accessible.name: "Raster brush preset"
                }
                Text {
                    visible: editor.tool !== "Marquee" && editor.tool !== "Select" && editor.tool !== "Animate"
                    text: "Size"
                    color: "#858585"
                    font.pixelSize: 11
                }
                Slider {
                    visible: editor.tool !== "Marquee" && editor.tool !== "Select" && editor.tool !== "Animate"
                    from: 0.5
                    to: 100
                    value: editor.brushSize
                    Layout.preferredWidth: 120
                    onMoved: editor.brushSize = value
                    Accessible.name: "Brush size"
                }
                Text {
                    visible: editor.tool !== "Marquee" && editor.tool !== "Select" && editor.tool !== "Animate"
                    text: editor.brushSize.toFixed(1) + " px"
                    color: "#aaaaaa"
                    Layout.preferredWidth: 58
                    font.family: "Menlo"
                    font.pixelSize: 10
                }
                RowLayout {
                    visible: editor.tool.startsWith("Raster ")
                    Label {
                        text: "Opacity"
                        color: "#aaaaaa"
                    }
                    Slider {
                        from: 0
                        to: 1
                        value: editor.brushOpacity
                        Layout.preferredWidth: 85
                        onMoved: editor.brushOpacity = value
                        Accessible.name: "Raster brush opacity"
                    }
                    Label {
                        text: Math.round(editor.brushOpacity * 100) + "%"
                        Layout.preferredWidth: 32
                    }
                }
                Label {
                    visible: editor.tool === "Edit points"
                    text: "Drag points · Double-click a segment to add · Delete removes the selected point"
                    color: "#bbbbbb"
                }
                Label {
                    visible: editor.tool === "Animate"
                    text: canvas.hasRegion ? "Animate layer · Drag to pose · Handles scale / rotate · Each gesture records a key" : "Animate layer · No artwork at this frame — extend the drawing exposure in the timeline"
                    color: "#bbbbbb"
                }
                C.CompactComboBox {
                    visible: editor.tool === "Marquee"
                    model: ["Vectors", "Raster pixels", "Vectors + raster"]
                    currentIndex: canvas.selectionMedia
                    onActivated: canvas.selectionMedia = currentIndex
                    implicitWidth: 150
                    implicitHeight: 28
                    Accessible.name: "Drawing selection media"
                }
                C.ToolButton {
                    visible: editor.tool === "Marquee" || editor.tool === "Select"
                    text: "Duplicate"
                    enabled: canvas.hasRegion
                    onClicked: canvas.transformRegion(1, 24, 24)
                }
                C.ToolButton {
                    visible: editor.tool === "Marquee" || editor.tool === "Select"
                    text: "Flip H"
                    enabled: canvas.hasRegion
                    onClicked: canvas.transformRegion(3)
                }
                C.ToolButton {
                    visible: editor.tool === "Marquee" || editor.tool === "Select"
                    text: "Flip V"
                    enabled: canvas.hasRegion
                    onClicked: canvas.transformRegion(4)
                }
                C.ToolButton {
                    visible: editor.tool === "Marquee" || editor.tool === "Select"
                    text: "Deselect"
                    enabled: canvas.hasRegion
                    onClicked: canvas.clearRegion()
                }
                C.ToolButton {
                    text: "Fill shape"
                    visible: !editor.tool.startsWith("Raster ") && editor.tool !== "Marquee" && editor.tool !== "Select" && editor.tool !== "Animate"
                    active: editor.filled
                    onClicked: editor.filled = !editor.filled
                    hint: "Fill new rectangles and ellipses"
                }
                C.CompactComboBox {
                    visible: !editor.tool.startsWith("Raster ") && editor.tool !== "Marquee" && editor.tool !== "Select" && editor.tool !== "Animate"
                    model: ["Underlay Art", "Color Art", "Line Art", "Overlay Art"]
                    currentIndex: editor.artLayer
                    implicitHeight: 28
                    implicitWidth: 120
                    onActivated: editor.artLayer = currentIndex
                    Accessible.name: "Active art layer"
                }
                Item {
                    Layout.fillWidth: true
                }
                C.ToolButton {
                    text: "Onion"
                    active: editor.onionSkin
                    hint: "Previous and next distinct drawing · O"
                    onClicked: editor.onionSkin = !editor.onionSkin
                }
                C.ToolButton {
                    text: "−"
                    onClicked: canvas.zoom /= 1.2
                    hint: "Zoom out"
                }
                Text {
                    text: Math.round(canvas.zoom * 100) + "%"
                    color: "#999999"
                    font.pixelSize: 11
                }
                C.ToolButton {
                    text: "+"
                    onClicked: canvas.zoom *= 1.2
                    hint: "Zoom in"
                }
                C.ToolButton {
                    text: "Fit"
                    onClicked: canvas.fit()
                    hint: "Fit canvas · F"
                }
            }
        }
        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#282828"
        }
        RowLayout {
            id: canvasWorkspace
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 160
            Layout.preferredHeight: 580
            spacing: 0
            Rectangle {
                Layout.preferredWidth: 48
                Layout.fillHeight: true
                color: "#111111"
                Flickable {
                    anchors.fill: parent
                    clip: true
                    contentHeight: toolColumn.height + 24
                    boundsBehavior: Flickable.StopAtBounds
                    Column {
                        id: toolColumn
                        y: 12
                        x: (parent.width - width) / 2
                        spacing: 4
                        Repeater {
                            model: [
                                {
                                    name: "Select",
                                    icon: "↖",
                                    key: "V"
                                },
                                {
                                    name: "Animate",
                                    icon: "◇",
                                    key: "A"
                                },
                                {
                                    name: "Marquee",
                                    icon: "▧",
                                    key: "M"
                                },
                                {
                                    name: "Pencil",
                                    icon: "╱",
                                    key: "B"
                                },
                                {
                                    name: "Raster ink",
                                    icon: "◉",
                                    key: ""
                                },
                                {
                                    name: "Eraser",
                                    icon: "▱",
                                    key: "E"
                                },
                                {
                                    name: "Rectangle",
                                    icon: "□",
                                    key: ""
                                },
                                {
                                    name: "Ellipse",
                                    icon: "○",
                                    key: ""
                                },
                                {
                                    name: "Recolor",
                                    icon: "◒",
                                    key: ""
                                },
                                {
                                    name: "Edit points",
                                    icon: "⌘",
                                    key: ""
                                }
                            ]
                            C.ToolButton {
                                required property var modelData
                                width: 34
                                height: 28
                                text: modelData.icon
                                font.pixelSize: 18
                                active: editor.tool === modelData.name
                                hint: modelData.name + (modelData.key ? " · " + modelData.key : "")
                                onClicked: editor.tool = modelData.name
                            }
                        }
                    }
                }
            }
            Rectangle {
                width: 1
                Layout.fillHeight: true
                color: "#282828"
            }
            Item {
                Layout.fillHeight: true
                Layout.fillWidth: true
                DrawingCanvas {
                    id: canvas
                    objectName: "drawingCanvas"
                    anchors.fill: parent
                    editor: root.backend
                }
                Text {
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.margins: 16
                    text: "CAMERA"
                    color: "#777777"
                    font.pixelSize: 10
                    font.letterSpacing: 1.4
                }
                Text {
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.margins: 14
                    text: editor.sceneWidth + " × " + editor.sceneHeight + "   ·   " + editor.fps.toFixed(2) + " fps"
                    color: "#777777"
                    font.family: "Menlo"
                    font.pixelSize: 10
                }
            }
            Rectangle {
                width: 1
                Layout.fillHeight: true
                color: "#282828"
            }
            Rectangle {
                Layout.preferredWidth: 250
                Layout.fillHeight: true
                color: "#101010"
                ScrollView {
                    anchors.fill: parent
                    clip: true
                    contentWidth: availableWidth
                    ColumnLayout {
                        width: 248
                        spacing: 16
                        Text {
                            Layout.leftMargin: 16
                            Layout.topMargin: 18
                            text: "PROPERTIES"
                            color: "#888888"
                            font.pixelSize: 10
                            font.letterSpacing: 1.4
                        }
                        Label {
                            visible: canvas.objectProperties.kind === "none" && root.inspectorMode !== "layer"
                            Layout.leftMargin: 16
                            Layout.rightMargin: 16
                            Layout.fillWidth: true
                            text: "Select an object on the canvas to edit its properties, or select a layer in the layer list."
                            wrapMode: Text.WordWrap
                            color: "#999999"
                        }
                        C.SelectionProperties {
                            visible: canvas.objectProperties.kind !== "none"
                            Layout.leftMargin: 16
                            Layout.rightMargin: 16
                            Layout.fillWidth: true
                            drawingCanvas: canvas
                            controller: editor
                        }
                        ColumnLayout {
                            visible: root.inspectorMode === "layer" && canvas.objectProperties.kind === "none"
                            Layout.fillWidth: true
                            Label {
                                Layout.leftMargin: 16
                                text: "Layer · " + (editor.layers.find(l => l.id === editor.selectedLayer)?.name || "")
                                font.pixelSize: 13
                                font.bold: true
                            }
                            RowLayout {
                                Layout.leftMargin: 16
                                C.CompactComboBox {
                                    model: ["Setup", "Animate"]
                                    enabled: editor.tool !== "Animate"
                                    currentIndex: editor.animateMode ? 1 : 0
                                    onActivated: editor.animateMode = currentIndex === 1
                                    Accessible.name: "Animation edit mode"
                                    implicitWidth: 105
                                }
                                C.CompactCheckBox {
                                    text: editor.tool === "Animate" ? "Gesture keys" : "Auto key"
                                    checked: editor.tool === "Animate" || editor.autoKey
                                    enabled: editor.tool !== "Animate" && editor.animateMode
                                    onToggled: editor.autoKey = checked
                                }
                            }
                            Label {
                                Layout.leftMargin: 16
                                Layout.rightMargin: 16
                                Layout.fillWidth: true
                                wrapMode: Text.WordWrap
                                color: "#aaaaaa"
                                text: {
                                    const revision = editor.animationKeys;
                                    const mode = editor.animateMode;
                                    return mode ? editor.keyState : "Rest values · existing keys keep their poses";
                                }
                            }
                            RowLayout {
                                Layout.leftMargin: 12
                                C.ToolButton {
                                    text: "‹ Key"
                                    onClicked: editor.nextKey(-1)
                                }
                                C.ToolButton {
                                    text: "Key ›"
                                    onClicked: editor.nextKey(1)
                                }
                                C.ToolButton {
                                    text: "Curves"
                                    onClicked: root.showCurves = true
                                }
                            }
                            GridLayout {
                                Layout.leftMargin: 16
                                Layout.rightMargin: 16
                                Layout.fillWidth: true
                                columns: 2
                                columnSpacing: 8
                                rowSpacing: 8
                                Repeater {
                                    model: [
                                        {
                                            key: "x",
                                            name: "Position X"
                                        },
                                        {
                                            key: "y",
                                            name: "Position Y"
                                        },
                                        {
                                            key: "rotation",
                                            name: "Rotation °"
                                        },
                                        {
                                            key: "opacity",
                                            name: "Opacity 0–1"
                                        },
                                        {
                                            key: "scaleX",
                                            name: "Scale X"
                                        },
                                        {
                                            key: "scaleY",
                                            name: "Scale Y"
                                        },
                                        {
                                            key: "pivotX",
                                            name: "Pivot X"
                                        },
                                        {
                                            key: "pivotY",
                                            name: "Pivot Y"
                                        }
                                    ]
                                    ColumnLayout {
                                        required property var modelData
                                        Layout.fillWidth: true
                                        spacing: 4
                                        Text {
                                            text: modelData.name
                                            color: "#888888"
                                            font.pixelSize: 10
                                        }
                                        C.PropertyNumber {
                                            Layout.fillWidth: true
                                            Layout.preferredWidth: 96
                                            number: {
                                                const f = editor.frame;
                                                return Number(editor.transform[modelData.key] || 0);
                                            }
                                            label: modelData.name
                                            onCommitted: value => editor.setTransform(modelData.key, value)
                                        }
                                    }
                                }
                            }
                            RowLayout {
                                Layout.leftMargin: 12
                                C.ToolButton {
                                    text: "◇ Add key"
                                    onClicked: editor.addKey(interpolation.currentIndex)
                                }
                                C.CompactComboBox {
                                    id: interpolation
                                    model: ["Linear", "Hold", "Smooth"]
                                    implicitWidth: 98
                                    implicitHeight: 28
                                    Accessible.name: "Key interpolation"
                                }
                            }
                            C.CompactComboBox {
                                Layout.leftMargin: 16
                                Layout.rightMargin: 16
                                Layout.fillWidth: true
                                implicitHeight: 30
                                model: [
                                    {
                                        id: 0,
                                        name: "No parent"
                                    }
                                ].concat(editor.layers.filter(function (l) {
                                    return l.id !== editor.selectedLayer;
                                }))
                                currentIndex: {
                                    const selected = editor.layers.find(function (l) {
                                        return l.id === editor.selectedLayer;
                                    });
                                    return model.findIndex(function (l) {
                                        return l.id === (selected ? selected.parent : 0);
                                    });
                                }
                                textRole: "name"
                                valueRole: "id"
                                onActivated: editor.setParent(currentValue)
                                Accessible.name: "Parent layer"
                            }
                        }
                        Rectangle {
                            Layout.fillWidth: true
                            height: 1
                            color: "#282828"
                        }
                        RowLayout {
                            Layout.leftMargin: 16
                            Layout.rightMargin: 12
                            Layout.fillWidth: true
                            Label {
                                text: "Palette"
                                font.pixelSize: 13
                                font.bold: true
                            }
                            Item {
                                Layout.fillWidth: true
                            }
                            C.ToolButton {
                                text: "+"
                                hint: "Add color"
                                onClicked: {
                                    root.colorEditId = 0;
                                    colorDialog.open();
                                }
                            }
                        }
                        Flow {
                            Layout.leftMargin: 16
                            Layout.rightMargin: 16
                            Layout.fillWidth: true
                            spacing: 8
                            Repeater {
                                model: editor.palette
                                Rectangle {
                                    required property var modelData
                                    width: 34
                                    height: 34
                                    radius: 5
                                    color: modelData.color
                                    border.width: editor.selectedSwatch === modelData.id ? 3 : 1
                                    border.color: editor.selectedSwatch === modelData.id ? "#b0b0b0" : "#444444"
                                    MouseArea {
                                        anchors.fill: parent
                                        onClicked: editor.selectedSwatch = modelData.id
                                        onDoubleClicked: {
                                            root.colorEditId = modelData.id;
                                            colorDialog.selectedColor = modelData.color;
                                            colorDialog.open();
                                        }
                                    }
                                    Accessible.name: modelData.name
                                }
                            }
                        }
                        Text {
                            Layout.leftMargin: 16
                            Layout.rightMargin: 16
                            Layout.fillWidth: true
                            text: "Double-click a swatch to recolor every linked stroke."
                            wrapMode: Text.WordWrap
                            color: "#777777"
                            font.pixelSize: 11
                        }
                        Rectangle {
                            Layout.fillWidth: true
                            height: 1
                            color: "#282828"
                        }
                        ColumnLayout {
                            Layout.leftMargin: 12
                            Layout.rightMargin: 12
                            Layout.fillWidth: true
                            spacing: 6
                            C.ToolButton {
                                text: "+ New drawing"
                                Layout.fillWidth: true
                                onClicked: editor.newDrawing(false)
                                hint: "Create a distinct drawing exposed on twos"
                            }
                            C.ToolButton {
                                text: "Duplicate drawing"
                                Layout.fillWidth: true
                                onClicked: editor.newDrawing(true)
                            }
                            C.ToolButton {
                                text: "Smooth selected stroke"
                                Layout.fillWidth: true
                                onClicked: canvas.smoothSelection()
                            }
                            C.ToolButton {
                                text: "Hold for 4 frames"
                                Layout.fillWidth: true
                                onClicked: editor.holdDrawing(4)
                            }
                        }
                        Item {
                            Layout.preferredHeight: 12
                        }
                    }
                }
            }
        }
        Rectangle {
            id: bottomSplitter
            objectName: "workspaceSplitter"
            Layout.fillWidth: true
            Layout.preferredHeight: 8
            Layout.minimumHeight: 8
            Layout.maximumHeight: 8
            color: resizeHandle.containsMouse || resizeHandle.pressed ? "#303030" : "#171717"
            Rectangle {
                anchors.centerIn: parent
                width: 40
                height: 2
                radius: 1
                color: "#666666"
            }
            MouseArea {
                id: resizeHandle
                anchors.fill: parent
                hoverEnabled: true
                preventStealing: true
                cursorShape: Qt.SplitVCursor
                property real startY
                property real startHeight
                onPressed: mouse => {
                    startY = mapToGlobal(mouse.x, mouse.y).y;
                    startHeight = root.effectiveBottomHeight;
                }
                onPositionChanged: mouse => {
                    if (pressed)
                        root.bottomHeight = Math.max(140, Math.min(root.maximumBottomHeight, startHeight + startY - mapToGlobal(mouse.x, mouse.y).y));
                }
                onDoubleClicked: root.bottomHeight = 280
                Accessible.name: "Resize timeline and curves vertically"
            }
        }
        Rectangle {
            id: bottomTabs
            Layout.fillWidth: true
            Layout.preferredHeight: 34
            Layout.minimumHeight: 34
            Layout.maximumHeight: 34
            color: "#111111"
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 14
                anchors.rightMargin: 14
                spacing: 8
                C.ToolButton {
                    text: "Timeline"
                    active: !root.xsheet && !root.showCurves
                    onClicked: {
                        root.showCurves = false;
                        root.xsheet = false;
                    }
                }
                C.ToolButton {
                    text: "Xsheet"
                    active: root.xsheet && !root.showCurves
                    onClicked: {
                        root.showCurves = false;
                        root.xsheet = true;
                    }
                }
                C.ToolButton {
                    text: "Curves"
                    active: root.showCurves
                    onClicked: root.showCurves = true
                }
                C.ToolButton {
                    text: "Timing tools"
                    active: root.showTimingTools
                    visible: !root.showCurves
                    onClicked: root.showTimingTools = !root.showTimingTools
                }
                C.ToolButton {
                    text: "Keys"
                    visible: !root.showCurves
                    active: root.keyEditing
                    hint: "Key mode: drag diamonds to move poses; double-click an empty cell to add a key. Turn off to select exposure ranges."
                    onClicked: root.keyEditing = !root.keyEditing
                }
                C.ToolButton {
                    text: "+ Key"
                    visible: !root.showCurves
                    hint: "Add pose key at the current frame"
                    onClicked: editor.addKey()
                }
                C.ToolButton {
                    text: "−"
                    visible: !root.showCurves && !root.xsheet
                    hint: "Narrow timeline frames"
                    enabled: root.timelineCell > 12
                    onClicked: {
                        root.timelineCell = Math.max(12, root.timelineCell - 10);
                        timeline.requestPaint();
                    }
                }
                C.ToolButton {
                    text: "+"
                    visible: !root.showCurves && !root.xsheet
                    hint: "Widen timeline frames for easier key placement"
                    enabled: root.timelineCell < 72
                    onClicked: {
                        root.timelineCell = Math.min(72, root.timelineCell + 10);
                        timeline.requestPaint();
                    }
                }
                Item {
                    Layout.fillWidth: true
                }
                C.ToolButton {
                    text: "|‹"
                    hint: "First frame"
                    onClicked: editor.frame = 0
                }
                C.ToolButton {
                    text: "‹"
                    hint: "Previous drawing"
                    onClicked: editor.nextDrawing(-1)
                }
                C.ToolButton {
                    text: editor.playing ? "Ⅱ" : "▶"
                    active: editor.playing
                    hint: "Play / pause · Space"
                    onClicked: editor.togglePlayback()
                }
                C.ToolButton {
                    text: "›"
                    hint: "Next drawing"
                    onClicked: editor.nextDrawing(1)
                }
                Text {
                    text: String(editor.frame + 1).padStart(4, "0")
                    color: "#eeeeee"
                    font.family: "Menlo"
                    font.pixelSize: 13
                    Layout.preferredWidth: 45
                }
                Text {
                    text: "/ " + editor.duration
                    color: "#757575"
                    font.family: "Menlo"
                    font.pixelSize: 11
                }
                Item {
                    Layout.fillWidth: true
                }
                C.ToolButton {
                    text: "+ 2 frames"
                    onClicked: editor.insertFrames(2)
                }
                C.ToolButton {
                    text: "Settings"
                    onClicked: settingsDialog.open()
                }
            }
        }
        C.TimingTools {
            id: timingTools
            visible: root.showTimingTools && !root.showCurves
            controller: editor
            Layout.fillWidth: true
            Layout.leftMargin: 10
            Layout.rightMargin: 10
            Layout.preferredHeight: childrenRect.height
        }
        C.CurveEditor {
            id: curveEditor
            Layout.minimumHeight: root.effectiveBottomHeight
            Layout.maximumHeight: root.effectiveBottomHeight
            visible: root.showCurves
            controller: editor
            Layout.fillWidth: true
            Layout.preferredHeight: root.effectiveBottomHeight
        }
        RowLayout {
            Layout.fillWidth: true
            visible: !root.showCurves
            Layout.preferredHeight: root.effectiveBottomHeight
            Layout.minimumHeight: root.effectiveBottomHeight
            Layout.maximumHeight: root.effectiveBottomHeight
            Layout.fillHeight: false
            spacing: 0
            Rectangle {
                Layout.preferredWidth: 250
                Layout.fillHeight: true
                color: "#101010"
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 30
                        Layout.leftMargin: 12
                        Layout.rightMargin: 6
                        Text {
                            text: "LAYERS"
                            color: "#797979"
                            font.pixelSize: 10
                            font.letterSpacing: 1
                        }
                        Item {
                            Layout.fillWidth: true
                        }
                        C.ToolButton {
                            text: "+"
                            hint: "Add layer"
                            onClicked: editor.addLayer()
                        }
                        C.ToolButton {
                            text: "↑"
                            hint: "Move layer up"
                            onClicked: editor.moveLayer(1)
                        }
                        C.ToolButton {
                            text: "↓"
                            hint: "Move layer down"
                            onClicked: editor.moveLayer(-1)
                        }
                        C.ToolButton {
                            text: "⋯"
                            hint: "Layer actions"
                            onClicked: layerMenu.open()
                        }
                        Menu {
                            id: layerMenu
                            Action {
                                text: "Duplicate layer"
                                onTriggered: editor.duplicateLayer(false)
                            }
                            Action {
                                text: "Clone linked drawings"
                                onTriggered: editor.duplicateLayer(true)
                            }
                            Action {
                                text: "Remove layer"
                                onTriggered: editor.removeLayer()
                            }
                        }
                    }
                    ListView {
                        Layout.fillHeight: true
                        Layout.fillWidth: true
                        clip: true
                        model: editor.layers
                        delegate: Rectangle {
                            required property var modelData
                            width: ListView.view.width
                            height: root.timelineRow
                            color: editor.selectedLayer === modelData.id ? "#292929" : "transparent"
                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    canvas.clearRegion();
                                    root.inspectorMode = "layer";
                                    editor.selectedLayer = modelData.id;
                                }
                            }
                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 4
                                anchors.rightMargin: 6
                                spacing: 2
                                C.ToolButton {
                                    text: modelData.visible ? "◉" : "○"
                                    implicitWidth: 24
                                    hint: "Toggle visibility"
                                    onClicked: editor.toggleLayer(modelData.id, "visible")
                                }
                                C.ToolButton {
                                    text: modelData.locked ? "L" : "·"
                                    implicitWidth: 24
                                    hint: "Toggle lock"
                                    onClicked: editor.toggleLayer(modelData.id, "locked")
                                }
                                C.CompactTextField {
                                    text: modelData.name
                                    Layout.fillWidth: true
                                    background: null
                                    selectByMouse: true
                                    font.pixelSize: 11
                                    onEditingFinished: editor.renameLayer(modelData.id, text)
                                    onActiveFocusChanged: if (activeFocus) {
                                        canvas.clearRegion();
                                        root.inspectorMode = "layer";
                                        editor.selectedLayer = modelData.id;
                                    }
                                    Accessible.name: "Layer name"
                                }
                                C.ToolButton {
                                    text: "S"
                                    active: modelData.solo
                                    implicitWidth: 24
                                    hint: "Solo layer"
                                    onClicked: editor.toggleLayer(modelData.id, "solo")
                                }
                            }
                        }
                    }
                }
            }
            Rectangle {
                width: 1
                Layout.fillHeight: true
                color: "#333333"
            }
            Flickable {
                id: timelineScroll
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                contentWidth: root.xsheet ? Math.max(width, editor.layers.length * 100 + 48) : Math.max(width, editor.duration * root.timelineCell)
                contentHeight: root.xsheet ? editor.duration * root.timelineRow + 30 : Math.max(height, editor.layers.length * root.timelineRow + 30)
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.horizontal: ScrollBar {}
                ScrollBar.vertical: ScrollBar {}
                onContentXChanged: timeline.requestPaint()
                onContentYChanged: timeline.requestPaint()
                Canvas {
                    id: timeline
                    objectName: "timelineCanvas"
                    x: timelineScroll.contentX
                    y: timelineScroll.contentY
                    width: timelineScroll.width
                    height: timelineScroll.height
                    onWidthChanged: requestPaint()
                    onHeightChanged: requestPaint()
                    function keyPosition(frame, row) {
                        return root.xsheet ? Qt.point(48 + row * 100 + 80 - timelineScroll.contentX, 30 + (frame + .5) * root.timelineRow - timelineScroll.contentY) : Qt.point((frame + .5) * root.timelineCell - timelineScroll.contentX, 30 + (row + .5) * root.timelineRow - timelineScroll.contentY);
                    }
                    function diamond(ctx, p, filled) {
                        ctx.beginPath();
                        ctx.moveTo(p.x, p.y - 6);
                        ctx.lineTo(p.x + 6, p.y);
                        ctx.lineTo(p.x, p.y + 6);
                        ctx.lineTo(p.x - 6, p.y);
                        ctx.closePath();
                        if (filled)
                            ctx.fill();
                        else
                            ctx.stroke();
                    }
                    onPaint: {
                        const ctx = getContext("2d");
                        ctx.reset();
                        ctx.fillStyle = "#141414";
                        ctx.fillRect(0, 0, width, height);
                        const data = editor.layers;
                        const ox = timelineScroll.contentX;
                        const oy = timelineScroll.contentY;
                        ctx.font = "10px Menlo";
                        ctx.textBaseline = "middle";
                        for (let selected = 0; selected < data.length; selected++) {
                            if (editor.selectedLayers.indexOf(data[selected].id) < 0)
                                continue;
                            const a = timelineInput.moving ? timelineInput.previewFrame : editor.rangeStart;
                            const b = timelineInput.resizing ? timelineInput.previewEnd : a + editor.rangeEnd - editor.rangeStart;
                            ctx.fillStyle = timelineInput.moving ? "#555555" : "#303030";
                            if (root.xsheet)
                                ctx.fillRect(48 + selected * 100 - ox, 30 + a * root.timelineRow - oy, 100, (b - a) * root.timelineRow);
                            else
                                ctx.fillRect(a * root.timelineCell - ox, 30 + selected * root.timelineRow - oy, (b - a) * root.timelineCell, root.timelineRow);
                            ctx.fillStyle = "#eeeeee";
                            if (root.xsheet)
                                ctx.fillRect(50 + selected * 100 - ox, 30 + b * root.timelineRow - oy - 3, 96, 3);
                            else
                                ctx.fillRect(b * root.timelineCell - ox - 3, 32 + selected * root.timelineRow - oy, 3, root.timelineRow - 4);
                        }

                        if (!root.xsheet) {
                            const first = Math.max(0, Math.floor(ox / root.timelineCell));
                            const last = Math.min(editor.duration, Math.ceil((ox + width) / root.timelineCell));
                            for (let f = first; f < last; f++) {
                                const x = f * root.timelineCell - ox;
                                ctx.strokeStyle = "#222222";
                                ctx.beginPath();
                                ctx.moveTo(x, 0);
                                ctx.lineTo(x, height);
                                ctx.stroke();
                                if (f % 5 === 0 || f === 0) {
                                    ctx.fillStyle = "#888888";
                                    ctx.fillText(String(f + 1), x + 4, 14 - oy);
                                }
                            }
                            for (let r = 0; r < data.length; r++) {
                                const y = 30 + r * root.timelineRow - oy;
                                if (y + root.timelineRow < 0 || y > height)
                                    continue;
                                ctx.strokeStyle = "#262626";
                                ctx.beginPath();
                                ctx.moveTo(0, y + root.timelineRow);
                                ctx.lineTo(width, y + root.timelineRow);
                                ctx.stroke();
                                const spans = data[r].spans;
                                for (let n = 0; n < spans.length; n++) {
                                    const e = spans[n];
                                    const x = e.start * root.timelineCell - ox;
                                    const w = (e.end - e.start) * root.timelineCell;
                                    if (x + w < 0 || x > width)
                                        continue;
                                    ctx.fillStyle = data[r].id === editor.selectedLayer ? "#777777" : "#414141";
                                    ctx.fillRect(x + 1, y + 7, w - 2, 20);
                                    ctx.fillStyle = "#e2e2e2";
                                    ctx.beginPath();
                                    ctx.arc(x + 8, y + 17, 2.5, 0, Math.PI * 2);
                                    ctx.fill();
                                }
                            }
                            for (let m = 0; m < editor.markers.length; ++m) {
                                const marker = editor.markers[m];
                                ctx.fillStyle = "#cccccc";
                                ctx.fillText("▼ " + marker.name, marker.frame * root.timelineCell - ox + 3, 24 - oy);
                            }
                            const px = editor.frame * root.timelineCell - ox;
                            ctx.fillStyle = "#ededed";
                            ctx.fillRect(px, 0, 2, height);
                            ctx.fillRect(px, 0, root.timelineCell, 3);
                        } else {
                            const first = Math.max(0, Math.floor((oy - 30) / root.timelineRow));
                            const last = Math.min(editor.duration, Math.ceil((oy + height) / root.timelineRow));
                            for (let f = first; f < last; f++) {
                                const y = 30 + f * root.timelineRow - oy;
                                ctx.fillStyle = f === editor.frame ? "#343434" : "#141414";
                                ctx.fillRect(0, y, Math.max(0, 48 - ox), root.timelineRow);
                                ctx.fillStyle = "#999999";
                                ctx.fillText(String(f + 1), 6, y + 17);
                                for (let r = 0; r < data.length; r++) {
                                    const x = 48 + r * 100 - ox;
                                    ctx.strokeStyle = "#333333";
                                    ctx.strokeRect(x, y, 100, root.timelineRow);
                                    for (let n = 0; n < data[r].spans.length; n++) {
                                        const e = data[r].spans[n];
                                        if (f >= e.start && f < e.end) {
                                            ctx.fillStyle = "#cccccc";
                                            ctx.fillText(f === e.start ? String(e.drawing) : "│", x + 40, y + 17);
                                            break;
                                        }
                                    }
                                }
                            }
                            ctx.fillStyle = "#222222";
                            ctx.fillRect(0, -oy, width, 30);
                            ctx.fillStyle = "#aaaaaa";
                            for (let r = 0; r < data.length; r++)
                                ctx.fillText(data[r].name.substring(0, 12), 54 + r * 100 - ox, 15 - oy);
                        }
                        ctx.fillStyle = "#eeeeee";
                        ctx.strokeStyle = "#ffffff";
                        for (let r = 0; r < data.length; ++r)
                            for (const frame of data[r].keys) {
                                const p = keyPosition(frame, r);
                                if (p.x >= 0 && p.x <= width && p.y >= 0 && p.y <= height)
                                    diamond(ctx, p, true);
                            }
                        if (timelineInput.keySource >= 0)
                            diamond(ctx, keyPosition(timelineInput.previewFrame, timelineInput.anchorRow), false);
                    }
                    MouseArea {
                        id: timelineInput
                        hoverEnabled: true
                        property bool overRangeEnd: {
                            const row = rowAt(Qt.point(mouseX, mouseY));
                            const edge = root.xsheet ? 30 + editor.rangeEnd * root.timelineRow - timelineScroll.contentY : editor.rangeEnd * root.timelineCell - timelineScroll.contentX;
                            return !root.keyEditing && row >= 0 && row < editor.layers.length && editor.selectedLayers.indexOf(editor.layers[row].id) >= 0 && Math.abs((root.xsheet ? mouseY : mouseX) - edge) <= 5;
                        }
                        cursorShape: keySource >= 0 ? Qt.ClosedHandCursor : resizing || overRangeEnd ? (root.xsheet ? Qt.SizeVerCursor : Qt.SizeHorCursor) : keyAt(Qt.point(mouseX, mouseY)) >= 0 ? Qt.OpenHandCursor : pressed && moving ? Qt.ClosedHandCursor : Qt.CrossCursor
                        preventStealing: true
                        anchors.fill: parent
                        property int keySource: -1
                        property bool canceled: false
                        function keyAt(mouse) {
                            const row = rowAt(mouse);
                            if (row < 0 || row >= editor.layers.length)
                                return -1;
                            let nearest = -1, distance = 12;
                            for (const frame of editor.layers[row].keys) {
                                const p = timeline.keyPosition(frame, row), d = Math.hypot(p.x - mouse.x, p.y - mouse.y);
                                if (d < distance) {
                                    nearest = frame;
                                    distance = d;
                                }
                            }
                            return nearest;
                        }
                        function cancel() {
                            canceled = true;
                            keySource = -1;
                            resizing = false;
                            moving = false;
                            previewFrame = -1;
                            timeline.requestPaint();
                        }
                        property int anchorFrame: 0
                        property int anchorRow: -1
                        property bool moving: false
                        property bool resizing: false
                        property int previewEnd: 0
                        property int previewFrame: -1
                        function frameAt(mouse) {
                            return root.xsheet ? Math.floor((mouse.y + timelineScroll.contentY - 30) / root.timelineRow) : Math.floor((mouse.x + timelineScroll.contentX) / root.timelineCell);
                        }
                        function rowAt(mouse) {
                            return root.xsheet ? Math.floor((mouse.x + timelineScroll.contentX - 48) / 100) : Math.floor((mouse.y + timelineScroll.contentY - 30) / root.timelineRow);
                        }
                        onPressed: function (mouse) {
                            timeline.forceActiveFocus();
                            canceled = false;
                            anchorFrame = frameAt(mouse);
                            anchorRow = rowAt(mouse);
                            const edge = root.xsheet ? 30 + editor.rangeEnd * root.timelineRow - timelineScroll.contentY : editor.rangeEnd * root.timelineCell - timelineScroll.contentX;
                            const coordinate = root.xsheet ? mouse.y : mouse.x;
                            resizing = !root.keyEditing && anchorRow >= 0 && anchorRow < editor.layers.length && editor.selectedLayers.indexOf(editor.layers[anchorRow].id) >= 0 && Math.abs(coordinate - edge) <= 5;
                            previewEnd = editor.rangeEnd;
                            moving = !resizing && (mouse.modifiers & Qt.AltModifier) && anchorRow >= 0 && anchorRow < editor.layers.length && anchorFrame >= editor.rangeStart && anchorFrame < editor.rangeEnd && editor.selectedLayers.indexOf(editor.layers[anchorRow].id) >= 0;
                            if (resizing) {
                                timeline.requestPaint();
                                return;
                            }
                            const key = keyAt(mouse);
                            if (!moving && key >= 0) {
                                editor.selectedLayer = editor.layers[anchorRow].id;
                                editor.frame = key;
                                canceled = false;
                                keySource = key;
                                previewFrame = key;
                                timeline.requestPaint();
                                return;
                            }
                            if (root.keyEditing && !moving && anchorRow >= 0 && anchorRow < editor.layers.length) {
                                editor.selectedLayer = editor.layers[anchorRow].id;
                                editor.frame = anchorFrame;
                                canceled = false;
                                return;
                            }
                            if (moving)
                                previewFrame = editor.rangeStart;
                            else if (anchorRow >= 0 && anchorRow < editor.layers.length)
                                editor.selectTimelineRange(anchorFrame, anchorFrame, anchorRow, anchorRow);
                            else
                                editor.frame = anchorFrame;
                        }
                        onPositionChanged: function (mouse) {
                            if (!pressed || canceled)
                                return;
                            if (keySource >= 0) {
                                previewFrame = Math.max(0, Math.min(editor.duration - 1, keySource + frameAt(mouse) - anchorFrame));
                                timeline.requestPaint();
                                return;
                            }
                            if (root.keyEditing && !moving) {
                                editor.frame = frameAt(mouse);
                                return;
                            }
                            if (resizing) {
                                previewEnd = Math.max(editor.rangeStart + 1, frameAt(mouse) + 1);
                                timeline.requestPaint();
                            } else if (moving) {
                                previewFrame = Math.max(0, Math.min(editor.duration - 1, editor.rangeStart + frameAt(mouse) - anchorFrame));
                                editor.report("Move preview: replace frames " + (previewFrame + 1) + "–" + (previewFrame + editor.rangeEnd - editor.rangeStart) + ". Release to overwrite; Escape cancels.");
                                timeline.requestPaint();
                            } else if (anchorRow >= 0)
                                editor.selectTimelineRange(anchorFrame, frameAt(mouse), anchorRow, rowAt(mouse));
                            else
                                editor.frame = frameAt(mouse);
                        }
                        onReleased: {
                            if (canceled)
                                return;
                            if (keySource >= 0) {
                                const source = keySource, destination = previewFrame;
                                cancel();
                                if (source !== destination)
                                    editor.movePoseKey(source, destination);
                                return;
                            }
                            if (resizing)
                                editor.retimeTimelineRange(previewEnd - editor.rangeStart);
                            resizing = false;
                            if (moving && previewFrame >= 0)
                                editor.moveTimelineRange(previewFrame, false);
                            moving = false;
                            previewFrame = -1;
                            timeline.requestPaint();
                        }
                        onCanceled: cancel()
                        onDoubleClicked: mouse => {
                            if (root.keyEditing) {
                                const row = rowAt(mouse), frame = frameAt(mouse);
                                cancel();
                                if (row >= 0 && row < editor.layers.length && frame >= 0 && frame < editor.duration) {
                                    editor.selectedLayer = editor.layers[row].id;
                                    editor.frame = frame;
                                    editor.addKey();
                                }
                            } else
                                editor.newDrawing(false);
                        }
                        Accessible.name: "Timeline. Drag diamonds to retime poses. Enable Keys to add keys by double-clicking. Alt-drag moves an exposure range."
                    }
                    Keys.onEscapePressed: {
                        timelineInput.cancel();
                    }
                }
                Connections {
                    target: editor
                    function onChanged() {
                        if (timelineInput.keySource >= 0)
                            timelineInput.cancel();
                        timeline.requestPaint();
                    }
                    function onFrameChanged() {
                        timeline.requestPaint();
                    }
                    function onSelectionChanged() {
                        timeline.requestPaint();
                    }
                    function onRangeChanged() {
                        timeline.requestPaint();
                    }
                }
                Connections {
                    target: root
                    function onXsheetChanged() {
                        timelineScroll.contentX = 0;
                        timelineScroll.contentY = 0;
                        timeline.requestPaint();
                    }
                }
            }
        }
        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#333333"
        }
        Rectangle {
            id: statusBar
            Layout.fillWidth: true
            Layout.preferredHeight: 22
            Layout.minimumHeight: 22
            Layout.maximumHeight: 22
            color: "#0b0b0b"
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 14
                anchors.rightMargin: 10
                Text {
                    text: editor.status
                    color: "#999999"
                    font.pixelSize: 10
                    elide: Text.ElideMiddle
                    Layout.fillWidth: true
                }
                ProgressBar {
                    visible: editor.exporting
                    value: editor.exportProgress
                    Layout.preferredWidth: 150
                }
                C.ToolButton {
                    visible: editor.exporting
                    text: "Cancel export"
                    implicitHeight: 24
                    onClicked: editor.cancelExport()
                }
                Text {
                    text: "LOCAL / OFFLINE"
                    color: "#606060"
                    font.pixelSize: 9
                    font.letterSpacing: 1
                }
            }
        }
    }

    FileDialog {
        id: openDialog
        title: "Open OPEN-TOON project"
        nameFilters: ["OPEN-TOON projects (*.otoon)"]
        onAccepted: editor.openProject(selectedFile)
    }
    FileDialog {
        id: saveDialog
        title: "Save OPEN-TOON project"
        fileMode: FileDialog.SaveFile
        defaultSuffix: "otoon"
        nameFilters: ["OPEN-TOON projects (*.otoon)"]
        onAccepted: editor.saveProject(selectedFile)
    }
    FileDialog {
        id: imageDialog
        title: "Import image"
        nameFilters: ["Images (*.png *.jpg *.jpeg *.bmp *.webp)"]
        onAccepted: editor.importImage(selectedFile)
    }
    FolderDialog {
        id: exportDialog
        title: "Choose a folder for a new PNG sequence export"
        onAccepted: editor.exportFrames(selectedFolder)
    }
    ColorDialog {
        id: colorDialog
        title: root.colorEditId ? "Edit linked palette color" : "Add palette color"
        options: ColorDialog.ShowAlphaChannel
        onAccepted: {
            if (root.colorEditId)
                editor.setSwatchColor(root.colorEditId, selectedColor);
            else
                editor.addSwatch(selectedColor);
        }
    }
    Dialog {
        id: discardDialog
        title: "Unsaved changes"
        anchors.centerIn: parent
        modal: true
        width: 400
        standardButtons: Dialog.Discard | Dialog.Cancel
        Label {
            width: parent.width
            text: "This scene has unsaved changes. Discard them and continue? Use Save first if you want to keep them."
            wrapMode: Text.WordWrap
        }
        onDiscarded: root.performAction(root.pendingAction)
    }
    Dialog {
        id: settingsDialog
        title: "Scene settings"
        anchors.centerIn: parent
        modal: true
        width: 420
        standardButtons: Dialog.Ok | Dialog.Cancel
        onOpened: {
            sceneName.text = editor.sceneName;
            sceneW.text = editor.sceneWidth;
            sceneH.text = editor.sceneHeight;
            sceneDuration.text = editor.duration;
            rateN.text = editor.fpsNumerator;
            rateD.text = editor.fpsDenominator;
        }
        ColumnLayout {
            anchors.fill: parent
            spacing: 10
            Label {
                text: "Scene name"
            }
            C.CompactTextField {
                id: sceneName
                Layout.fillWidth: true
            }
            RowLayout {
                C.CompactTextField {
                    id: sceneW
                    placeholderText: "Width"
                    Layout.fillWidth: true
                    validator: IntValidator {
                        bottom: 1
                        top: 8192
                    }
                }
                Label {
                    text: "×"
                }
                C.CompactTextField {
                    id: sceneH
                    placeholderText: "Height"
                    Layout.fillWidth: true
                    validator: IntValidator {
                        bottom: 1
                        top: 8192
                    }
                }
            }
            Label {
                text: "Duration in frames (use Remove Frames to shorten)"
                font.pixelSize: 10
            }
            C.CompactTextField {
                id: sceneDuration
                Layout.fillWidth: true
                validator: IntValidator {
                    bottom: 1
                    top: 1000000
                }
            }
            Label {
                text: "Frame rate: numerator / denominator"
                font.pixelSize: 10
            }
            RowLayout {
                C.CompactTextField {
                    id: rateN
                    Layout.fillWidth: true
                    validator: IntValidator {
                        bottom: 1
                        top: 240000
                    }
                }
                Label {
                    text: "/"
                }
                C.CompactTextField {
                    id: rateD
                    Layout.fillWidth: true
                    validator: IntValidator {
                        bottom: 1
                        top: 10000
                    }
                }
            }
            Label {
                text: "Changing the rate preserves frame count and changes duration in seconds. The operation can be undone."
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                color: "#999999"
                font.pixelSize: 11
            }
        }
        onAccepted: editor.setScene(sceneName.text, Number(sceneW.text), Number(sceneH.text), Number(sceneDuration.text), Number(rateN.text), Number(rateD.text))
    }
    Dialog {
        id: compactDialog
        title: "Compact project history"
        anchors.centerIn: parent
        width: 470
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel
        ColumnLayout {
            anchors.fill: parent
            Label {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                text: "Keep the newest revisions and remove unused media. A complete backup of the original history will be created beside the project before any revisions are removed."
            }
            Label {
                text: "Revisions to retain"
            }
            C.CompactSpinBox {
                id: retainedRevisions
                from: 1
                to: 1000
                value: 20
                editable: true
                Accessible.name: "Revisions to retain"
            }
        }
        onAccepted: editor.compactProject(retainedRevisions.value)
    }
    Dialog {
        id: historyDialog
        title: "Saved revisions"
        anchors.centerIn: parent
        width: 480
        height: 360
        modal: true
        standardButtons: Dialog.Close
        ListView {
            anchors.fill: parent
            clip: true
            model: editor.revisions
            delegate: ItemDelegate {
                required property var modelData
                width: ListView.view.width
                text: "#" + modelData.id + "  " + modelData.created + "  " + modelData.label
                onClicked: {
                    editor.restoreRevision(modelData.id);
                    historyDialog.close();
                }
            }
            Label {
                anchors.centerIn: parent
                visible: parent.count === 0
                text: "Save the project to begin its revision history."
                color: "#999999"
            }
        }
    }
    Dialog {
        id: helpDialog
        title: "Drawing controls"
        anchors.centerIn: parent
        width: 490
        modal: true
        standardButtons: Dialog.Close
        Label {
            width: parent.width
            wrapMode: Text.WordWrap
            text: "B — Pencil\nE — Eraser\nV — Select and move a stroke\nA — Animate layer poses on canvas\nM — Rectangular vector/raster selection\nO — Onion skin\nF — Fit canvas\nSpace — Play / pause\nLeft / Right — Previous / next frame\nCmd/Ctrl+Z — Undo\n\nDraw with the left mouse button. Pan with the middle button or trackpad scroll. Ctrl+scroll zooms. Double-click a timeline cell to create a new drawing.\n\nTablets use pressure when available; physical tablet validation is pending. This is an experimental build, not the P11 release."
        }
    }
    Dialog {
        id: recoveryDialog
        title: "Recover previous work?"
        anchors.centerIn: parent
        modal: true
        width: 420
        standardButtons: Dialog.Yes | Dialog.No
        Label {
            width: parent.width
            wrapMode: Text.WordWrap
            text: "A recovery snapshot from an earlier session is available. Open it as an unsaved scene?"
        }
        onAccepted: editor.recover()
    }
    Component.onCompleted: if (editor.recoveryPath.length)
        recoveryDialog.open()
}
