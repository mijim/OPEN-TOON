import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

Item {
    id: root
    required property var controller
    signal layerChosen(int layer)
    readonly property var selected: controller.layers.find(l => l.id === controller.selectedLayer)
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
                text: "Drag to reorder; Alt-drag a Drawing onto another to use it as cutter"
                color: "#777777"
                font.pixelSize: 10
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
                                      ? "#666666" : modelData.kind === "Cutter" ||
                                      modelData.kind === "Opacity" ||
                                      modelData.kind === "Bypassed opacity" ||
                                      modelData.kind === "Multiply" ||
                                      modelData.kind === "Screen" ||
                                      modelData.kind === "Add" ||
                                      modelData.kind === "Bypassed blend" ||
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
                            property real downX: 0
                            property real downY: 0
                            onPressed: mouse => {
                                dragging = false
                                dragCutter = (mouse.modifiers & Qt.AltModifier) !== 0
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
                                        root.controller.moveDrawingAfter(source, target)
                                    }
                                }
                            }
                            onCanceled: {
                                root.draggedLayer = 0
                                root.dropLayer = 0
                            }
                            onClicked: {
                                if (dragging)
                                    return
                                root.previewNodeId = modelData.id
                                root.previewNodeKind = modelData.kind
                                root.previewLayerId = modelData.layer
                                root.refreshPreview()
                                if (modelData.layer > 0)
                                    root.layerChosen(modelData.layer)
                            }
                            Accessible.name: modelData.layer > 0
                                             ? "Preview and select " + modelData.name + " composition source"
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
            }
        }
        }
    }
}
