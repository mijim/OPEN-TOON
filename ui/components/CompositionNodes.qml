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
        (n.kind === "Group output" || n.kind === "Bypassed group") &&
        n.group === selectedGroup)
    readonly property bool selectedGroupLocked: controller.layers.some(l =>
        l.compositeGroup === selectedGroup && l.locked)
    property int groupStartLayer: 0
    property int previewNodeId: 0
    property string previewNodeKind: ""
    property int previewLayerId: 0
    property string previewData: ""
    property int draggedLayer: 0
    property int draggedGroup: 0
    property int dropLayer: 0
    property int matchIndex: -1
    readonly property int cardWidth: 148
    readonly property int cardGap: 8
    readonly property var operators: [
        {id: "drawing", name: "Drawing", category: "Source", description: "Add an empty drawing layer."},
        {id: "normal", name: "Normal", category: "Composite", description: "Use ordinary source-over blending."},
        {id: "multiply", name: "Multiply", category: "Composite", description: "Darken overlapping colors."},
        {id: "screen", name: "Screen", category: "Composite", description: "Lighten overlapping colors."},
        {id: "add", name: "Add", category: "Composite", description: "Add overlapping colors with a clamp."},
        {id: "inside", name: "Inside cutter", category: "Matte", description: "Keep coverage inside the assigned cutter."},
        {id: "outside", name: "Outside cutter", category: "Matte", description: "Keep coverage outside the assigned cutter."},
        {id: "group", name: "Group span", category: "Structure", description: "Group from the marked drawing to the selection."}
    ]
    readonly property var matchingOperators: {
        const query = operatorSearch ? operatorSearch.text.trim().toLowerCase() : ""
        return query ? operators.filter(op =>
            op.name.toLowerCase().includes(query) ||
            op.category.toLowerCase().includes(query)) : operators
    }
    readonly property var matchingNodeIds: {
        const query = nodeSearch ? nodeSearch.text.trim().toLowerCase() : ""
        return query ? controller.compositionNodes.filter(n =>
            n.name.toLowerCase().includes(query) || n.kind.toLowerCase().includes(query))
            .map(n => n.id) : []
    }
    function toggleNodeBypass(node) {
        if (node.group > 0 && (node.kind === "Group output" ||
                               node.kind === "Bypassed group")) {
            root.controller.setCompositeGroupBypassed(node.group,
                                                       node.kind === "Group output")
            return true
        }
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
    function canApplyOperator(id) {
        if (id === "drawing")
            return true
        if (id === "group")
            return groupStartLayer > 0 && selected && selected.id !== groupStartLayer &&
                   (selected.kind === 0 || selected.kind === 3)
        return selected && !selected.locked && (selected.kind === 0 || selected.kind === 3) &&
               (id !== "inside" && id !== "outside" || selected.matte > 0)
    }
    function applyOperator(id) {
        if (!canApplyOperator(id))
            return false
        if (id === "drawing") {
            controller.addLayer()
            return true
        }
        if (id === "group") {
            const first = groupStartLayer
            const grouped = controller.groupDrawings(first, selected.id)
            if (grouped)
                groupStartLayer = 0
            return grouped
        }
        if (id === "inside" || id === "outside")
            return controller.setMatteInverted(id === "outside")
        const modes = {normal: 0, multiply: 1, screen: 2, add: 3}
        return modes[id] === undefined ? false : controller.setLayerBlendMode(modes[id])
    }
    function nudgeSelectedGroup(direction) {
        const ordered = controller.layers.filter(l => l.kind === 0 || l.kind === 3).reverse()
        const members = ordered.filter(l => l.compositeGroup === selectedGroup)
        if (members.length === 0)
            return
        const boundary = direction < 0 ? members[0] : members[members.length - 1]
        const neighbor = ordered[ordered.findIndex(l => l.id === boundary.id) + direction]
        if (neighbor)
            controller.moveCompositeGroup(selectedGroup, neighbor.id, direction < 0)
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
                text: root.selectedGroup > 0
                      ? "Alt+Shift-click: edit members"
                      : "Drag: front · Shift-drag: behind · Alt-drag: cutter · Shift-click: group"
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
                objectName: "nodeBack"
                text: "Back"
                enabled: root.selected && (root.selected.kind === 0 || root.selected.kind === 3)
                onClicked: root.selectedGroup > 0 ? root.nudgeSelectedGroup(-1)
                                                   : root.controller.moveLayer(-1)
                Accessible.name: "Move selected drawing or group backward in composite order"
            }
            ToolButton {
                objectName: "nodeFront"
                text: "Front"
                enabled: root.selected && (root.selected.kind === 0 || root.selected.kind === 3)
                onClicked: root.selectedGroup > 0 ? root.nudgeSelectedGroup(1)
                                                   : root.controller.moveLayer(1)
                Accessible.name: "Move selected drawing or group forward in composite order"
            }
            ToolButton {
                objectName: "nodeDuplicateGroup"
                text: "Duplicate"
                visible: root.selectedGroup > 0
                enabled: visible && !root.selectedGroupLocked
                onClicked: root.controller.duplicateCompositeGroup(root.selectedGroup)
                Accessible.name: "Duplicate selected composite group with independent artwork"
            }
            ToolButton {
                objectName: "nodeUngroup"
                text: "Ungroup"
                visible: root.selectedGroup > 0
                onClicked: root.controller.ungroupDrawings(root.selectedGroup)
                Accessible.name: "Ungroup selected composite drawings"
            }
            CompactCheckBox {
                objectName: "nodeBypassGroup"
                text: "Bypass group"
                visible: root.selectedGroup > 0
                enabled: visible && !root.selectedGroupLocked
                checked: root.selected?.compositeGroupBypassed || false
                onClicked: root.controller.setCompositeGroupBypassed(root.selectedGroup, checked)
                Accessible.name: "Bypass selected composite group"
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
            ToolButton {
                objectName: "nodeOperatorButton"
                text: "Add operator"
                onClicked: operatorPopup.open()
                Accessible.name: "Open composition operator library"
            }
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
                        readonly property int movableGroup: modelData.kind === "Group output" ||
                                                            modelData.kind === "Bypassed group"
                                                            ? modelData.group : 0
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
                            preventStealing: modelData.kind === "Drawing" || movableGroup > 0
                            property bool dragging: false
                            property bool dragCutter: false
                            property bool dragBehind: false
                            property real downX: 0
                            property real downY: 0
                            onPressed: mouse => {
                                dragging = false
                                dragCutter = modelData.kind === "Drawing" &&
                                             (mouse.modifiers & Qt.AltModifier) !== 0
                                dragBehind = !dragCutter && (mouse.modifiers & Qt.ShiftModifier) !== 0
                                downX = mouse.x
                                downY = mouse.y
                            }
                            onPositionChanged: mouse => {
                                if (!pressed || (modelData.kind !== "Drawing" && movableGroup === 0))
                                    return
                                if (!dragging && Math.hypot(mouse.x - downX, mouse.y - downY) > 8) {
                                    dragging = true
                                    root.draggedLayer = modelData.layer
                                    root.draggedGroup = movableGroup
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
                                const group = root.draggedGroup
                                const target = root.dropLayer
                                root.draggedLayer = 0
                                root.draggedGroup = 0
                                root.dropLayer = 0
                                if (dragging && target > 0) {
                                    if (group > 0) {
                                        root.controller.moveCompositeGroup(group, target, dragBehind)
                                    } else if (dragCutter) {
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
                                root.draggedGroup = 0
                                root.dropLayer = 0
                            }
                            onClicked: mouse => {
                                if (dragging)
                                    return
                                if ((mouse.modifiers & (Qt.AltModifier | Qt.ShiftModifier)) ===
                                        (Qt.AltModifier | Qt.ShiftModifier) &&
                                    modelData.kind === "Drawing" && root.selectedGroup > 0) {
                                    const panel = root
                                    const group = panel.selectedGroup
                                    const layer = modelData.layer
                                    if (panel.controller.toggleCompositeGroupMember(group, layer))
                                        panel.layerChosen(layer)
                                    return
                                }
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
                ToolButton {
                    objectName: "nodeDeleteSource"
                    Layout.fillWidth: true
                    visible: root.previewNodeKind === "Drawing" && root.previewLayerId > 0
                    text: "Delete source…"
                    onClicked: {
                        deleteSourcePopup.sourceLayer = root.previewLayerId
                        deleteSourcePopup.open()
                    }
                    Accessible.name: "Choose how to delete the previewed Drawing or Part source"
                }
            }
        }
        }
    }
    Popup {
        id: deleteSourcePopup
        objectName: "nodeDeleteSourcePopup"
        property int sourceLayer: 0
        parent: root
        x: Math.max(8, root.width - width - 12)
        y: 76
        width: 320
        height: 174
        padding: 10
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        background: Rectangle {
            color: "#171717"
            border.color: "#555555"
            radius: 5
        }
        contentItem: ColumnLayout {
            spacing: 6
            Label {
                text: "DELETE COMPOSITION SOURCE"
                color: "#eeeeee"
                font.pixelSize: 11
                font.bold: true
            }
            Label {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                text: "Choose what happens to cutter users and group boundaries."
                color: "#999999"
                font.pixelSize: 10
            }
            ToolButton {
                objectName: "nodeDeleteProtect"
                Layout.fillWidth: true
                text: "Protect references · reject if used"
                onClicked: {
                    if (root.controller.deleteCompositionSource(deleteSourcePopup.sourceLayer, false)) {
                        root.previewNodeId = 0
                        deleteSourcePopup.close()
                    }
                }
                Accessible.name: "Delete source only when no graph references use it"
            }
            ToolButton {
                objectName: "nodeDeleteDisconnect"
                Layout.fillWidth: true
                text: "Disconnect references and delete"
                onClicked: {
                    if (root.controller.deleteCompositionSource(deleteSourcePopup.sourceLayer, true)) {
                        root.previewNodeId = 0
                        deleteSourcePopup.close()
                    }
                }
                Accessible.name: "Disconnect cutter users and group boundaries, then delete source"
            }
        }
    }
    Popup {
        id: operatorPopup
        objectName: "nodeOperatorPopup"
        parent: root
        x: Math.max(8, root.width - width - 12)
        y: 76
        width: 340
        height: Math.min(350, Math.max(170, root.height - 82))
        padding: 8
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        onOpened: operatorSearch.forceActiveFocus()
        background: Rectangle {
            color: "#171717"
            border.color: "#555555"
            radius: 5
        }
        contentItem: ColumnLayout {
            spacing: 6
            Label {
                text: "OPERATORS"
                color: "#aaaaaa"
                font.pixelSize: 10
                font.letterSpacing: 1
            }
            CompactTextField {
                id: operatorSearch
                objectName: "nodeOperatorSearch"
                Layout.fillWidth: true
                placeholderText: "Find by name or category"
                Accessible.name: "Search composition operators by name or category"
            }
            ListView {
                objectName: "nodeOperatorList"
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                model: root.matchingOperators
                delegate: Rectangle {
                    required property var modelData
                    objectName: "nodeOperator" + modelData.id
                    width: ListView.view.width
                    height: 52
                    color: operatorMouse.containsMouse ? "#292929" : "#1d1d1d"
                    border.color: "#333333"
                    Column {
                        anchors.fill: parent
                        anchors.margins: 6
                        spacing: 2
                        Label {
                            text: modelData.category.toUpperCase() + "  /  " + modelData.name
                            color: root.canApplyOperator(modelData.id) ? "#eeeeee" : "#777777"
                            font.pixelSize: 11
                        }
                        Label {
                            text: modelData.description
                            color: "#888888"
                            font.pixelSize: 10
                            elide: Text.ElideRight
                            width: parent.width
                        }
                    }
                    MouseArea {
                        id: operatorMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        enabled: root.canApplyOperator(modelData.id)
                        cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
                        onClicked: {
                            const operatorId = modelData.id
                            operatorPopup.close()
                            root.applyOperator(operatorId)
                        }
                        Accessible.name: modelData.category + ": " + modelData.name + ". " + modelData.description
                    }
                }
                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
            }
        }
    }
}
