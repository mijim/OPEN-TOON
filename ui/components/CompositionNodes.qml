import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

Item {
    id: root
    required property var controller
    required property int displayedNodeId
    signal layerChosen(int layer)
    signal displayRequested(int nodeId, int kindCode, int layer)
    signal finalDisplayRequested()
    readonly property var selected: controller.layers.find(l => l.id === controller.selectedLayer)
    readonly property int selectedGroup: selected?.compositeGroup || 0
    readonly property var selectedGroupNode: controller.compositionNodes.find(n =>
        n.kind === "Group output" && n.group === selectedGroup)
    property int groupStartLayer: 0
    property int previewNodeId: 0
    property string previewNodeKind: ""
    property int previewLayerId: 0
    property string previewData: ""
    property int draggedLayer: 0
    property int dropLayer: 0
    property int matchIndex: -1
    readonly property int cardWidth: 148
    readonly property int cardGap: 8
    readonly property var matchingNodeIds: {
        const query = nodeSearch ? nodeSearch.text.trim().toLowerCase() : ""
        return query ? controller.compositionNodes.filter(n =>
            n.name.toLowerCase().includes(query) || n.kind.toLowerCase().includes(query))
            .map(n => n.id) : []
    }
    function toggleNodeBypass(node) {
        if (node.layer <= 0)
            return false
        if (node.kind === "Opacity" || node.kind === "Bypassed opacity") {
            root.layerChosen(node.layer)
            root.controller.setOpacityBypassed(node.kind === "Opacity")
        } else if (node.kind === "Apply matte" || node.kind === "Bypassed cutter") {
            root.layerChosen(node.layer)
            root.controller.setMatteBypassed(node.kind === "Apply matte")
        } else if (node.kind === "Multiply" || node.kind === "Screen" ||
                   node.kind === "Add" || node.kind === "Bypassed blend") {
            root.layerChosen(node.layer)
            root.controller.setBlendBypassed(node.kind !== "Bypassed blend")
        } else if (node.kind === "Composite" || node.kind === "Bypassed composite") {
            root.layerChosen(node.layer)
            root.controller.setCompositeBypassed(node.kind === "Composite")
        } else {
            return false
        }
        return true
    }
    function showMatch(index) {
        if (matchingNodeIds.length === 0) {
            matchIndex = -1
            return
        }
        matchIndex = (index + matchingNodeIds.length) % matchingNodeIds.length
        const nodeIndex = controller.compositionNodes.findIndex(n =>
            n.id === matchingNodeIds[matchIndex])
        if (nodeIndex >= 0) {
            const center = 12 + nodeIndex * (cardWidth + cardGap) + cardWidth / 2
            graphScroll.contentX = Math.max(0, Math.min(
                graphScroll.contentWidth - graphScroll.width,
                center - graphScroll.width / 2))
        }
    }
    function refreshPreview() {
        if (previewNodeId <= 0)
            return
        const node = controller.compositionNodes.find(n => n.id === previewNodeId)
        if (!node || node.kind !== previewNodeKind || node.layer !== previewLayerId) {
            previewNodeId = 0
            previewData = ""
            return
        }
        previewData = controller.compositionNodePreview(previewNodeId)
    }
    Connections {
        target: root.controller
        function onChanged() { root.refreshPreview() }
        function onFrameChanged() { root.refreshPreview() }
    }
    implicitHeight: 280

    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 38
            Layout.leftMargin: 12
            Layout.rightMargin: 12
            spacing: 8
            Label {
                text: "DERIVED COMPOSITION"
                color: "#999999"
                font.pixelSize: 10
                font.letterSpacing: 1
            }
            Label {
                text: "Drag: front · Shift-drag: behind · Alt-drag: cutter · Shift-click: group"
                visible: root.selectedGroup === 0
                color: "#777777"
                font.pixelSize: 10
            }
            CompactTextField {
                objectName: "nodeGroupName"
                visible: root.selectedGroup > 0
                implicitWidth: 116
                text: root.selectedGroupNode?.name || ""
                onAccepted: root.controller.renameCompositeGroup(root.selectedGroup, text)
                Accessible.name: "Composite group name"
            }
            Item { Layout.fillWidth: true }
            ToolButton {
                text: "Back"
                enabled: root.selected && (root.selected.kind === 0 || root.selected.kind === 3)
                onClicked: root.controller.moveLayer(-1)
                Accessible.name: "Move selected drawing backward in composite order"
            }
            ToolButton {
                text: "Front"
                enabled: root.selected && (root.selected.kind === 0 || root.selected.kind === 3)
                onClicked: root.controller.moveLayer(1)
                Accessible.name: "Move selected drawing forward in composite order"
            }
            ToolButton {
                objectName: "nodeUngroup"
                text: "Ungroup"
                visible: root.selectedGroup > 0
                onClicked: root.controller.ungroupDrawings(root.selectedGroup)
                Accessible.name: "Ungroup selected composite drawings"
            }
            CompactComboBox {
                objectName: "nodeCutterPicker"
                implicitWidth: 170
                enabled: root.selected && !root.selected.locked &&
                         (root.selected.kind === 0 || root.selected.kind === 3)
                model: [{id: 0, name: "No cutter matte"}].concat(
                    root.controller.layers.filter(l =>
                        (l.kind === 0 || l.kind === 3) && l.id !== root.selected?.id &&
                        l.visible && l.matte === 0))
                currentIndex: Math.max(0, model.findIndex(l => l.id === root.selected?.matte))
                textRole: "name"
                valueRole: "id"
                onActivated: root.controller.setLayerMatte(currentValue)
                Accessible.name: "Selected drawing cutter matte"
            }
            CompactCheckBox {
                objectName: "nodeBypassMatte"
                text: "Bypass"
                visible: root.selected?.matte > 0
                enabled: visible && !root.selected?.locked
                checked: root.selected?.matteBypassed || false
                onClicked: root.controller.setMatteBypassed(checked)
                Accessible.name: "Bypass selected cutter matte"
            }
            CompactCheckBox {
                objectName: "nodeInvertMatte"
                text: "Outside"
                visible: root.selected?.matte > 0
                enabled: visible && !root.selected?.locked
                checked: root.selected?.invertMatte || false
                onClicked: root.controller.setMatteInverted(checked)
                Accessible.name: "Invert selected cutter matte"
            }
            CompactCheckBox {
                objectName: "nodePaintCutter"
                text: "Paint cutter"
                visible: root.selected?.matte > 0
                enabled: visible && !root.selected?.locked &&
                         !root.controller.layers.find(l => l.id === root.selected?.matte)?.locked
                checked: root.controller.layers.find(l => l.id === root.selected?.matte)?.paintMatteSource || false
                onClicked: root.controller.setMatteSourceVisible(checked)
                Accessible.name: "Paint cutter source in composition"
            }
        }
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 34
            Layout.leftMargin: 12
            Layout.rightMargin: 12
            spacing: 8
            Label {
                text: "BLEND"
                color: "#999999"
                font.pixelSize: 10
                font.letterSpacing: 1
            }
            CompactComboBox {
                objectName: "nodeBlendMode"
                implicitWidth: 130
                enabled: root.selected && !root.selected.locked &&
                         (root.selected.kind === 0 || root.selected.kind === 3)
                model: ["Normal", "Multiply", "Screen", "Add"]
                currentIndex: root.selected?.blendMode || 0
                onActivated: root.controller.setLayerBlendMode(currentIndex)
                Accessible.name: "Selected drawing blend mode"
            }
            CompactCheckBox {
                objectName: "nodeBypassBlend"
                text: "Bypass blend"
                visible: root.selected && (root.selected.kind === 0 || root.selected.kind === 3)
                enabled: visible && !root.selected?.locked && root.selected?.blendMode !== 0
                checked: root.selected?.blendBypassed || false
                onClicked: root.controller.setBlendBypassed(checked)
                Accessible.name: "Bypass selected drawing blend mode"
            }
            CompactCheckBox {
                objectName: "nodeBypassComposite"
                text: "Bypass layer"
                visible: root.selected && (root.selected.kind === 0 || root.selected.kind === 3)
                enabled: visible && !root.selected?.locked
                checked: root.selected?.compositeBypassed || false
                onClicked: root.controller.setCompositeBypassed(checked)
                Accessible.name: "Bypass selected drawing in the composition"
            }
            PropertyNumber {
                objectName: "nodeOpacity"
                implicitWidth: 88
                visible: root.selected && (root.selected.kind === 0 || root.selected.kind === 3)
                enabled: visible && !root.selected?.locked
                number: root.controller.transform.opacity === undefined
                        ? 1 : root.controller.transform.opacity
                label: "Selected drawing opacity 0–1"
                onCommitted: value => root.controller.setTransform("opacity", value)
            }
            CompactCheckBox {
                objectName: "nodeBypassOpacity"
                text: "Bypass opacity"
                visible: root.selected && (root.selected.kind === 0 || root.selected.kind === 3)
                enabled: visible && !root.selected?.locked
                checked: root.selected?.opacityBypassed || false
                onClicked: root.controller.setOpacityBypassed(checked)
                Accessible.name: "Bypass selected drawing opacity"
            }
            Item { Layout.fillWidth: true }
            CompactTextField {
                id: nodeSearch
                objectName: "nodeSearch"
                implicitWidth: 132
                placeholderText: "Find node"
                onTextChanged: root.showMatch(0)
                onAccepted: root.showMatch(root.matchIndex + 1)
                Accessible.name: "Find composition node by name or kind"
            }
            Label {
                text: root.matchingNodeIds.length > 0
                      ? (root.matchIndex + 1) + "/" + root.matchingNodeIds.length : ""
                color: "#999999"
                font.pixelSize: 10
            }
            ToolButton {
                objectName: "nodeSearchNext"
                text: "Next"
                enabled: root.matchingNodeIds.length > 1
                onClicked: root.showMatch(root.matchIndex + 1)
                Accessible.name: "Find next matching composition node"
            }
        }
        Rectangle { Layout.fillWidth: true; height: 1; color: "#303030" }
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0
        Flickable {
            id: graphScroll
            objectName: "compositionNodeStrip"
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            contentWidth: graphRow.width + 24
            contentHeight: graphRow.height + 28
            Row {
                id: graphRow
                x: 12
                y: 14
                spacing: root.cardGap
                Repeater {
                    model: root.controller.compositionNodes
                    Rectangle {
                        required property var modelData
                        readonly property int drawingLayer: modelData.kind === "Drawing" ? modelData.layer : 0
                        objectName: "compositionNode" + modelData.id
                        width: root.cardWidth
                        height: Math.max(110, graphScroll.height - 30)
                        radius: 5
                        color: modelData.id === root.previewNodeId ? "#292929" : "#181818"
                        border.color: drawingLayer > 0 && root.dropLayer === drawingLayer
                                      ? "#eeeeee" : root.matchIndex >= 0 &&
                                      root.matchingNodeIds[root.matchIndex] === modelData.id
                                      ? "#dddddd" : root.matchingNodeIds.includes(modelData.id)
                                      ? "#666666" : drawingLayer > 0 &&
                                      root.groupStartLayer === drawingLayer
                                      ? "#ffffff" : modelData.group > 0
                                      ? "#aaaaaa" : modelData.kind === "Cutter" ||
                                      modelData.kind === "Opacity" ||
                                      modelData.kind === "Bypassed opacity" ||
                                      modelData.kind === "Multiply" ||
                                      modelData.kind === "Screen" ||
                                      modelData.kind === "Add" ||
                                      modelData.kind === "Bypassed blend" ||
                                      modelData.kind === "Bypassed composite" ||
                                      modelData.kind === "Bypassed cutter" ||
                                      modelData.kind === "Invert matte" ||
                                      modelData.kind === "Apply matte"
                                      ? "#7b7b7b" : "#393939"
                        Column {
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 6
                            Label {
                                width: parent.width
                                text: "#" + modelData.id + "  " + modelData.kind
                                color: "#999999"
                                font.pixelSize: 10
                                elide: Text.ElideRight
                            }
                            Label {
                                width: parent.width
                                text: modelData.name
                                color: "#eeeeee"
                                font.pixelSize: 12
                                font.bold: true
                                elide: Text.ElideRight
                            }
                            Label {
                                width: parent.width
                                text: modelData.inputs.length
                                      ? modelData.inputs.map(p => "In " + p.slot + " ← #" + p.source).join("\n")
                                      : "Source"
                                color: "#777777"
                                font.pixelSize: 10
                                lineHeight: 1.3
                            }
                        }
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            preventStealing: modelData.kind === "Drawing"
                            property bool dragging: false
                            property bool dragCutter: false
                            property bool dragBehind: false
                            property real downX: 0
                            property real downY: 0
                            onPressed: mouse => {
                                dragging = false
                                dragCutter = (mouse.modifiers & Qt.AltModifier) !== 0
                                dragBehind = !dragCutter && (mouse.modifiers & Qt.ShiftModifier) !== 0
                                downX = mouse.x
                                downY = mouse.y
                            }
                            onPositionChanged: mouse => {
                                if (!pressed || modelData.kind !== "Drawing")
                                    return
                                if (!dragging && Math.hypot(mouse.x - downX, mouse.y - downY) > 8) {
                                    dragging = true
                                    root.draggedLayer = modelData.layer
                                }
                                if (dragging) {
                                    const point = mapToItem(graphRow, mouse.x, mouse.y)
                                    const card = graphRow.childAt(point.x, point.y)
                                    root.dropLayer = card && card.drawingLayer !== root.draggedLayer
                                                     ? card.drawingLayer : 0
                                }
                            }
                            onReleased: {
                                const source = root.draggedLayer
                                const target = root.dropLayer
                                root.draggedLayer = 0
                                root.dropLayer = 0
                                if (dragging && target > 0) {
                                    if (dragCutter) {
                                        root.layerChosen(target)
                                        root.controller.setLayerMatte(source)
                                    } else {
                                        root.layerChosen(source)
                                        if (dragBehind)
                                            root.controller.moveDrawingBefore(source, target)
                                        else
                                            root.controller.moveDrawingAfter(source, target)
                                    }
                                }
                            }
                            onCanceled: {
                                root.draggedLayer = 0
                                root.dropLayer = 0
                            }
                            onClicked: mouse => {
                                if (dragging)
                                    return
                                if ((mouse.modifiers & Qt.ShiftModifier) !== 0 &&
                                    modelData.kind === "Drawing") {
                                    if (root.groupStartLayer === modelData.layer) {
                                        root.groupStartLayer = 0
                                        return
                                    }
                                    if (root.groupStartLayer === 0) {
                                        root.groupStartLayer = modelData.layer
                                        root.layerChosen(modelData.layer)
                                        return
                                    }
                                    const first = root.groupStartLayer
                                    root.groupStartLayer = 0
                                    root.layerChosen(modelData.layer)
                                    root.controller.groupDrawings(first, modelData.layer)
                                    return
                                }
                                if ((mouse.modifiers & Qt.AltModifier) !== 0 &&
                                    root.toggleNodeBypass(modelData))
                                    return
                                root.previewNodeId = modelData.id
                                root.previewNodeKind = modelData.kind
                                root.previewLayerId = modelData.layer
                                root.refreshPreview()
                                if (modelData.layer > 0)
                                    root.layerChosen(modelData.layer)
                                else if (modelData.group > 0) {
                                    const member = root.controller.layers.find(l =>
                                        l.compositeGroup === modelData.group)
                                    if (member)
                                        root.layerChosen(member.id)
                                }
                            }
                            Accessible.name: modelData.layer > 0
                                             ? "Preview and select " + modelData.name + " composition source; Alt-click bypassable nodes to toggle"
                                             : "Preview " + modelData.kind + " composition node"
                        }
                    }
                }
            }
            ScrollBar.horizontal: ScrollBar { policy: ScrollBar.AsNeeded }
        }
        Rectangle {
            objectName: "compositionNodePreviewPanel"
            visible: root.previewNodeId > 0
            Layout.preferredWidth: visible ? Math.min(274, root.width * 0.28) : 0
            Layout.fillHeight: true
            color: "#141414"
            border.color: "#303030"
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 8
                Label {
                    text: "NODE #" + root.previewNodeId + " PREVIEW"
                    color: "#999999"
                    font.pixelSize: 10
                    font.letterSpacing: 1
                }
                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: "#333333"
                    Image {
                        objectName: "compositionNodePreviewImage"
                        anchors.fill: parent
                        source: root.previewData
                        fillMode: Image.PreserveAspectFit
                        cache: false
                    }
                }
                Label {
                    text: "Matte preview shows alpha as grayscale"
                    color: "#777777"
                    font.pixelSize: 10
                }
                ToolButton {
                    objectName: "nodeDisplayButton"
                    Layout.fillWidth: true
                    visible: root.previewNodeId > 0 &&
                             root.previewNodeKind !== "Transform"
                    text: root.displayedNodeId === root.previewNodeId
                          ? "Show final output" : "Show on canvas"
                    onClicked: root.displayedNodeId === root.previewNodeId
                               ? root.finalDisplayRequested()
                               : root.displayRequested(root.previewNodeId,
                                     root.controller.compositionNodes.find(n => n.id === root.previewNodeId)?.kindCode || 0,
                                     root.previewLayerId)
                    Accessible.name: text
                }
            }
        }
        }
    }
}
