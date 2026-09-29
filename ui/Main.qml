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
    property bool showNodes: false
    property bool showTimingTools: false
    property int meshBindColumns: 4
    property int meshBindRows: 4
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
    readonly property var activePublishedViews: editor.characterViews.filter(v =>
        v.published && v.controlGroup === editor.selectedControlGroup)
    readonly property var activePublishedPoses: editor.characterPoses.filter(p =>
        p.published && p.controlGroup === editor.selectedControlGroup)
    readonly property var activePublishedDrawings: editor.publishedCharacterSubstitutions.filter(d =>
        d.group === editor.selectedControlGroup)
    readonly property bool drawingSelectionTool: ["Select", "Marquee", "Lasso"].includes(editor.tool)
    readonly property bool canvasEditingFocused: canvas.activeFocus && drawingSelectionTool
    property bool xsheet: false
    readonly property bool keyWorkspaceFocused: (showCurves && curveEditor.activeFocus) || (!showCurves && !showNodes && keyEditing && timeline.activeFocus)
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
        enabled: !root.textEditing && (root.keyWorkspaceFocused || timeline.activeFocus || root.canvasEditingFocused)
        onActivated: root.canvasEditingFocused ? canvas.copyVectorSelection() : root.keyWorkspaceFocused ? editor.copyPoseKeys() : editor.copyTimelineRange()
    }
    Shortcut {
        sequences: [StandardKey.Paste]
        enabled: !root.textEditing && (root.keyWorkspaceFocused || timeline.activeFocus || root.canvasEditingFocused)
        onActivated: root.canvasEditingFocused ? canvas.pasteVectorSelection() : root.keyWorkspaceFocused ? editor.pastePoseKeys() : editor.pasteTimelineRange(0, false)
    }
    Shortcut {
        sequences: [StandardKey.SelectAll]
        enabled: !root.textEditing && (root.keyWorkspaceFocused || root.canvasEditingFocused)
        onActivated: root.canvasEditingFocused ? canvas.selectAllVectors() : editor.selectPoseRange(0, editor.duration - 1)
    }
    Shortcut {
        sequences: [StandardKey.Cut]
        enabled: !root.textEditing && root.canvasEditingFocused
        onActivated: canvas.copyVectorSelection(true)
    }
    Shortcut { sequence: "L"; enabled: !root.textEditing; onActivated: editor.tool = "Lasso" }
    Shortcut { sequence: "Shift+Right"; enabled: root.canvasEditingFocused && !root.textEditing; onActivated: canvas.nudgeVectorSelection(10, 0) }
    Shortcut { sequence: "Shift+Left"; enabled: root.canvasEditingFocused && !root.textEditing; onActivated: canvas.nudgeVectorSelection(-10, 0) }
    Shortcut { sequence: "Up"; enabled: root.canvasEditingFocused && !root.textEditing; onActivated: canvas.nudgeVectorSelection(0, -1) }
    Shortcut { sequence: "Down"; enabled: root.canvasEditingFocused && !root.textEditing; onActivated: canvas.nudgeVectorSelection(0, 1) }
    Shortcut { sequence: "Shift+Up"; enabled: root.canvasEditingFocused && !root.textEditing; onActivated: canvas.nudgeVectorSelection(0, -10) }
    Shortcut { sequence: "Shift+Down"; enabled: root.canvasEditingFocused && !root.textEditing; onActivated: canvas.nudgeVectorSelection(0, 10) }
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
        onActivated: root.canvasEditingFocused ? canvas.nudgeVectorSelection(1, 0) : root.keyWorkspaceFocused && editor.selectedPoseFrames.length ? editor.moveSelectedPoseKeys(1) : editor.frame++
    }
    Shortcut {
        sequence: "Left"
        enabled: !root.textEditing
        onActivated: root.canvasEditingFocused ? canvas.nudgeVectorSelection(-1, 0) : root.keyWorkspaceFocused && editor.selectedPoseFrames.length ? editor.moveSelectedPoseKeys(-1) : editor.frame--
    }
    Shortcut {
        sequence: "F"
        enabled: !root.textEditing
        onActivated: canvas.fit()
    }
    Shortcut {
        sequences: ["Delete", "Backspace"]
        enabled: !root.textEditing
        onActivated: {
            if (root.keyWorkspaceFocused) {
                if (editor.selectedPoseFrames.length)
                    editor.deleteSelectedPoseKeys();
                else
                    editor.deleteKey();
            } else
                canvas.deleteSelection();
        }
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
                text: "Import registered PNG parts…"
                onTriggered: partsDialog.open()
            }
            Action {
                text: "Import PNG sequence…"
                onTriggered: sequenceDialog.open()
            }
            Action {
                text: "Import PCM16 WAV…"
                onTriggered: audioDialog.open()
            }
            Action {
                text: editor.activeCamera ? "Edit output camera" : "Add output camera"
                onTriggered: { editor.addCamera(); root.inspectorMode = "layer"; canvas.clearRegion(); }
            }
            Action {
                text: "Export PNG sequence…"
                enabled: !editor.exporting
                onTriggered: exportDialog.open()
            }
            Action {
                text: "Export PCM WAV mix…"
                enabled: !editor.exporting
                onTriggered: audioExportDialog.open()
            }
            Action {
                text: "Export selected PCM WAV range…"
                enabled: !editor.exporting && editor.rangeEnd > editor.rangeStart
                onTriggered: audioRangeExportDialog.open()
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
            Menu {
                title: "Composition"
                Action {
                    text: "Legacy appearance"
                    checkable: true
                    checked: editor.compositionProfile === 0
                    onTriggered: editor.setCompositionProfile(0)
                }
                Action {
                    text: "Linear sRGB"
                    checkable: true
                    checked: editor.compositionProfile === 1
                    onTriggered: editor.setCompositionProfile(1)
                }
            }
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
                C.CompactComboBox {
                    objectName: "workspacePicker"
                    model: ["Rig", "Animator"]
                    currentIndex: editor.workspaceMode === "Animator" ? 1 : 0
                    onActivated: editor.workspaceMode = currentText
                    Accessible.name: "Workspace"
                    implicitWidth: 112
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
                    visible: editor.tool !== "Marquee" && editor.tool !== "Lasso" && editor.tool !== "Select" && editor.tool !== "Animate" && editor.tool !== "Camera" && editor.tool !== "Mesh"
                    text: "Size"
                    color: "#858585"
                    font.pixelSize: 11
                }
                Slider {
                    visible: editor.tool !== "Marquee" && editor.tool !== "Lasso" && editor.tool !== "Select" && editor.tool !== "Animate" && editor.tool !== "Camera" && editor.tool !== "Mesh"
                    from: 0.5
                    to: 100
                    value: editor.brushSize
                    Layout.preferredWidth: 120
                    onMoved: editor.brushSize = value
                    Accessible.name: "Brush size"
                }
                Text {
                    visible: editor.tool !== "Marquee" && editor.tool !== "Lasso" && editor.tool !== "Select" && editor.tool !== "Animate" && editor.tool !== "Camera" && editor.tool !== "Mesh"
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
                    visible: editor.tool === "Camera"
                    text: "Drag frame to pan · corners to zoom · circle to rotate · Shift constrains"
                    color: "#bbbbbb"
                }
                Label {
                    visible: editor.tool === "Camera"
                    text: editor.cameraZoom.toFixed(2) + "×"
                    color: "#dddddd"
                }
                C.ToolButton {
                    visible: editor.tool === "Camera"
                    text: "Reset frame"
                    hint: "Return this camera key to full-scene framing"
                    onClicked: editor.resetCameraPose()
                }
                C.ToolButton {
                    visible: editor.activeCamera > 0
                    text: canvas.cameraGuidesVisible ? "Guides on" : "Guides"
                    hint: "Show non-exported safe-frame guides"
                    onClicked: canvas.cameraGuidesVisible = !canvas.cameraGuidesVisible
                }
                C.ToolButton {
                    text: "Guides ▾"
                    hint: "Drawing-local grid and optional snapping for pencil and primitives"
                    onClicked: gridMenu.open()
                    Menu {
                        id: gridMenu
                        MenuItem { text: "Show grid"; checkable: true; checked: canvas.gridVisible; onTriggered: canvas.gridVisible = !canvas.gridVisible }
                        MenuItem { text: "Snap pencil and shapes"; checkable: true; checked: canvas.snapToGrid; onTriggered: canvas.snapToGrid = !canvas.snapToGrid }
                        MenuSeparator {}
                        MenuItem { text: "Spacing · 10 px"; onTriggered: canvas.gridSpacing = 10 }
                        MenuItem { text: "Spacing · 20 px"; onTriggered: canvas.gridSpacing = 20 }
                        MenuItem { text: "Spacing · 40 px"; onTriggered: canvas.gridSpacing = 40 }
                        MenuItem { text: "Spacing · 80 px"; onTriggered: canvas.gridSpacing = 80 }
                    }
                }
                Label {
                    visible: ["Line", "Rectangle", "Ellipse"].includes(editor.tool)
                    text: "Shift constrains · " + (canvas.snapToGrid ? "Grid snap on" : "Free placement")
                    color: "#bbbbbb"
                }
                C.ToolButton {
                    objectName: "motionPathModeButton"
                    text: "Path"
                    visible: editor.tool === "Animate"
                    active: canvas.motionPathEditing
                    enabled: canvas.motionPathEditing || (canvas.hasRegion && editor.animationKeys.length > 0)
                    hint: "Drag pose keys on the canvas path. Shift constrains direction. Double-click to add a sampled pose; inserting a pose can change the timing curves between keys."
                    onClicked: canvas.motionPathEditing = !canvas.motionPathEditing
                }
                Label {
                    visible: editor.tool === "Animate"
                    text: canvas.motionPathEditing ? "Path · Drag keys · Double-click to add · Shift constrains · Escape cancels" : canvas.hasRegion ? "Animate layer · Drag to pose · Handles scale / rotate · Each gesture records a key" : "Animate layer · No artwork at this frame — extend the drawing exposure in the timeline"
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
                Label {
                    visible: editor.tool === "Select" || editor.tool === "Lasso" || (editor.tool === "Marquee" && canvas.selectionMedia === 0)
                    text: "Shift adds · Alt subtracts · Drag handles to transform"
                    color: "#bbbbbb"
                }
                C.ToolButton {
                    visible: root.drawingSelectionTool
                    text: "Edit ▾"
                    hint: "Vector clipboard and selection"
                    onClicked: vectorEditMenu.open()
                    Menu {
                        id: vectorEditMenu
                        MenuItem { text: "Select all vectors"; onTriggered: canvas.selectAllVectors() }
                        MenuItem { text: "Invert vector selection"; onTriggered: canvas.selectAllVectors(true) }
                        MenuSeparator {}
                        MenuItem { text: "Copy vectors"; enabled: !!canvas.objectProperties.vectorOnly && canvas.hasRegion; onTriggered: canvas.copyVectorSelection() }
                        MenuItem { text: "Cut vectors"; enabled: !!canvas.objectProperties.vectorOnly && canvas.hasRegion; onTriggered: canvas.copyVectorSelection(true) }
                        MenuItem { text: "Paste vectors in place"; enabled: canvas.hasVectorClipboard; onTriggered: canvas.pasteVectorSelection() }
                    }
                }
                C.ToolButton {
                    visible: root.drawingSelectionTool
                    text: "Duplicate"
                    enabled: canvas.hasRegion
                    onClicked: canvas.transformRegion(1, 24, 24)
                }
                C.ToolButton {
                    visible: root.drawingSelectionTool
                    text: "Flip H"
                    enabled: canvas.hasRegion
                    onClicked: canvas.transformRegion(3)
                }
                C.ToolButton {
                    visible: root.drawingSelectionTool
                    text: "Flip V"
                    enabled: canvas.hasRegion
                    onClicked: canvas.transformRegion(4)
                }
                C.ToolButton {
                    visible: root.drawingSelectionTool
                    text: "Deselect"
                    enabled: canvas.hasRegion
                    onClicked: canvas.clearRegion()
                }
                C.ToolButton {
                    text: "Fill shape"
                    visible: !editor.tool.startsWith("Raster ") && editor.tool !== "Marquee" && editor.tool !== "Lasso" && editor.tool !== "Select" && editor.tool !== "Animate" && editor.tool !== "Mesh"
                    active: editor.filled
                    onClicked: editor.filled = !editor.filled
                    hint: "Fill new rectangles and ellipses"
                }
                C.CompactComboBox {
                    visible: !editor.tool.startsWith("Raster ") && editor.tool !== "Marquee" && editor.tool !== "Lasso" && editor.tool !== "Select" && editor.tool !== "Animate" && editor.tool !== "Mesh"
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
                                    key: "V"
                                },
                                {
                                    name: "Animate",
                                    key: "A"
                                },
                                { name: "Camera", key: "" },
                                {
                                    name: "Marquee",
                                    key: "M"
                                },
                                { name: "Lasso", key: "L" },
                                { name: "Line", key: "" },
                                {
                                    name: "Pencil",
                                    key: "B"
                                },
                                {
                                    name: "Raster ink",
                                    key: ""
                                },
                                {
                                    name: "Eraser",
                                    key: "E"
                                },
                                {
                                    name: "Rectangle",
                                    key: ""
                                },
                                {
                                    name: "Ellipse",
                                    key: ""
                                },
                                {
                                    name: "Recolor",
                                    key: ""
                                },
                                {
                                    name: "Edit points",
                                    key: ""
                                }
                            ]
                            C.ToolButton {
                                required property var modelData
                                width: 34
                                height: 28
                                toolIcon: modelData.name
                                active: editor.tool === modelData.name
                                enabled: modelData.name !== "Camera" || editor.activeCamera > 0
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
                Rectangle {
                    id: canvasControls
                    objectName: "canvasAnimatorControls"
                    anchors.top: parent.top
                    anchors.right: parent.right
                    anchors.margins: 16
                    width: Math.min(254, parent.width - 32)
                    height: Math.min(canvasControlsContent.implicitHeight + 20,
                                     Math.max(120, parent.height - 32))
                    visible: editor.workspaceMode === "Animator" && editor.characterId > 0 &&
                             (root.activePublishedPoses.length > 0 || root.activePublishedDrawings.length > 0)
                    z: 2
                    radius: 6
                    color: "#ee151515"
                    border.color: "#555555"
                    ScrollView {
                        anchors.fill: parent
                        anchors.margins: 10
                        clip: true
                        contentWidth: availableWidth
                        ColumnLayout {
                            id: canvasControlsContent
                            width: 232
                            spacing: 6
                            Label {
                                text: "CHARACTER CONTROLS"
                                color: "#aaaaaa"
                                font.pixelSize: 10
                                font.letterSpacing: 1.1
                            }
                            C.CompactComboBox {
                                objectName: "canvasControlGroupPicker"
                                Layout.fillWidth: true
                                visible: editor.characterControlGroups.length > 1
                                model: editor.characterControlGroups
                                currentIndex: model.indexOf(editor.selectedControlGroup)
                                onActivated: editor.selectedControlGroup = currentText
                                Accessible.name: "Canvas control group"
                            }
                            C.CompactComboBox {
                                objectName: "canvasPosePicker"
                                Layout.fillWidth: true
                                visible: root.activePublishedPoses.length > 0
                                model: root.activePublishedPoses
                                textRole: "name"
                                valueRole: "id"
                                currentIndex: model.findIndex(p => p.id === editor.selectedCharacterPose)
                                onActivated: editor.selectCharacterPose(currentValue)
                                Accessible.name: "Canvas published pose"
                            }
                            RowLayout {
                                Layout.fillWidth: true
                                visible: root.activePublishedPoses.length > 0
                                C.CompactButton {
                                    text: "Apply"
                                    enabled: root.activePublishedPoses.some(p => p.id === editor.selectedCharacterPose)
                                    onClicked: editor.applySelectedCharacterPose()
                                    Accessible.name: "Apply canvas pose"
                                }
                                Slider {
                                    id: canvasPoseBlend
                                    objectName: "canvasPoseBlend"
                                    Layout.fillWidth: true
                                    implicitHeight: 24
                                    from: 0
                                    to: 1
                                    value: 0
                                    enabled: root.activePublishedPoses.some(p => p.id === editor.selectedCharacterPose)
                                    onPressedChanged: {
                                        if (pressed)
                                            editor.beginSelectedCharacterPoseBlend()
                                        else
                                            editor.endSelectedCharacterPoseBlend()
                                    }
                                    onMoved: editor.updateSelectedCharacterPoseBlend(value)
                                    Accessible.name: "Blend canvas character pose"
                                    background: Rectangle {
                                        x: canvasPoseBlend.leftPadding
                                        y: canvasPoseBlend.topPadding + canvasPoseBlend.availableHeight / 2 - height / 2
                                        width: canvasPoseBlend.availableWidth
                                        height: 3
                                        radius: 2
                                        color: "#303030"
                                        Rectangle {
                                            width: canvasPoseBlend.visualPosition * parent.width
                                            height: parent.height
                                            radius: parent.radius
                                            color: "#b8b8b8"
                                        }
                                    }
                                    handle: Rectangle {
                                        x: canvasPoseBlend.leftPadding + canvasPoseBlend.visualPosition *
                                           (canvasPoseBlend.availableWidth - width)
                                        y: canvasPoseBlend.topPadding + canvasPoseBlend.availableHeight / 2 - height / 2
                                        width: 12
                                        height: 12
                                        radius: 6
                                        color: "#e8e8e8"
                                        border.color: "#171717"
                                    }
                                }
                                Label {
                                    text: Math.round(canvasPoseBlend.value * 100) + "%"
                                    color: "#aaaaaa"
                                    font.pixelSize: 10
                                    Layout.preferredWidth: 32
                                }
                            }
                            Connections {
                                target: editor
                                function onPoseSelectionChanged() { canvasPoseBlend.value = 0 }
                            }
                            Repeater {
                                model: root.activePublishedDrawings
                                ColumnLayout {
                                    id: canvasDrawingGroup
                                    required property var modelData
                                    Layout.fillWidth: true
                                    spacing: 2
                                    Label {
                                        text: canvasDrawingGroup.modelData.name
                                        color: "#aaaaaa"
                                        font.pixelSize: 10
                                    }
                                    C.CompactComboBox {
                                        objectName: "canvasDrawingPicker"
                                        Layout.fillWidth: true
                                        model: canvasDrawingGroup.modelData.options
                                        textRole: "name"
                                        valueRole: "id"
                                        currentIndex: model.findIndex(v => v.id === canvasDrawingGroup.modelData.selected)
                                        onActivated: editor.applyPublishedSubstitution(canvasDrawingGroup.modelData.part,
                                                                                        currentValue)
                                        Accessible.name: "Canvas drawings for " + canvasDrawingGroup.modelData.name
                                    }
                                }
                            }
                        }
                    }
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
                            text: editor.workspaceMode === "Animator" ? "CHARACTER" : "PROPERTIES"
                            color: "#888888"
                            font.pixelSize: 10
                            font.letterSpacing: 1.4
                        }
                        Label {
                            visible: editor.workspaceMode === "Rig" && canvas.objectProperties.kind === "none" && root.inspectorMode !== "layer"
                            Layout.leftMargin: 16
                            Layout.rightMargin: 16
                            Layout.fillWidth: true
                            text: "Select an object on the canvas to edit its properties, or select a layer in the layer list."
                            wrapMode: Text.WordWrap
                            color: "#999999"
                        }
                        C.SelectionProperties {
                            visible: editor.workspaceMode === "Rig" && canvas.objectProperties.kind !== "none"
                            Layout.leftMargin: 16
                            Layout.rightMargin: 16
                            Layout.fillWidth: true
                            drawingCanvas: canvas
                            controller: editor
                        }
                        ColumnLayout {
                            id: layerInspector
                            visible: editor.workspaceMode === "Rig" && root.inspectorMode === "layer" && canvas.objectProperties.kind === "none"
                            Layout.fillWidth: true
                            property var rigLayer: editor.layers.find(l => l.id === editor.selectedLayer)
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
                                    enabled: editor.tool !== "Animate" && editor.tool !== "Camera"
                                    currentIndex: editor.animateMode ? 1 : 0
                                    onActivated: editor.animateMode = currentIndex === 1
                                    Accessible.name: "Animation edit mode"
                                    implicitWidth: 105
                                }
                                C.CompactCheckBox {
                                    text: editor.tool === "Animate" ? "Gesture keys" : "Auto key"
                                    checked: editor.tool === "Animate" || editor.tool === "Camera" || editor.autoKey
                                    enabled: editor.tool !== "Animate" && editor.tool !== "Camera" && editor.animateMode
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
                                    onClicked: {
                                        root.showNodes = false;
                                        root.showCurves = true;
                                    }
                                }
                                C.ToolButton {
                                    text: "Pose ▾"
                                    visible: layerInspector.rigLayer?.kind !== 4
                                    hint: "Copy, paste or reset the selected layer transform"
                                    onClicked: layerPoseMenu.open()
                                    Menu {
                                        id: layerPoseMenu
                                        MenuItem { text: "Copy pose"; onTriggered: editor.copyTransformPose() }
                                        MenuSeparator {}
                                        MenuItem { text: "Paste full pose"; enabled: editor.hasCopiedTransform; onTriggered: editor.pasteTransformPose(0) }
                                        MenuItem { text: "Paste position"; enabled: editor.hasCopiedTransform; onTriggered: editor.pasteTransformPose(1) }
                                        MenuItem { text: "Paste rotation"; enabled: editor.hasCopiedTransform; onTriggered: editor.pasteTransformPose(2) }
                                        MenuItem { text: "Paste scale"; enabled: editor.hasCopiedTransform; onTriggered: editor.pasteTransformPose(3) }
                                        MenuItem { text: "Paste opacity"; enabled: editor.hasCopiedTransform; onTriggered: editor.pasteTransformPose(4) }
                                        MenuItem { text: "Paste pivot"; enabled: editor.hasCopiedTransform; onTriggered: editor.pasteTransformPose(5) }
                                        MenuSeparator {}
                                        MenuItem { text: "Paste mirrored horizontally"; enabled: editor.hasCopiedTransform; onTriggered: editor.pasteTransformPose(6) }
                                        MenuItem { text: "Paste mirrored vertically"; enabled: editor.hasCopiedTransform; onTriggered: editor.pasteTransformPose(7) }
                                        MenuSeparator {}
                                        MenuItem { text: editor.animateMode ? "Reset to setup pose" : "Reset setup transform"; onTriggered: editor.resetTransformPose() }
                                    }
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
                                    model: layerInspector.rigLayer?.kind === 4 ? [
                                        { key: "x", name: "Center X · px" },
                                        { key: "y", name: "Center Y · px" },
                                        { key: "rotation", name: "Rotation °" },
                                        { key: "zoom", name: "Zoom · ratio" }
                                    ] : [
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
                                                return modelData.key === "zoom" ? editor.cameraZoom :
                                                    Number(editor.transform[modelData.key] || 0);
                                            }
                                            label: modelData.name
                                            onCommitted: value => modelData.key === "zoom" ?
                                                editor.setCameraZoom(value) : editor.setTransform(modelData.key, value)
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
                                enabled: layerInspector.rigLayer?.kind !== 1 && layerInspector.rigLayer?.kind !== 4
                                model: {
                                    const layers = editor.layers;
                                    const selected = layerInspector.rigLayer;
                                    if (!selected)
                                        return [];
                                    if (selected.kind !== 2 && selected.kind !== 3)
                                        return [{id: 0, name: "No parent"}].concat(layers.filter(l => l.id !== selected.id && l.kind !== 4));
                                    function ancestor(id) {
                                        let node = layers.find(l => l.id === id);
                                        let depth = 0;
                                        while (node && node.parent && depth++ < layers.length)
                                            node = layers.find(l => l.id === node.parent);
                                        return node?.id || 0;
                                    }
                                    function beneath(candidate, id) {
                                        let node = candidate;
                                        let depth = 0;
                                        while (node && node.parent && depth++ < layers.length) {
                                            if (node.parent === id)
                                                return true;
                                            node = layers.find(l => l.id === node.parent);
                                        }
                                        return false;
                                    }
                                    const rootId = ancestor(selected.id);
                                    return layers.filter(l => (l.kind === 1 || l.kind === 2 ||
                                                               (selected.kind === 3 && l.kind === 3)) &&
                                                           l.id !== selected.id && ancestor(l.id) === rootId &&
                                                           !beneath(l, selected.id));
                                }
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
                            C.CompactComboBox {
                                objectName: "cutterMattePicker"
                                Layout.leftMargin: 16
                                Layout.rightMargin: 16
                                Layout.fillWidth: true
                                implicitHeight: 30
                                visible: layerInspector.rigLayer?.kind === 0 || layerInspector.rigLayer?.kind === 3
                                enabled: visible && !layerInspector.rigLayer?.locked
                                model: {
                                    const selected = layerInspector.rigLayer;
                                    if (!selected)
                                        return [];
                                    return [{id: 0, name: "No cutter matte"}].concat(
                                        editor.layers.filter(l => (l.kind === 0 || l.kind === 3) &&
                                                             l.id !== selected.id && l.visible && l.matte === 0));
                                }
                                currentIndex: Math.max(0, model.findIndex(l => l.id === layerInspector.rigLayer?.matte))
                                textRole: "name"
                                valueRole: "id"
                                onActivated: editor.setLayerMatte(currentValue)
                                Accessible.name: "Cutter matte source"
                            }
                            C.CompactCheckBox {
                                Layout.leftMargin: 16
                                visible: layerInspector.rigLayer?.matte > 0
                                enabled: visible && !layerInspector.rigLayer?.locked
                                text: "Bypass cutter"
                                checked: layerInspector.rigLayer?.matteBypassed || false
                                onClicked: editor.setMatteBypassed(checked)
                                Accessible.name: "Bypass cutter matte"
                            }
                            C.CompactCheckBox {
                                Layout.leftMargin: 16
                                visible: layerInspector.rigLayer?.matte > 0
                                enabled: visible && !layerInspector.rigLayer?.locked
                                text: "Use outside of cutter"
                                checked: layerInspector.rigLayer?.invertMatte || false
                                onClicked: editor.setMatteInverted(checked)
                                Accessible.name: "Invert cutter matte"
                            }
                            C.CompactCheckBox {
                                objectName: "paintCutterSource"
                                Layout.leftMargin: 16
                                visible: layerInspector.rigLayer?.matte > 0
                                enabled: visible && !layerInspector.rigLayer?.locked &&
                                         !editor.layers.find(l => l.id === layerInspector.rigLayer?.matte)?.locked
                                text: "Paint cutter source"
                                checked: editor.layers.find(l => l.id === layerInspector.rigLayer?.matte)?.paintMatteSource || false
                                onClicked: editor.setMatteSourceVisible(checked)
                                Accessible.name: "Paint cutter source in composition"
                            }
                            RowLayout {
                                Layout.leftMargin: 12
                                Layout.rightMargin: 16
                                Layout.fillWidth: true
                                C.ToolButton {
                                    text: "Rig ▾"
                                    hint: "Character and peg actions"
                                    onClicked: rigMenu.open()
                                    Menu {
                                        id: rigMenu
                                        MenuItem {
                                            text: "Make character from layer"
                                            enabled: layerInspector.rigLayer?.kind === 0
                                            onTriggered: editor.makeCharacter()
                                        }
                                        MenuItem {
                                            text: "Attach unparented drawings"
                                            enabled: editor.characterId > 0
                                            onTriggered: editor.attachUnparentedDrawings()
                                        }
                                        MenuItem {
                                            text: "Add parent peg"
                                            enabled: layerInspector.rigLayer?.kind === 2 || layerInspector.rigLayer?.kind === 3
                                            onTriggered: editor.addPeg()
                                        }
                                        MenuItem {
                                            text: "Follow parent bone tip"
                                            enabled: editor.selectedCanFollowBoneTip
                                            checkable: true
                                            checked: editor.selectedFollowsBoneTip
                                            onTriggered: editor.toggleSelectedBoneTipAttachment()
                                        }
                                        MenuItem {
                                            text: "Center rest pivot on drawing"
                                            enabled: layerInspector.rigLayer?.kind === 0 || layerInspector.rigLayer?.kind === 3
                                            onTriggered: editor.centerRestPivot()
                                        }
                                        MenuItem {
                                            text: "Duplicate full character"
                                            enabled: editor.characterId > 0
                                            onTriggered: editor.duplicateCharacter()
                                        }
                                        MenuItem {
                                            text: "Detach selected part"
                                            enabled: layerInspector.rigLayer?.kind === 3
                                            onTriggered: editor.detachPart()
                                        }
                                        MenuItem {
                                            text: "Dissolve peg, keep children"
                                            enabled: layerInspector.rigLayer?.kind === 2
                                            onTriggered: editor.dissolvePeg()
                                        }
                                        MenuItem {
                                            text: "Delete selected rig branch"
                                            enabled: layerInspector.rigLayer?.kind === 2 || layerInspector.rigLayer?.kind === 3
                                            onTriggered: editor.deleteRigBranch()
                                        }
                                    }
                                }
                                Label {
                                    Layout.fillWidth: true
                                    text: ({0: "Drawing", 1: "Character", 2: "Peg", 3: "Part", 4: "Camera"})[layerInspector.rigLayer?.kind ?? 0]
                                    color: "#999999"
                                    font.pixelSize: 11
                                    horizontalAlignment: Text.AlignRight
                                }
                            }
                            ColumnLayout {
                                visible: layerInspector.rigLayer?.kind === 3
                                Layout.leftMargin: 16
                                Layout.rightMargin: 16
                                Layout.fillWidth: true
                                spacing: 6
                                Label { text: "Part role"; color: "#999999"; font.pixelSize: 10 }
                                C.CompactTextField {
                                    Layout.fillWidth: true
                                    text: layerInspector.rigLayer?.role || ""
                                    placeholderText: "e.g. Left hand"
                                    onEditingFinished: editor.setPartRole(text)
                                    Accessible.name: "Character part role"
                                }
                                Label { text: "Substitution at current frame"; color: "#999999"; font.pixelSize: 10 }
                                ListView {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 76
                                    clip: true
                                    orientation: ListView.Horizontal
                                    spacing: 4
                                    model: editor.substitutions
                                    delegate: Rectangle {
                                        required property var modelData
                                        width: 62
                                        height: 72
                                        radius: 3
                                        color: modelData.id === editor.selectedSubstitution ? "#363636" : "#181818"
                                        border.color: modelData.id === editor.selectedSubstitution ? "#dedede" : "#303030"
                                        Column {
                                            anchors.centerIn: parent
                                            spacing: 2
                                            Rectangle {
                                                width: 54
                                                height: 54
                                                color: "#eeeeee"
                                                Image {
                                                    anchors.fill: parent
                                                    anchors.margins: 1
                                                    fillMode: Image.PreserveAspectFit
                                                    source: {
                                                        const revision = editor.documentRevision;
                                                        return editor.substitutionThumbnail(modelData.id);
                                                    }
                                                    asynchronous: false
                                                }
                                            }
                                            Text {
                                                width: 54
                                                text: modelData.name
                                                color: "#dddddd"
                                                font.pixelSize: 9
                                                horizontalAlignment: Text.AlignHCenter
                                                elide: Text.ElideRight
                                            }
                                        }
                                        MouseArea {
                                            anchors.fill: parent
                                            onClicked: editor.selectSubstitution(modelData.id)
                                        }
                                        Accessible.name: "Substitution " + modelData.name
                                    }
                                }
                                RowLayout {
                                    Layout.fillWidth: true
                                    C.CompactButton { text: "+ Blank"; onClicked: editor.createSubstitution(false) }
                                    C.CompactButton { text: "Duplicate"; onClicked: editor.createSubstitution(true) }
                                    C.CompactButton {
                                        text: "Remove"
                                        enabled: editor.selectedSubstitution > 0
                                        onClicked: editor.removeSubstitution(editor.selectedSubstitution)
                                    }
                                }
                                RowLayout {
                                    Layout.fillWidth: true
                                    C.CompactButton {
                                        text: "‹"
                                        Accessible.name: "Previous substitution"
                                        ToolTip.text: "Choose previous substitution at the playhead"
                                        ToolTip.visible: hovered
                                        onClicked: editor.stepSubstitution(-1)
                                    }
                                    C.CompactButton {
                                        text: "›"
                                        Accessible.name: "Next substitution"
                                        ToolTip.text: "Choose next substitution at the playhead"
                                        ToolTip.visible: hovered
                                        onClicked: editor.stepSubstitution(1)
                                    }
                                    Item { Layout.fillWidth: true }
                                    C.CompactButton {
                                        text: "←"
                                        enabled: editor.selectedSubstitution > 0
                                        Accessible.name: "Move substitution earlier in gallery"
                                        ToolTip.text: "Move selected substitution earlier in the gallery"
                                        ToolTip.visible: hovered
                                        onClicked: editor.moveSubstitution(editor.selectedSubstitution, -1)
                                    }
                                    C.CompactButton {
                                        text: "→"
                                        enabled: editor.selectedSubstitution > 0
                                        Accessible.name: "Move substitution later in gallery"
                                        ToolTip.text: "Move selected substitution later in the gallery"
                                        ToolTip.visible: hovered
                                        onClicked: editor.moveSubstitution(editor.selectedSubstitution, 1)
                                    }
                                }
                                C.CompactTextField {
                                    Layout.fillWidth: true
                                    text: editor.substitutions.find(s => s.id === editor.selectedSubstitution)?.name || ""
                                    placeholderText: "Rename selected substitution"
                                    enabled: editor.selectedSubstitution > 0
                                    onEditingFinished: editor.renameSubstitution(editor.selectedSubstitution, text)
                                    Accessible.name: "Substitution name"
                                }
                                C.CompactCheckBox {
                                    text: "Show in Animator"
                                    checked: editor.substitutions.find(s => s.id === editor.selectedSubstitution)?.published || false
                                    enabled: editor.selectedSubstitution > 0
                                    onClicked: editor.setSelectedSubstitutionPublished(checked)
                                    Accessible.name: "Publish selected substitution"
                                }
                                C.CompactTextField {
                                    Layout.fillWidth: true
                                    text: editor.substitutions.find(s => s.id === editor.selectedSubstitution)?.controlGroup || "Main"
                                    placeholderText: "Control group"
                                    enabled: editor.selectedSubstitution > 0
                                    onEditingFinished: editor.setSelectedSubstitutionControlGroup(text)
                                    Accessible.name: "Drawing control group"
                                }
                                Label { text: "Drawing mesh · current substitution"; color: "#999999"; font.pixelSize: 10 }
                                RowLayout {
                                    Layout.fillWidth: true
                                    visible: !editor.selectedMeshBound
                                    Label { text: "Grid"; color: "#aaaaaa"; font.pixelSize: 10 }
                                    C.CompactSpinBox {
                                        id: meshColumnsInput
                                        Layout.preferredWidth: 62
                                        from: 1; to: 32
                                        value: root.meshBindColumns
                                        editable: true
                                        onValueModified: root.meshBindColumns = value
                                        Accessible.name: "Mesh columns, one to 32"
                                    }
                                    Label { text: "×"; color: "#aaaaaa"; font.pixelSize: 10 }
                                    C.CompactSpinBox {
                                        id: meshRowsInput
                                        Layout.preferredWidth: 62
                                        from: 1; to: 32
                                        value: root.meshBindRows
                                        editable: true
                                        onValueModified: root.meshBindRows = value
                                        Accessible.name: "Mesh rows, one to 32"
                                    }
                                    Item { Layout.fillWidth: true }
                                    C.CompactButton {
                                        text: "Bind"
                                        enabled: editor.selectedSubstitution > 0
                                        onClicked: { if (editor.bindSelectedMesh(root.meshBindColumns, root.meshBindRows)) { canvas.meshRestEditing = false; editor.tool = "Mesh"; } }
                                    }
                                    C.CompactButton {
                                        text: "Contour"
                                        visible: editor.substitutions.find(s => s.id === editor.selectedSubstitution)?.image === true
                                        Accessible.name: "Bind contour mesh to image substitution"
                                        ToolTip.text: "Fit the mesh rows to the image silhouette"
                                        ToolTip.visible: hovered
                                        onClicked: { if (editor.bindSelectedContourMesh(root.meshBindColumns, root.meshBindRows)) { canvas.meshRestEditing = false; editor.tool = "Mesh"; } }
                                    }
                                }
                                RowLayout {
                                    Layout.fillWidth: true
                                    visible: editor.selectedMeshBound
                                    Label {
                                        text: "Grid " + editor.selectedMeshColumns + " × " + editor.selectedMeshRows + " · max 32 per axis"
                                        color: "#999999"
                                        font.pixelSize: 10
                                    }
                                    Item { Layout.fillWidth: true }
                                    C.CompactButton {
                                        text: "Remove"
                                        onClicked: editor.removeSelectedMesh()
                                    }
                                }
                                RowLayout {
                                    Layout.fillWidth: true
                                    visible: editor.selectedMeshBound
                                    C.CompactButton {
                                        text: "Pose vertices"
                                        enabled: editor.selectedMeshDeformer === 0
                                        onClicked: { canvas.meshRestEditing = false; editor.tool = "Mesh"; }
                                    }
                                    C.CompactButton {
                                        text: "Rest vertices"
                                        enabled: editor.selectedMeshDeformer === 0
                                        onClicked: { canvas.meshRestEditing = true; editor.tool = "Mesh"; }
                                    }
                                    C.CompactButton {
                                        text: "Reset pose"
                                        enabled: editor.selectedMeshDeformer === 0
                                        onClicked: editor.resetSelectedMeshPose()
                                    }
                                }
                                RowLayout {
                                    Layout.fillWidth: true
                                    visible: editor.selectedMeshBound
                                    C.CompactButton {
                                        text: "Bone chain"
                                        enabled: editor.selectedMeshDeformer === 0
                                        onClicked: { if (editor.bindSelectedBone()) { canvas.meshRestEditing = false; editor.tool = "Mesh"; } }
                                    }
                                    C.CompactButton {
                                        text: "Curve"
                                        enabled: editor.selectedMeshDeformer === 0
                                        onClicked: { if (editor.bindSelectedCurve()) { canvas.meshRestEditing = false; editor.tool = "Mesh"; } }
                                    }
                                    Item { Layout.fillWidth: true }
                                    C.CompactButton {
                                        text: "Remove control"
                                        enabled: editor.selectedMeshDeformer > 0
                                        onClicked: editor.removeSelectedDeformer()
                                    }
                                }
                                RowLayout {
                                    Layout.fillWidth: true
                                    visible: editor.selectedMeshDeformer > 0
                                    C.CompactButton {
                                        text: editor.selectedMeshDeformer === 1 ? "Pose joints" : "Pose curve"
                                        onClicked: { canvas.meshRestEditing = false; editor.tool = "Mesh"; }
                                    }
                                    C.CompactButton {
                                        text: editor.selectedMeshDeformer === 1 ? "Rest joints" : "Rest curve"
                                        onClicked: { canvas.meshRestEditing = true; editor.tool = "Mesh"; }
                                    }
                                }
                                RowLayout {
                                    Layout.fillWidth: true
                                    visible: editor.selectedMeshDeformer === 1
                                    Label { text: "Elbow influence (px)"; color: "#999999"; font.pixelSize: 10 }
                                    Item { Layout.fillWidth: true }
                                    C.CompactSpinBox {
                                        Layout.preferredWidth: 72
                                        from: 1
                                        to: Math.max(1, Math.floor(editor.selectedBoneMaxTransition))
                                        value: Math.round(editor.selectedBoneTransition)
                                        editable: true
                                        onValueModified: {
                                            if (!editor.setSelectedBoneTransition(value))
                                                value = Math.round(editor.selectedBoneTransition)
                                        }
                                        Accessible.name: "Elbow influence radius in pixels"
                                    }
                                }
                                RowLayout {
                                    Layout.fillWidth: true
                                    visible: editor.selectedMeshDeformer > 0
                                    Label {
                                        text: canvas.meshRestEditing
                                              ? (editor.selectedMeshDeformer === 1
                                                 ? "Drag joints or influence handle · Esc cancels"
                                                 : "Drag rest controls on canvas · Esc cancels")
                                              : "Drag pose controls on canvas · Esc cancels"
                                        color: "#999999"
                                        font.pixelSize: 10
                                        Layout.fillWidth: true
                                        wrapMode: Text.Wrap
                                    }
                                    C.CompactButton {
                                        text: "Rest key"
                                        onClicked: editor.resetSelectedDeformerPose()
                                    }
                                }
                                RowLayout {
                                    Layout.fillWidth: true
                                    visible: editor.selectedCanMatchPreviousDeformerPose
                                    Label {
                                        text: "Drawing change at this frame"
                                        color: "#999999"
                                        font.pixelSize: 10
                                    }
                                    Item { Layout.fillWidth: true }
                                    C.CompactButton {
                                        text: "Match previous pose"
                                        Accessible.name: "Match previous drawing's deformer pose"
                                        ToolTip.text: "Key the incoming drawing to the outgoing bone or curve pose"
                                        ToolTip.visible: hovered
                                        onClicked: editor.matchSelectedPreviousDeformerPose()
                                    }
                                }
                            }
                            ColumnLayout {
                                visible: editor.characterId > 0
                                Layout.leftMargin: 16
                                Layout.rightMargin: 16
                                Layout.fillWidth: true
                                spacing: 6
                                Label { text: "Character views"; color: "#999999"; font.pixelSize: 10 }
                                C.CompactComboBox {
                                    Layout.fillWidth: true
                                    model: editor.characterViews
                                    textRole: "name"
                                    valueRole: "id"
                                    currentIndex: model.findIndex(v => v.id === editor.selectedView)
                                    onActivated: editor.selectView(currentValue)
                                    Accessible.name: "Selected character view set"
                                }
                                RowLayout {
                                    Layout.fillWidth: true
                                    C.CompactButton {
                                        text: "‹"
                                        enabled: editor.characterViews.length > 1
                                        Accessible.name: "Previous character view"
                                        onClicked: editor.stepCharacterView(-1)
                                    }
                                    C.CompactButton {
                                        text: "›"
                                        enabled: editor.characterViews.length > 1
                                        Accessible.name: "Next character view"
                                        onClicked: editor.stepCharacterView(1)
                                    }
                                    Item { Layout.fillWidth: true }
                                    C.CompactButton {
                                        text: "←"
                                        enabled: editor.selectedView > 0
                                        Accessible.name: "Move character view earlier"
                                        onClicked: editor.moveCharacterView(-1)
                                    }
                                    C.CompactButton {
                                        text: "→"
                                        enabled: editor.selectedView > 0
                                        Accessible.name: "Move character view later"
                                        onClicked: editor.moveCharacterView(1)
                                    }
                                }
                                RowLayout {
                                    Layout.fillWidth: true
                                    C.CompactButton { text: "+ Capture"; onClicked: editor.captureCharacterView() }
                                    C.CompactButton {
                                        text: "Apply"
                                        enabled: editor.selectedView > 0
                                        onClicked: editor.applyCharacterView()
                                    }
                                    C.CompactButton {
                                        text: "Update"
                                        enabled: editor.selectedView > 0
                                        onClicked: editor.updateCharacterView()
                                    }
                                }
                                RowLayout {
                                    Layout.fillWidth: true
                                    C.CompactButton {
                                        text: "Apply range"
                                        enabled: editor.selectedView > 0
                                        onClicked: editor.applyCharacterViewToRange()
                                    }
                                    C.CompactButton {
                                        text: "Set this part"
                                        enabled: editor.selectedView > 0 && layerInspector.rigLayer?.kind === 3
                                        onClicked: editor.updateSelectedPartInView()
                                    }
                                }
                                RowLayout {
                                    Layout.fillWidth: true
                                    C.CompactButton {
                                        text: "Duplicate"
                                        enabled: editor.selectedView > 0
                                        onClicked: editor.duplicateCharacterView()
                                    }
                                    C.CompactButton {
                                        text: "Remove"
                                        enabled: editor.selectedView > 0
                                        onClicked: editor.removeCharacterView()
                                    }
                                }
                                C.CompactTextField {
                                    Layout.fillWidth: true
                                    text: editor.characterViews.find(v => v.id === editor.selectedView)?.name || ""
                                    placeholderText: "View set name"
                                    enabled: editor.selectedView > 0
                                    onEditingFinished: editor.renameCharacterView(text)
                                    Accessible.name: "Character view set name"
                                }
                                C.CompactCheckBox {
                                    text: "Show in Animator"
                                    checked: editor.characterViews.find(v => v.id === editor.selectedView)?.published || false
                                    enabled: editor.selectedView > 0
                                    onClicked: editor.setSelectedViewPublished(checked)
                                    Accessible.name: "Publish character view"
                                }
                                C.CompactTextField {
                                    Layout.fillWidth: true
                                    text: editor.characterViews.find(v => v.id === editor.selectedView)?.controlGroup || "Main"
                                    placeholderText: "Control group"
                                    enabled: editor.selectedView > 0
                                    onEditingFinished: editor.setSelectedViewControlGroup(text)
                                    Accessible.name: "View control group"
                                }
                                Rectangle { Layout.fillWidth: true; height: 1; color: "#282828" }
                                Label { text: "Character poses"; color: "#999999"; font.pixelSize: 10 }
                                C.CompactComboBox {
                                    Layout.fillWidth: true
                                    model: editor.characterPoses
                                    textRole: "name"
                                    valueRole: "id"
                                    currentIndex: model.findIndex(p => p.id === editor.selectedCharacterPose)
                                    onActivated: editor.selectCharacterPose(currentValue)
                                    Accessible.name: "Selected character pose"
                                }
                                RowLayout {
                                    Layout.fillWidth: true
                                    C.CompactComboBox {
                                        id: poseTarget
                                        Layout.fillWidth: true
                                        model: ["Selected part", "All parts"]
                                        Accessible.name: "Pose capture target"
                                    }
                                    C.CompactComboBox {
                                        id: poseChannels
                                        Layout.fillWidth: true
                                        model: [
                                            { name: "All channels", mask: 511 },
                                            { name: "Transforms", mask: 255 },
                                            { name: "Position", mask: 3 },
                                            { name: "Rotation", mask: 4 },
                                            { name: "Scale", mask: 24 },
                                            { name: "Opacity", mask: 32 },
                                            { name: "Pivot", mask: 192 },
                                            { name: "Drawing", mask: 256 }
                                        ]
                                        textRole: "name"
                                        valueRole: "mask"
                                        Accessible.name: "Pose capture channels"
                                    }
                                }
                                RowLayout {
                                    Layout.fillWidth: true
                                    C.CompactButton {
                                        text: "+ Capture"
                                        enabled: poseTarget.currentIndex === 1 || layerInspector.rigLayer?.kind === 3
                                        onClicked: editor.captureSelectedCharacterPose(poseChannels.currentValue, poseTarget.currentIndex === 1)
                                    }
                                    C.CompactButton {
                                        text: "Apply"
                                        enabled: editor.selectedCharacterPose > 0
                                        onClicked: editor.applySelectedCharacterPose()
                                    }
                                    C.CompactButton {
                                        objectName: "mirrorPoseButton"
                                        text: "Mirror"
                                        enabled: editor.selectedCharacterPose > 0
                                        onClicked: editor.mirrorSelectedCharacterPose()
                                        Accessible.name: "Mirror selected character pose"
                                    }
                                    Item { Layout.fillWidth: true }
                                    C.CompactButton {
                                        text: "Remove"
                                        enabled: editor.selectedCharacterPose > 0
                                        onClicked: editor.removeSelectedCharacterPose()
                                    }
                                }
                                RowLayout {
                                    Layout.fillWidth: true
                                    visible: layerInspector.rigLayer?.kind === 3 && editor.selectedCharacterPose > 0
                                    C.CompactButton {
                                        text: "Set this part"
                                        onClicked: editor.setSelectedPartInCharacterPose(poseChannels.currentValue)
                                    }
                                    C.CompactButton {
                                        text: "Remove part"
                                        enabled: {
                                            const pose = editor.characterPoses.find(p => p.id === editor.selectedCharacterPose)
                                            return pose?.parts > 1 && pose.entries.some(e => e.part === editor.selectedLayer)
                                        }
                                        onClicked: editor.removeSelectedPartFromCharacterPose()
                                    }
                                }
                                RowLayout {
                                    Layout.fillWidth: true
                                    visible: editor.selectedCharacterPose > 0 && editor.poseTransferTargets.length > 0
                                    C.CompactComboBox {
                                        id: poseTransferTarget
                                        objectName: "poseTransferTargetPicker"
                                        Layout.fillWidth: true
                                        model: editor.poseTransferTargets
                                        textRole: "name"
                                        valueRole: "id"
                                        currentIndex: model.length > 0 ? 0 : -1
                                        Accessible.name: "Pose destination character"
                                    }
                                    C.CompactButton {
                                        text: "Copy to"
                                        enabled: poseTransferTarget.currentValue > 0
                                        onClicked: editor.transferSelectedCharacterPose(poseTransferTarget.currentValue)
                                        Accessible.name: "Copy pose to character"
                                    }
                                }
                                Repeater {
                                    model: editor.characterPoses.find(p => p.id === editor.selectedCharacterPose)?.entries || []
                                    Label {
                                        required property var modelData
                                        Layout.fillWidth: true
                                        text: {
                                            const mask = modelData.channels
                                            const names = []
                                            if (mask & 3) names.push("Position")
                                            if (mask & 4) names.push("Rotation")
                                            if (mask & 24) names.push("Scale")
                                            if (mask & 32) names.push("Opacity")
                                            if (mask & 192) names.push("Pivot")
                                            if (mask & 256) names.push("Drawing")
                                            return modelData.name + " · " + names.join(", ")
                                        }
                                        color: "#777777"
                                        font.pixelSize: 10
                                        elide: Text.ElideRight
                                    }
                                }
                                RowLayout {
                                    Layout.fillWidth: true
                                    Label { text: "Blend"; color: "#999999"; font.pixelSize: 10 }
                                    Slider {
                                        id: poseBlend
                                        Layout.fillWidth: true
                                        implicitHeight: 24
                                        from: 0
                                        to: 1
                                        value: 0
                                        enabled: editor.selectedCharacterPose > 0
                                        onPressedChanged: {
                                            if (pressed)
                                                editor.beginSelectedCharacterPoseBlend()
                                            else
                                                editor.endSelectedCharacterPoseBlend()
                                        }
                                        onMoved: editor.updateSelectedCharacterPoseBlend(value)
                                        Accessible.name: "Blend selected character pose"
                                        background: Rectangle {
                                            x: poseBlend.leftPadding
                                            y: poseBlend.topPadding + poseBlend.availableHeight / 2 - height / 2
                                            width: poseBlend.availableWidth
                                            height: 3
                                            radius: 2
                                            color: "#303030"
                                            Rectangle {
                                                width: poseBlend.visualPosition * parent.width
                                                height: parent.height
                                                radius: parent.radius
                                                color: "#b8b8b8"
                                            }
                                        }
                                        handle: Rectangle {
                                            x: poseBlend.leftPadding + poseBlend.visualPosition *
                                               (poseBlend.availableWidth - width)
                                            y: poseBlend.topPadding + poseBlend.availableHeight / 2 - height / 2
                                            width: 12
                                            height: 12
                                            radius: 6
                                            color: "#e8e8e8"
                                            border.color: "#171717"
                                        }
                                    }
                                    Label {
                                        text: Math.round(poseBlend.value * 100) + "%"
                                        color: "#aaaaaa"
                                        font.pixelSize: 10
                                        Layout.preferredWidth: 32
                                        horizontalAlignment: Text.AlignRight
                                    }
                                }
                                Connections {
                                    target: editor
                                    function onPoseSelectionChanged() { poseBlend.value = 0 }
                                }
                                C.CompactTextField {
                                    Layout.fillWidth: true
                                    text: editor.characterPoses.find(p => p.id === editor.selectedCharacterPose)?.name || ""
                                    placeholderText: "Pose name"
                                    enabled: editor.selectedCharacterPose > 0
                                    onEditingFinished: editor.renameSelectedCharacterPose(text)
                                    Accessible.name: "Character pose name"
                                }
                                C.CompactCheckBox {
                                    text: "Show in Animator"
                                    checked: editor.characterPoses.find(p => p.id === editor.selectedCharacterPose)?.published || false
                                    enabled: editor.selectedCharacterPose > 0
                                    onClicked: editor.setSelectedCharacterPosePublished(checked)
                                    Accessible.name: "Publish character pose"
                                }
                                C.CompactTextField {
                                    Layout.fillWidth: true
                                    text: editor.characterPoses.find(p => p.id === editor.selectedCharacterPose)?.controlGroup || "Main"
                                    placeholderText: "Control group"
                                    enabled: editor.selectedCharacterPose > 0
                                    onEditingFinished: editor.setSelectedCharacterPoseControlGroup(text)
                                    Accessible.name: "Pose control group"
                                }
                            }
                        }
                        ColumnLayout {
                            objectName: "animatorDashboard"
                            visible: editor.workspaceMode === "Animator"
                            Layout.leftMargin: 16
                            Layout.rightMargin: 16
                            Layout.fillWidth: true
                            spacing: 8
                            Label {
                                Layout.fillWidth: true
                                text: editor.characterId > 0
                                      ? (editor.layers.find(l => l.id === editor.characterId)?.name || "Character")
                                      : "Select a character or one of its parts"
                                color: "#eeeeee"
                                font.pixelSize: 13
                                font.bold: true
                                wrapMode: Text.WordWrap
                            }
                            C.CompactComboBox {
                                objectName: "animatorControlGroupPicker"
                                Layout.fillWidth: true
                                visible: editor.characterControlGroups.length > 1
                                model: editor.characterControlGroups
                                currentIndex: model.indexOf(editor.selectedControlGroup)
                                onActivated: editor.selectedControlGroup = currentText
                                Accessible.name: "Animator control group"
                            }
                            Label {
                                visible: root.activePublishedViews.length > 0
                                text: "Published views"
                                color: "#999999"
                                font.pixelSize: 10
                            }
                            Repeater {
                                model: root.activePublishedViews
                                C.CompactButton {
                                    required property var modelData
                                    Layout.fillWidth: true
                                    text: modelData.name
                                    Accessible.name: "Apply view " + modelData.name
                                    onClicked: {
                                        editor.selectView(modelData.id)
                                        editor.applyCharacterView()
                                    }
                                }
                            }
                            Label {
                                visible: editor.characterId > 0 && root.activePublishedViews.length === 0
                                text: "No published views in this group."
                                wrapMode: Text.WordWrap
                                color: "#777777"
                                font.pixelSize: 10
                            }
                            Label {
                                visible: root.activePublishedDrawings.length > 0
                                text: "Published drawings"
                                color: "#999999"
                                font.pixelSize: 10
                            }
                            Repeater {
                                model: root.activePublishedDrawings
                                ColumnLayout {
                                    id: publishedPartGroup
                                    objectName: "publishedDrawingGroup"
                                    required property var modelData
                                    Layout.fillWidth: true
                                    spacing: 3
                                    Label {
                                        text: publishedPartGroup.modelData.name
                                        color: "#aaaaaa"
                                        font.pixelSize: 10
                                    }
                                    C.CompactComboBox {
                                        Layout.fillWidth: true
                                        model: publishedPartGroup.modelData.options
                                        textRole: "name"
                                        valueRole: "id"
                                        currentIndex: model.findIndex(v => v.id === publishedPartGroup.modelData.selected)
                                        onActivated: editor.applyPublishedSubstitution(publishedPartGroup.modelData.part,
                                                                                        currentValue)
                                        Accessible.name: "Published drawings for " + publishedPartGroup.modelData.name
                                    }
                                }
                            }
                            Label {
                                visible: root.activePublishedPoses.length > 0
                                text: "Published poses"
                                color: "#999999"
                                font.pixelSize: 10
                            }
                            C.CompactComboBox {
                                objectName: "animatorPosePicker"
                                Layout.fillWidth: true
                                visible: root.activePublishedPoses.length > 0
                                model: root.activePublishedPoses
                                textRole: "name"
                                valueRole: "id"
                                currentIndex: model.findIndex(p => p.id === editor.selectedCharacterPose)
                                onActivated: editor.selectCharacterPose(currentValue)
                                Accessible.name: "Published character pose"
                            }
                            RowLayout {
                                Layout.fillWidth: true
                                visible: root.activePublishedPoses.length > 0
                                C.CompactButton {
                                    text: "Apply"
                                    enabled: root.activePublishedPoses.some(p => p.id === editor.selectedCharacterPose)
                                    onClicked: editor.applySelectedCharacterPose()
                                }
                                Slider {
                                    id: animatorPoseBlend
                                    objectName: "animatorPoseBlend"
                                    Layout.fillWidth: true
                                    implicitHeight: 24
                                    from: 0
                                    to: 1
                                    value: 0
                                    enabled: root.activePublishedPoses.some(p => p.id === editor.selectedCharacterPose)
                                    onPressedChanged: {
                                        if (pressed)
                                            editor.beginSelectedCharacterPoseBlend()
                                        else
                                            editor.endSelectedCharacterPoseBlend()
                                    }
                                    onMoved: editor.updateSelectedCharacterPoseBlend(value)
                                    Accessible.name: "Blend published character pose"
                                    background: Rectangle {
                                        x: animatorPoseBlend.leftPadding
                                        y: animatorPoseBlend.topPadding + animatorPoseBlend.availableHeight / 2 - height / 2
                                        width: animatorPoseBlend.availableWidth
                                        height: 3
                                        radius: 2
                                        color: "#303030"
                                        Rectangle {
                                            width: animatorPoseBlend.visualPosition * parent.width
                                            height: parent.height
                                            radius: parent.radius
                                            color: "#b8b8b8"
                                        }
                                    }
                                    handle: Rectangle {
                                        x: animatorPoseBlend.leftPadding + animatorPoseBlend.visualPosition *
                                           (animatorPoseBlend.availableWidth - width)
                                        y: animatorPoseBlend.topPadding + animatorPoseBlend.availableHeight / 2 - height / 2
                                        width: 12
                                        height: 12
                                        radius: 6
                                        color: "#e8e8e8"
                                        border.color: "#171717"
                                    }
                                }
                                Label {
                                    text: Math.round(animatorPoseBlend.value * 100) + "%"
                                    font.pixelSize: 10
                                    color: "#aaaaaa"
                                }
                            }
                            Connections {
                                target: editor
                                function onPoseSelectionChanged() { animatorPoseBlend.value = 0 }
                            }
                            Label {
                                visible: editor.characterId > 0 && root.activePublishedPoses.length === 0
                                text: "No published poses in this group."
                                wrapMode: Text.WordWrap
                                color: "#777777"
                                font.pixelSize: 10
                            }
                        }
                        Rectangle {
                            Layout.fillWidth: true
                            height: 1
                            color: "#282828"
                            visible: editor.workspaceMode === "Rig"
                        }
                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.leftMargin: 12
                            Layout.rightMargin: 12
                            spacing: 6
                            RowLayout {
                                Layout.fillWidth: true
                                Label { text: "Audio"; font.pixelSize: 13; font.bold: true }
                                Item { Layout.fillWidth: true }
                                C.ToolButton {
                                    text: "+"
                                    hint: "Import PCM16 WAV at the current frame"
                                    onClicked: audioDialog.open()
                                }
                            }
                            Repeater {
                                model: editor.audioClips
                                ColumnLayout {
                                    required property var modelData
                                    Layout.fillWidth: true
                                    spacing: 4
                                    Label {
                                        Layout.fillWidth: true
                                        text: modelData.name + " · " + modelData.channels + "ch / " + modelData.sampleRate + " Hz"
                                        elide: Text.ElideRight
                                        font.pixelSize: 10
                                        color: "#bbbbbb"
                                    }
                                    RowLayout {
                                        Layout.fillWidth: true
                                        Label { text: "Start"; font.pixelSize: 10; color: "#888888" }
                                        C.CompactTextField {
                                            Layout.fillWidth: true
                                            text: String(modelData.start + 1)
                                            validator: IntValidator { bottom: 1; top: editor.duration }
                                            onEditingFinished: if (acceptableInput) editor.moveAudioClip(modelData.id, Number(text) - 1)
                                            Accessible.name: "Audio clip start frame"
                                        }
                                        Label { text: "Gain"; font.pixelSize: 10; color: "#888888" }
                                        C.CompactTextField {
                                            Layout.preferredWidth: 48
                                            text: Number(modelData.gain).toFixed(2)
                                            validator: DoubleValidator { bottom: 0; top: 4; locale: "C" }
                                            onEditingFinished: if (acceptableInput) editor.setAudioClipGain(modelData.id, Number(text))
                                            Accessible.name: "Audio clip gain"
                                        }
                                    }
                                    RowLayout {
                                        Layout.fillWidth: true
                                        Label { text: "Samples"; font.pixelSize: 10; color: "#888888" }
                                        C.CompactTextField {
                                            id: audioIn
                                            Layout.fillWidth: true
                                            text: String(modelData.inSample)
                                            validator: IntValidator { bottom: 0; top: 2147483647 }
                                            Accessible.name: "Audio clip in sample"
                                        }
                                        Label { text: "–"; font.pixelSize: 10; color: "#888888" }
                                        C.CompactTextField {
                                            id: audioOut
                                            Layout.fillWidth: true
                                            text: String(modelData.outSample)
                                            validator: IntValidator { bottom: 1; top: 2147483647 }
                                            Accessible.name: "Audio clip out sample"
                                        }
                                        C.ToolButton {
                                            text: "Set"
                                            hint: "Apply nondestructive audio trim"
                                            onClicked: editor.trimAudioClip(modelData.id, Number(audioIn.text), Number(audioOut.text))
                                        }
                                    }
                                    RowLayout {
                                        Layout.fillWidth: true
                                        Label { text: "Repeats"; font.pixelSize: 10; color: "#888888" }
                                        C.CompactSpinBox {
                                            from: 1
                                            to: 64
                                            value: modelData.repeats
                                            Layout.preferredWidth: 88
                                            onValueModified: editor.setAudioClipRepeats(modelData.id, value)
                                            Accessible.name: "Audio clip repeat count"
                                        }
                                        Item { Layout.fillWidth: true }
                                        Label { text: "× trimmed range"; font.pixelSize: 10; color: "#777777" }
                                    }
                                    Label {
                                        text: "Linear fades · source samples"
                                        font.pixelSize: 10
                                        color: "#777777"
                                    }
                                    RowLayout {
                                        Layout.fillWidth: true
                                        Label { text: "Fade in"; font.pixelSize: 10; color: "#888888" }
                                        C.CompactTextField {
                                            id: audioFadeIn
                                            objectName: "audioFadeInSamples"
                                            Layout.fillWidth: true
                                            text: String(modelData.fadeInSamples)
                                            validator: IntValidator { bottom: 0; top: 2147483647 }
                                            Accessible.name: "Audio fade in source samples"
                                        }
                                        Label { text: "Out"; font.pixelSize: 10; color: "#888888" }
                                        C.CompactTextField {
                                            id: audioFadeOut
                                            objectName: "audioFadeOutSamples"
                                            Layout.fillWidth: true
                                            text: String(modelData.fadeOutSamples)
                                            validator: IntValidator { bottom: 0; top: 2147483647 }
                                            Accessible.name: "Audio fade out source samples"
                                        }
                                        C.ToolButton {
                                            text: "Set"
                                            hint: "Set linear audio fades in source samples"
                                            onClicked: if (audioFadeIn.acceptableInput && audioFadeOut.acceptableInput)
                                                           editor.setAudioClipFades(modelData.id, Number(audioFadeIn.text), Number(audioFadeOut.text))
                                        }
                                    }
                                    RowLayout {
                                        Layout.fillWidth: true
                                        C.ToolButton {
                                            objectName: "audioDuplicateClip"
                                            text: "Duplicate"
                                            hint: "Copy this clip at the current frame using its original WAV"
                                            Layout.fillWidth: true
                                            onClicked: editor.duplicateAudioClip(modelData.id, editor.frame)
                                        }
                                        C.ToolButton {
                                            objectName: "audioSplitClip"
                                            text: "Split"
                                            hint: "Divide a 48 kHz single-pass clip at the current frame"
                                            enabled: modelData.sampleRate === 48000 && modelData.repeats === 1 &&
                                                     editor.frame > modelData.start && editor.frame < modelData.end
                                            onClicked: editor.splitAudioClip(modelData.id, editor.frame)
                                        }
                                        C.ToolButton {
                                            objectName: "audioMuteClip"
                                            text: modelData.muted ? "Unmute" : "Mute"
                                            active: modelData.muted
                                            hint: modelData.muted ? "Include this clip in playback and WAV export"
                                                                  : "Silence this clip in playback and WAV export"
                                            onClicked: editor.setAudioClipMuted(modelData.id, !modelData.muted)
                                        }
                                        C.ToolButton {
                                            objectName: "audioSoloClip"
                                            text: modelData.solo ? "Unsolo" : "Solo"
                                            active: modelData.solo
                                            hint: modelData.solo ? "Return this clip to the shared mix"
                                                                 : "Play this clip with other soloed clips only"
                                            onClicked: editor.setAudioClipSolo(modelData.id, !modelData.solo)
                                        }
                                    }
                                    C.ToolButton {
                                        text: "Remove clip"
                                        Layout.fillWidth: true
                                        hint: "Remove this clip placement without changing its source WAV"
                                        onClicked: editor.removeAudioClip(modelData.id)
                                    }
                                }
                            }
                        }
                        Rectangle { Layout.fillWidth: true; height: 1; color: "#282828" }
                        RowLayout {
                            visible: editor.workspaceMode === "Rig"
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
                            visible: editor.workspaceMode === "Rig"
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
                            visible: editor.workspaceMode === "Rig"
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
                            visible: editor.workspaceMode === "Rig"
                        }
                        ColumnLayout {
                            visible: editor.workspaceMode === "Rig"
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
                    active: !root.xsheet && !root.showCurves && !root.showNodes
                    onClicked: {
                        root.showCurves = false;
                        root.showNodes = false;
                        root.xsheet = false;
                    }
                }
                C.ToolButton {
                    text: "Xsheet"
                    active: root.xsheet && !root.showCurves && !root.showNodes
                    onClicked: {
                        root.showCurves = false;
                        root.showNodes = false;
                        root.xsheet = true;
                    }
                }
                C.ToolButton {
                    text: "Curves"
                    active: root.showCurves && !root.showNodes
                    onClicked: {
                        root.showNodes = false;
                        root.showCurves = true;
                    }
                }
                C.ToolButton {
                    text: "Nodes"
                    active: root.showNodes
                    onClicked: {
                        root.showCurves = false;
                        root.showNodes = true;
                    }
                }
                C.ToolButton {
                    text: "Timing tools"
                    active: root.showTimingTools
                    visible: !root.showCurves && !root.showNodes
                    onClicked: root.showTimingTools = !root.showTimingTools
                }
                C.ToolButton {
                    text: editor.selectedPoseFrames.length > 1 ? "Keys · " + editor.selectedPoseFrames.length : "Keys"
                    visible: !root.showCurves && !root.showNodes
                    active: root.keyEditing
                    hint: "Key mode: drag diamonds to move poses; double-click an empty cell to add a key. Turn off to select exposure ranges."
                    onClicked: root.keyEditing = !root.keyEditing
                }
                C.ToolButton {
                    text: "+ Key"
                    visible: !root.showCurves && !root.showNodes
                    hint: "Add pose key at the current frame"
                    onClicked: editor.addKey()
                }
                C.ToolButton {
                    text: "−"
                    visible: !root.showCurves && !root.showNodes && !root.xsheet
                    hint: "Narrow timeline frames"
                    enabled: root.timelineCell > 12
                    onClicked: {
                        root.timelineCell = Math.max(12, root.timelineCell - 10);
                        timeline.requestPaint();
                    }
                }
                C.ToolButton {
                    text: "+"
                    visible: !root.showCurves && !root.showNodes && !root.xsheet
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
            visible: root.showTimingTools && !root.showCurves && !root.showNodes
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
        C.CompositionNodes {
            id: compositionNodes
            objectName: "compositionNodesPanel"
            visible: root.showNodes
            controller: editor
            Layout.fillWidth: true
            Layout.preferredHeight: root.effectiveBottomHeight
            Layout.minimumHeight: root.effectiveBottomHeight
            Layout.maximumHeight: root.effectiveBottomHeight
            onLayerChosen: layer => {
                canvas.clearRegion();
                root.inspectorMode = "layer";
                editor.selectedLayer = layer;
            }
        }
        RowLayout {
            Layout.fillWidth: true
            visible: !root.showCurves && !root.showNodes
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
                                text: layerInspector.rigLayer?.kind === 2 || layerInspector.rigLayer?.kind === 3 ?
                                      "Duplicate rig branch" : "Duplicate layer"
                                onTriggered: editor.duplicateLayer(false)
                            }
                            Action {
                                text: layerInspector.rigLayer?.kind === 2 || layerInspector.rigLayer?.kind === 3 ?
                                      "Clone branch with linked artwork" : "Clone linked drawings"
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
                                    visible: modelData.kind !== 4
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
                                    visible: modelData.kind !== 4
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
                contentHeight: root.xsheet ? editor.duration * root.timelineRow + 30 : Math.max(height, (editor.layers.length + editor.audioClips.length) * root.timelineRow + 30)
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
                            const clips = editor.audioClips;
                            const anySolo = clips.some(c => c.solo);
                            for (let r = 0; r < clips.length; r++) {
                                const clip = clips[r];
                                const inactive = clip.muted || (anySolo && !clip.solo);
                                const y = 30 + (data.length + r) * root.timelineRow - oy;
                                if (y + root.timelineRow < 0 || y > height)
                                    continue;
                                ctx.fillStyle = "#1e1e1e";
                                ctx.fillRect(0, y, width, root.timelineRow);
                                ctx.strokeStyle = "#313131";
                                ctx.beginPath(); ctx.moveTo(0, y + root.timelineRow); ctx.lineTo(width, y + root.timelineRow); ctx.stroke();
                                const delta = timelineInput.audioDragClipId === clip.id ? timelineInput.audioPreviewStart - clip.start : 0;
                                const first = Math.max(clip.start, Math.max(0, Math.floor(ox / root.timelineCell) - delta));
                                const last = Math.min(clip.end, Math.ceil((ox + width) / root.timelineCell) - delta);
                                const peaks = first < last ? editor.audioWaveform(clip.id, first, last - first) : [];
                                for (let f = first; f < last; f++) {
                                    const amplitude = peaks[f - first];
                                    if (amplitude <= 0) continue;
                                    const x = (f + delta) * root.timelineCell - ox + 1;
                                    const h = Math.max(1, amplitude * 12);
                                    ctx.fillStyle = inactive ? "#525252" : delta ? "#ffffff" : "#a8a8a8";
                                    ctx.fillRect(x, y + 17 - h, Math.max(1, root.timelineCell - 2), h * 2);
                                }
                                const editingFade = timelineInput.audioFadeClipId === clip.id;
                                const fadeIn = editingFade && timelineInput.audioFadeSide === "in"
                                             ? timelineInput.audioFadePreviewSamples : clip.fadeInSamples;
                                const fadeOut = editingFade && timelineInput.audioFadeSide === "out"
                                              ? timelineInput.audioFadePreviewSamples : clip.fadeOutSamples;
                                {
                                    const pixelsPerSample = root.timelineCell * editor.fps / clip.sampleRate;
                                    const beginX = (clip.start + delta) * root.timelineCell - ox;
                                    const endX = beginX + (clip.outSample - clip.inSample) *
                                                 clip.repeats * pixelsPerSample;
                                    ctx.save();
                                    ctx.beginPath();
                                    ctx.rect(0, y, width, root.timelineRow);
                                    ctx.clip();
                                    ctx.strokeStyle = inactive ? "#777777" : "#eeeeee";
                                    ctx.lineWidth = 1;
                                    const inX = beginX + fadeIn * pixelsPerSample;
                                    const outX = endX - fadeOut * pixelsPerSample;
                                    if (fadeIn > 0) {
                                        ctx.beginPath();
                                        ctx.moveTo(beginX, y + 17);
                                        ctx.lineTo(inX, y + 6);
                                        ctx.moveTo(beginX, y + 17);
                                        ctx.lineTo(inX, y + 28);
                                        ctx.stroke();
                                    }
                                    if (fadeOut > 0) {
                                        ctx.beginPath();
                                        ctx.moveTo(outX, y + 6);
                                        ctx.lineTo(endX, y + 17);
                                        ctx.moveTo(outX, y + 28);
                                        ctx.lineTo(endX, y + 17);
                                        ctx.stroke();
                                    }
                                    ctx.fillStyle = inactive ? "#777777" : "#eeeeee";
                                    ctx.fillRect(inX - 2, y + 4, 5, 5);
                                    ctx.fillRect(outX - 2, y + 4, 5, 5);
                                    ctx.restore();
                                }
                                ctx.fillStyle = inactive ? "#777777" : "#eeeeee";
                                ctx.fillText(clip.name + (clip.muted ? " · Muted" : clip.solo ? " · Solo" : anySolo ? " · Other solo" : ""),
                                             (clip.start + delta) * root.timelineCell - ox + 3, y + 5);
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
                                if (p.x >= 0 && p.x <= width && p.y >= 0 && p.y <= height) {
                                    const selected = data[r].id === editor.selectedLayer && editor.selectedPoseFrames.indexOf(frame) >= 0;
                                    ctx.fillStyle = selected ? "#ffffff" : "#888888";
                                    diamond(ctx, p, true);
                                    if (selected)
                                        ctx.strokeRect(p.x - 8, p.y - 8, 16, 16);
                                }
                            }
                        if (timelineInput.keySource >= 0)
                            for (const frame of editor.selectedPoseFrames)
                                diamond(ctx, keyPosition(frame + timelineInput.previewFrame - timelineInput.keySource, timelineInput.anchorRow), false);
                    }
                    MouseArea {
                        id: timelineInput
                        objectName: "timelineInput"
                        hoverEnabled: true
                        property bool overRangeEnd: {
                            const row = rowAt(Qt.point(mouseX, mouseY));
                            const edge = root.xsheet ? 30 + editor.rangeEnd * root.timelineRow - timelineScroll.contentY : editor.rangeEnd * root.timelineCell - timelineScroll.contentX;
                            return !root.keyEditing && row >= 0 && row < editor.layers.length && editor.selectedLayers.indexOf(editor.layers[row].id) >= 0 && Math.abs((root.xsheet ? mouseY : mouseX) - edge) <= 5;
                        }
                        cursorShape: audioFadeClipId >= 0 || fadeHandleAt(Qt.point(mouseX, mouseY))
                                     ? Qt.SizeHorCursor : audioDragClipId >= 0 ? Qt.ClosedHandCursor :
                                       keySource >= 0 ? Qt.ClosedHandCursor : resizing || overRangeEnd
                                     ? (root.xsheet ? Qt.SizeVerCursor : Qt.SizeHorCursor) :
                                       audioAt(Qt.point(mouseX, mouseY)) || keyAt(Qt.point(mouseX, mouseY)) >= 0
                                     ? Qt.OpenHandCursor : pressed && moving ? Qt.ClosedHandCursor : Qt.CrossCursor
                        preventStealing: true
                        anchors.fill: parent
                        property int keySource: -1
                        property bool duplicateKeys: false
                        property bool canceled: false
                        property int audioDragClipId: -1
                        property int audioDragStart: 0
                        property int audioPreviewStart: 0
                        property int audioFadeClipId: -1
                        property string audioFadeSide: ""
                        property int audioFadePreviewSamples: 0
                        function fadeHandleAt(mouse) {
                            if (root.xsheet)
                                return null;
                            const index = rowAt(mouse) - editor.layers.length;
                            const clips = editor.audioClips;
                            if (index < 0 || index >= clips.length)
                                return null;
                            const clip = clips[index];
                            const y = 30 + (editor.layers.length + index) * root.timelineRow - timelineScroll.contentY;
                            if (Math.abs(mouse.y - (y + 6)) > 8)
                                return null;
                            const pixelsPerSample = root.timelineCell * editor.fps / clip.sampleRate;
                            const beginX = clip.start * root.timelineCell - timelineScroll.contentX;
                            const endX = beginX + (clip.outSample - clip.inSample) *
                                         clip.repeats * pixelsPerSample;
                            const inDistance = Math.abs(mouse.x - (beginX + clip.fadeInSamples * pixelsPerSample));
                            const outDistance = Math.abs(mouse.x - (endX - clip.fadeOutSamples * pixelsPerSample));
                            if (Math.min(inDistance, outDistance) > 8)
                                return null;
                            return {clip: clip, side: inDistance <= outDistance ? "in" : "out"};
                        }
                        function audioAt(mouse) {
                            if (root.xsheet)
                                return null;
                            const index = rowAt(mouse) - editor.layers.length;
                            const clips = editor.audioClips;
                            if (index < 0 || index >= clips.length)
                                return null;
                            const clip = clips[index];
                            const frame = frameAt(mouse);
                            return frame >= clip.start && frame < clip.end ? clip : null;
                        }
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
                            editor.endAudioScrub();
                            canceled = true;
                            keySource = -1;
                            audioDragClipId = -1;
                            audioFadeClipId = -1;
                            audioFadeSide = "";
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
                            const fade = fadeHandleAt(mouse);
                            if (fade) {
                                audioFadeClipId = fade.clip.id;
                                audioFadeSide = fade.side;
                                audioFadePreviewSamples = fade.side === "in"
                                                        ? fade.clip.fadeInSamples : fade.clip.fadeOutSamples;
                                return;
                            }
                            const audio = audioAt(mouse);
                            if (audio) {
                                audioDragClipId = audio.id;
                                audioDragStart = audio.start;
                                audioPreviewStart = audio.start;
                                editor.frame = anchorFrame;
                                return;
                            }
                            const edge = root.xsheet ? 30 + editor.rangeEnd * root.timelineRow - timelineScroll.contentY : editor.rangeEnd * root.timelineCell - timelineScroll.contentX;
                            const coordinate = root.xsheet ? mouse.y : mouse.x;
                            resizing = !root.keyEditing && anchorRow >= 0 && anchorRow < editor.layers.length && editor.selectedLayers.indexOf(editor.layers[anchorRow].id) >= 0 && Math.abs(coordinate - edge) <= 5;
                            previewEnd = editor.rangeEnd;
                            moving = !root.keyEditing && !resizing && (mouse.modifiers & Qt.AltModifier) && anchorRow >= 0 && anchorRow < editor.layers.length && anchorFrame >= editor.rangeStart && anchorFrame < editor.rangeEnd && editor.selectedLayers.indexOf(editor.layers[anchorRow].id) >= 0;
                            if (resizing) {
                                timeline.requestPaint();
                                return;
                            }
                            const key = keyAt(mouse);
                            if (!moving && key >= 0) {
                                editor.selectedLayer = editor.layers[anchorRow].id;
                                editor.selectPoseKey(key, !!(mouse.modifiers & Qt.ShiftModifier), !!(mouse.modifiers & (Qt.ControlModifier | Qt.MetaModifier)));
                                duplicateKeys = !!(mouse.modifiers & Qt.AltModifier);
                                canceled = editor.selectedPoseFrames.indexOf(key) < 0;
                                if (canceled)
                                    return;
                                keySource = key;
                                previewFrame = key;
                                timeline.requestPaint();
                                return;
                            }
                            if (root.keyEditing && !moving && anchorRow >= 0 && anchorRow < editor.layers.length) {
                                editor.selectedLayer = editor.layers[anchorRow].id;
                                editor.frame = anchorFrame;
                                editor.beginAudioScrub();
                                canceled = false;
                                return;
                            }
                            if (moving)
                                previewFrame = editor.rangeStart;
                            else if (anchorRow >= 0 && anchorRow < editor.layers.length)
                                editor.selectTimelineRange(anchorFrame, anchorFrame, anchorRow, anchorRow);
                            else
                                editor.frame = anchorFrame;
                            if (!moving)
                                editor.beginAudioScrub();
                        }
                        onPositionChanged: function (mouse) {
                            if (!pressed || canceled)
                                return;
                            if (audioFadeClipId >= 0) {
                                const clip = editor.audioClips.find(c => c.id === audioFadeClipId);
                                if (!clip) { cancel(); return; }
                                const total = (clip.outSample - clip.inSample) * clip.repeats;
                                const pixelsPerSample = root.timelineCell * editor.fps / clip.sampleRate;
                                const beginX = clip.start * root.timelineCell - timelineScroll.contentX;
                                const endX = beginX + total * pixelsPerSample;
                                const raw = audioFadeSide === "in"
                                            ? (mouse.x - beginX) / pixelsPerSample
                                            : (endX - mouse.x) / pixelsPerSample;
                                const other = audioFadeSide === "in"
                                              ? clip.fadeOutSamples : clip.fadeInSamples;
                                audioFadePreviewSamples = Math.max(0, Math.min(total - other, Math.round(raw)));
                                timeline.requestPaint();
                                return;
                            }
                            if (audioDragClipId >= 0) {
                                audioPreviewStart = Math.max(0, Math.min(editor.duration - 1, audioDragStart + frameAt(mouse) - anchorFrame));
                                timeline.requestPaint();
                                return;
                            }
                            if (keySource >= 0) {
                                const selected = editor.selectedPoseFrames;
                                const offset = Math.max(-selected[0], Math.min(editor.duration - 1 - selected[selected.length - 1], frameAt(mouse) - anchorFrame));
                                previewFrame = keySource + offset;
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
                            editor.endAudioScrub();
                            if (canceled)
                                return;
                            if (audioFadeClipId >= 0) {
                                const clip = editor.audioClips.find(c => c.id === audioFadeClipId);
                                const preview = audioFadePreviewSamples;
                                const side = audioFadeSide;
                                audioFadeClipId = -1;
                                audioFadeSide = "";
                                if (clip && preview !== (side === "in" ? clip.fadeInSamples : clip.fadeOutSamples))
                                    editor.setAudioClipFades(clip.id,
                                        side === "in" ? preview : clip.fadeInSamples,
                                        side === "out" ? preview : clip.fadeOutSamples);
                                timeline.requestPaint();
                                return;
                            }
                            if (audioDragClipId >= 0) {
                                const clipId = audioDragClipId, start = audioPreviewStart, oldStart = audioDragStart;
                                audioDragClipId = -1;
                                if (start !== oldStart)
                                    editor.moveAudioClip(clipId, start);
                                timeline.requestPaint();
                                return;
                            }
                            if (keySource >= 0) {
                                const offset = previewFrame - keySource, duplicate = duplicateKeys;
                                cancel();
                                if (offset !== 0)
                                    editor.moveSelectedPoseKeys(offset, duplicate);
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
                            if (!root.xsheet && rowAt(mouse) >= editor.layers.length)
                                return;
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
                        Accessible.name: "Timeline. Drag audio waveforms to move clips or their upper fade handles to adjust fades. Drag diamonds to retime poses. Enable Keys to add keys by double-clicking. Alt-drag moves an exposure range."
                    }
                    Keys.onEscapePressed: {
                        if (timelineInput.audioDragClipId >= 0 || timelineInput.audioFadeClipId >= 0 || timelineInput.keySource >= 0 || timelineInput.moving || timelineInput.resizing)
                            timelineInput.cancel();
                        else
                            editor.clearPoseSelection();
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
                    function onKeySelectionChanged() {
                        if (timelineInput.keySource >= 0)
                            timelineInput.cancel();
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
    FileDialog {
        id: partsDialog
        title: "Import registered PNG parts · same canvas size"
        fileMode: FileDialog.OpenFiles
        nameFilters: ["PNG images (*.png)"]
        onAccepted: editor.importParts(selectedFiles)
    }
    FileDialog {
        id: sequenceDialog
        title: "Import numbered PNG sequence · gaps stay empty"
        fileMode: FileDialog.OpenFiles
        nameFilters: ["PNG images (*.png)"]
        onAccepted: editor.importImageSequence(selectedFiles)
    }
    FileDialog {
        id: audioDialog
        title: "Import PCM16 WAV"
        nameFilters: ["PCM WAV audio (*.wav)"]
        onAccepted: editor.importAudio(selectedFile)
    }
    FolderDialog {
        id: exportDialog
        title: "Choose a folder for a new PNG sequence export"
        onAccepted: editor.exportFrames(selectedFolder)
    }
    FileDialog {
        id: audioExportDialog
        title: "Export PCM WAV mix"
        fileMode: FileDialog.SaveFile
        defaultSuffix: "wav"
        nameFilters: ["PCM WAV audio (*.wav)"]
        onAccepted: editor.exportAudio(selectedFile)
    }
    FileDialog {
        id: audioRangeExportDialog
        title: "Export selected PCM WAV range"
        fileMode: FileDialog.SaveFile
        defaultSuffix: "wav"
        nameFilters: ["PCM WAV audio (*.wav)"]
        onAccepted: editor.exportAudioRange(selectedFile, editor.rangeStart, editor.rangeEnd)
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
            text: "B — Pencil\nE — Eraser\nV — Select and move vectors (Shift adds, Alt subtracts)\nA — Animate layer poses on canvas\nM — Rectangular vector/raster selection\nL — Whole-vector lasso\nO — Onion skin\nF — Fit canvas\nSpace — Play / pause\nLeft / Right — Previous / next frame\nCmd/Ctrl+Z — Undo\n\nDraw with the left mouse button. Pan with the middle button or trackpad scroll. Ctrl+scroll zooms. Double-click a timeline cell to create a new drawing.\n\nTablets use pressure when available; physical tablet validation is pending. This is an experimental build, not the P11 release."
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
