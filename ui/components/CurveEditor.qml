import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import "." as C

FocusScope {
    id: root
    objectName: "curveEditorPanel"
    required property var controller
    property var keys: controller.animationKeys
    property var samples: []
    property string channel: channels.currentValue || "all"
    readonly property bool combined: channel === "all"
    function selectChannel(value) {
        for (let i = 0; i < channels.model.length; ++i)
            if (channels.model[i].key === value) {
                channels.currentIndex = i;
                return;
            }
    }
    property int timeZoom: 1
    property int timeStart: 0
    readonly property int timeSpan: Math.max(1, Math.ceil((controller.duration - 1) / timeZoom))
    readonly property int firstFrame: Math.max(0, Math.min(timeStart, controller.duration - 1 - timeSpan))
    readonly property int lastFrame: firstFrame + timeSpan
    function zoomTime(factor) {
        timeZoom = Math.max(1, Math.min(64, timeZoom * factor));
        timeStart = Math.max(0, controller.frame - Math.floor(timeSpan / 2));
    }
    onFirstFrameChanged: if (graph)
        graph.requestPaint()
    onLastFrameChanged: if (graph)
        graph.requestPaint()
    property real low: -1
    property real high: 1
    property bool showNumbers: false
    property var selected: null
    property var segmentEnd: selected ? keys.find(k => k.frame > selected.frame) || null : null
    function handles() {
        if (!selected)
            return [1 / 3, 1 / 3, 2 / 3, 2 / 3];
        if (pointer.dragHandle >= 0)
            return pointer.draft;
        return selected.easing[channel] || (selected.interpolation === 2 ? [1 / 3, 0, 2 / 3, 1] : [1 / 3, 1 / 3, 2 / 3, 2 / 3]);
    }
    function preset(values) {
        if (selected && segmentEnd) {
            if (combined)
                controller.setPoseCurveHandles(selected.frame, values[0], values[1], values[2], values[3]);
            else
                controller.setCurveHandles(selected.frame, channel, values[0], values[1], values[2], values[3]);
        }
    }
    function tangent(index) {
        if (!selected || !segmentEnd || Math.abs(segmentEnd[channel] - selected[channel]) < 1e-9)
            return null;
        const h = handles();
        return Qt.point(graph.px(selected.frame + h[index * 2] * (segmentEnd.frame - selected.frame)), graph.py(selected[channel] + h[index * 2 + 1] * (segmentEnd[channel] - selected[channel])));
    }
    function tangentAt(x, y) {
        for (let i = 0; i < 2; ++i) {
            const p = tangent(i);
            if (p && Math.hypot(p.x - x, p.y - y) <= 10)
                return i;
        }
        return -1;
    }
    function previewEase(t, h) {
        const cubic = (u, a, b) => 3 * (1 - u) * (1 - u) * u * a + 3 * (1 - u) * u * u * b + u * u * u;
        let lo = 0, hi = 1;
        for (let i = 0; i < 24; ++i) {
            let m = (lo + hi) / 2;
            if (cubic(m, h[0], h[2]) < t)
                lo = m;
            else
                hi = m;
        }
        return cubic((lo + hi) / 2, h[1], h[3]);
    }
    function refresh() {
        if (!controller || !graph)
            return;
        if (channel === "all") {
            selectCurrent();
            if (overview)
                overview.refresh();
            return;
        }
        samples = controller.curveSamples(channel);
        let lo = Infinity, hi = -Infinity;
        for (const p of samples) {
            lo = Math.min(lo, p.value);
            hi = Math.max(hi, p.value);
        }
        if (!isFinite(lo)) {
            lo = 0;
            hi = 0;
        }
        for (let i = 0; i + 1 < keys.length; ++i) {
            const k = keys[i], end = keys[i + 1], h = k.easing[channel];
            if (h)
                for (const y of [h[1], h[3]]) {
                    const value = k[channel] + y * (end[channel] - k[channel]);
                    lo = Math.min(lo, value);
                    hi = Math.max(hi, value);
                }
        }
        let pad = Math.max((hi - lo) * 0.15, channel === "opacity" ? 0.1 : 1);
        low = lo - pad;
        high = hi + pad;
        selectCurrent();
        graph.requestPaint();
    }
    function selectCurrent() {
        selected = keys.find(k => k.frame === controller.frame) || null;
        if (selected && !combined) {
            keyFrame.value = selected.frame + 1;
            keyValue.text = Number(selected[channel]).toFixed(3);
            keyValue.valueEdited = false;
            easing.currentIndex = selected.interpolation;
        }
    }
    onVisibleChanged: if (visible)
        refresh()
    Component.onCompleted: refresh()
    onChannelChanged: Qt.callLater(refresh)
    Connections {
        target: root.controller
        function onChanged() {
            pointer.dragHandle = -1;
            pointer.dragKey = null;
            if (root.visible)
                root.refresh();
        }
        function onFrameChanged() {
            if (pointer.dragHandle >= 0 && root.controller.frame !== pointer.handleFrame)
                pointer.dragHandle = -1;
            if (pointer.dragKey && root.controller.frame !== pointer.dragKey.frame)
                pointer.dragKey = null;
            root.selectCurrent();
            graph.requestPaint();
        }
    }
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 6
        spacing: 4
        RowLayout {
            CompactComboBox {
                id: channels
                textRole: "name"
                valueRole: "key"
                objectName: "curveChannelSelector"
                model: [
                    {
                        name: "All motion",
                        key: "all"
                    },
                    {
                        name: "Position X · px",
                        key: "x"
                    },
                    {
                        name: "Position Y · px",
                        key: "y"
                    },
                    {
                        name: "Rotation · degrees",
                        key: "rotation"
                    },
                    {
                        name: "Scale X · ratio",
                        key: "scaleX"
                    },
                    {
                        name: "Scale Y · ratio",
                        key: "scaleY"
                    },
                    {
                        name: "Opacity · 0–1",
                        key: "opacity"
                    },
                    {
                        name: "Pivot X · px",
                        key: "pivotX"
                    },
                    {
                        name: "Pivot Y · px",
                        key: "pivotY"
                    }
                ]
                Layout.preferredWidth: 155
                Accessible.name: "Curve channel"
            }
            C.ToolButton {
                text: "◇"
                hint: "Animate layer on canvas · A"
                active: root.controller.tool === "Animate"
                onClicked: root.controller.tool = "Animate"
            }
            C.ToolButton {
                text: "‹|"
                hint: "Previous key"
                onClicked: root.controller.nextKey(-1)
            }
            C.ToolButton {
                text: "|›"
                hint: "Next key"
                onClicked: root.controller.nextKey(1)
            }
            C.ToolButton {
                text: "Fit"
                hint: "Fit full duration and curve values"
                onClicked: {
                    root.timeZoom = 1;
                    root.timeStart = 0;
                    root.refresh();
                }
            }
            C.ToolButton {
                text: "−"
                hint: "Zoom out in time"
                enabled: root.timeZoom > 1
                onClicked: root.zoomTime(.5)
            }
            Label {
                text: root.timeZoom + "×"
                color: "#888888"
                font.pixelSize: 10
            }
            C.ToolButton {
                text: "+"
                hint: "Zoom in around current frame"
                enabled: root.timeZoom < 64
                onClicked: root.zoomTime(2)
            }
            Rectangle {
                Layout.preferredWidth: 1
                Layout.preferredHeight: 16
                color: "#333333"
            }
            C.ToolButton {
                text: "Linear"
                hint: root.combined ? "Linear transition for all pose channels" : "Linear outgoing channel curve"
                enabled: root.segmentEnd !== null
                onClicked: root.preset([1 / 3, 1 / 3, 2 / 3, 2 / 3])
            }
            C.ToolButton {
                text: "Ease"
                hint: root.combined ? "Ease all pose channels together" : "Ease in / out"
                enabled: root.segmentEnd !== null
                onClicked: root.preset([1 / 3, 0, 2 / 3, 1])
            }
            C.ToolButton {
                text: "Overshoot"
                hint: root.combined ? "Overshoot for all pose channels" : "Editable overshoot"
                enabled: root.segmentEnd !== null
                onClicked: root.preset([.25, 0, .65, 1.8])
            }
            Label {
                text: (root.controller.selectedPoseFrames.length > 1 ? root.controller.selectedPoseFrames.length + " keys · " : "") + "Frame " + (root.controller.frame + 1) + (root.selected && root.selected.easing[root.channel] ? " · Bézier" : "")
                color: "#999999"
                font.pixelSize: 10
                Layout.fillWidth: true
                elide: Text.ElideRight
            }
            C.ToolButton {
                text: "+ Key"
                hint: "Add pose key at current frame"
                onClicked: root.controller.addKey()
            }
            C.ToolButton {
                text: "− Key"
                hint: "Delete selected pose keys, or the current key when no keys are selected"
                enabled: root.selected !== null || root.controller.selectedPoseFrames.length > 0
                onClicked: root.controller.selectedPoseFrames.length ? root.controller.deleteSelectedPoseKeys() : root.controller.deleteKey()
            }
            C.ToolButton {
                text: "Copy"
                hint: "Copy selected pose keys in local layer units, including pivots"
                enabled: root.controller.selectedPoseFrames.length > 0
                onClicked: root.controller.copyPoseKeys()
            }
            C.ToolButton {
                text: "Paste"
                hint: "Paste pose keys at the playhead on the selected layer; existing keys are protected"
                enabled: root.controller.hasPoseClipboard
                onClicked: root.controller.pastePoseKeys()
            }
            C.ToolButton {
                text: "Keys ▾"
                hint: "Interpolation and repetition for the selected full-pose block"
                enabled: root.controller.selectedPoseFrames.length > 0
                onClicked: keyBlockMenu.open()
                Menu {
                    id: keyBlockMenu
                    MenuItem { text: "Linear segments"; onTriggered: root.controller.interpolateSelectedPoseKeys(0) }
                    MenuItem { text: "Hold poses"; onTriggered: root.controller.interpolateSelectedPoseKeys(1) }
                    MenuItem { text: "Smooth segments"; onTriggered: root.controller.interpolateSelectedPoseKeys(2) }
                    MenuItem { text: "Ease in / out"; onTriggered: root.controller.interpolateSelectedPoseKeys(3) }
                    MenuItem { text: "Overshoot"; onTriggered: root.controller.interpolateSelectedPoseKeys(4) }
                    MenuItem { text: "Fast start / soft stop"; onTriggered: root.controller.interpolateSelectedPoseKeys(5) }
                    MenuSeparator {}
                    MenuItem { text: "Append one copy"; onTriggered: root.controller.repeatSelectedPoseKeys(1) }
                    MenuItem { text: "Append three copies"; onTriggered: root.controller.repeatSelectedPoseKeys(3) }
                }
            }
            C.ToolButton {
                text: "Values"
                enabled: !root.combined
                hint: "Show precise numeric key controls"
                active: root.showNumbers
                onClicked: root.showNumbers = !root.showNumbers
            }
            C.ToolButton {
                text: "?"
                hint: "Drag square keys or round handles. Double-click to add a key. Escape cancels. Add a middle key to create a bounce between equal values."
            }
        }
        MotionOverview {
            id: overview
            controller: root.controller
            firstFrame: root.firstFrame
            lastFrame: root.lastFrame
            visible: root.combined
            Layout.fillWidth: true
            Layout.fillHeight: true
            evaluateEase: root.previewEase
        }
        Canvas {
            id: graph
            visible: !root.combined
            objectName: "animationCurveCanvas"
            Layout.fillWidth: true
            Layout.fillHeight: true
            property real plotLeft: 64
            property real plotRight: width - 20
            property real plotTop: 12
            property real plotBottom: height - 64
            function px(frame) {
                return plotLeft + (frame - root.firstFrame) / Math.max(1, root.lastFrame - root.firstFrame) * (plotRight - plotLeft);
            }
            function py(value) {
                return plotBottom - (value - root.low) / (root.high - root.low) * (plotBottom - plotTop);
            }
            function frameAt(x) {
                return Math.min(root.controller.duration - 1, root.firstFrame + Math.round(Math.max(0, Math.min(1, (x - plotLeft) / (plotRight - plotLeft))) * (root.lastFrame - root.firstFrame)));
            }
            function keyAt(x, y) {
                let nearest = null, distance = 14;
                for (const k of root.keys) {
                    if (k.frame < root.firstFrame || k.frame > root.lastFrame)
                        continue;
                    const d = Math.hypot(px(k.frame) - x, py(k[root.channel]) - y);
                    if (d < distance) {
                        nearest = k;
                        distance = d;
                    }
                }
                return nearest;
            }
            function valueAt(y) {
                return root.low + (plotBottom - y) / (plotBottom - plotTop) * (root.high - root.low);
            }
            onPaint: {
                const ctx = getContext("2d");
                ctx.reset();
                ctx.fillStyle = "#0b0b0b";
                ctx.fillRect(0, 0, width, height);
                ctx.font = "11px sans-serif";
                const ticks = Math.max(1, Math.min(4, Math.floor((plotBottom - plotTop) / 30)));
                for (let i = 0; i <= ticks; ++i) {
                    let y = plotTop + (plotBottom - plotTop) * i / ticks;
                    ctx.strokeStyle = "#292929";
                    ctx.beginPath();
                    ctx.moveTo(plotLeft, y);
                    ctx.lineTo(plotRight, y);
                    ctx.stroke();
                    ctx.fillStyle = "#aaaaaa";
                    ctx.fillText((root.high - (root.high - root.low) * i / ticks).toFixed(2), 4, y + 4);
                    let f = Math.round(root.firstFrame + (root.lastFrame - root.firstFrame) * i / ticks), x = px(f);
                    ctx.fillText(String(f + 1), x - 5, height - 8);
                }
                ctx.save();
                ctx.beginPath();
                ctx.rect(plotLeft - 7, plotTop - 7, plotRight - plotLeft + 14, plotBottom - plotTop + 14);
                ctx.clip();
                ctx.strokeStyle = "#dddddd";
                ctx.lineWidth = 1.5;
                ctx.beginPath();
                root.samples.forEach((p, i) => {
                    if (pointer.dragHandle >= 0 && root.selected && root.segmentEnd && p.frame >= root.selected.frame && p.frame <= root.segmentEnd.frame) {
                        let value = root.selected[root.channel] + (root.segmentEnd[root.channel] - root.selected[root.channel]) * root.previewEase((p.frame - root.selected.frame) / (root.segmentEnd.frame - root.selected.frame), pointer.draft);
                        if (root.channel === "opacity")
                            value = Math.max(0, Math.min(1, value));
                        p = {
                            frame: p.frame,
                            value: value
                        };
                    }
                    if (i === 0)
                        ctx.moveTo(px(p.frame), py(p.value));
                    else
                        ctx.lineTo(px(p.frame), py(p.value));
                });
                ctx.stroke();
                ctx.strokeStyle = "#777777";
                ctx.beginPath();
                ctx.moveTo(px(root.controller.frame), plotTop);
                ctx.lineTo(px(root.controller.frame), plotBottom);
                ctx.stroke();
                for (const k of root.keys) {
                    let x = px(k.frame), y = py(k[root.channel]);
                    ctx.fillStyle = k.frame === root.controller.frame ? "#ffffff" : "#888888";
                    ctx.fillRect(x - 5, y - 5, 10, 10);
                }
                if (root.selected && root.segmentEnd) {
                    ctx.strokeStyle = "#888888";
                    ctx.fillStyle = "#151515";
                    for (let i = 0; i < 2; ++i) {
                        const h = root.tangent(i), k = i === 0 ? root.selected : root.segmentEnd;
                        if (!h)
                            continue;
                        ctx.beginPath();
                        ctx.moveTo(px(k.frame), py(k[root.channel]));
                        ctx.lineTo(h.x, h.y);
                        ctx.stroke();
                        ctx.beginPath();
                        ctx.arc(h.x, h.y, 5, 0, 2 * Math.PI);
                        ctx.fill();
                        ctx.stroke();
                    }
                }
                if (pointer.dragKey) {
                    ctx.strokeStyle = "#ffffff";
                    ctx.strokeRect(px(pointer.targetFrame) - 7, py(pointer.targetValue) - 7, 14, 14);
                }
                ctx.restore();
            }
            MouseArea {
                id: pointer
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: pressed ? Qt.ClosedHandCursor : root.tangentAt(mouseX, mouseY) >= 0 || graph.keyAt(mouseX, mouseY) ? Qt.OpenHandCursor : Qt.CrossCursor
                property int dragHandle: -1
                property int handleFrame: -1
                property var draft: []
                property var dragKey: null
                property int targetFrame: 0
                property real targetValue: 0
                property bool moved: false
                property point origin
                onPressed: mouse => {
                    forceActiveFocus();
                    dragHandle = root.tangentAt(mouse.x, mouse.y);
                    if (dragHandle >= 0) {
                        handleFrame = root.selected.frame;
                        // Read stored handles before exposing the draft through handles().
                        draft = (root.selected.easing[root.channel] || (root.selected.interpolation === 2 ? [1 / 3, 0, 2 / 3, 1] : [1 / 3, 1 / 3, 2 / 3, 2 / 3])).slice();
                        return;
                    }
                    moved = false;
                    origin = Qt.point(mouse.x, mouse.y);
                    dragKey = graph.keyAt(mouse.x, mouse.y);
                    if (dragKey) {
                        root.controller.clearPoseSelection();
                        root.controller.selectPoseKey(dragKey.frame);
                        targetFrame = dragKey.frame;
                        targetValue = dragKey[root.channel];
                        root.controller.frame = dragKey.frame;
                    } else
                        root.controller.frame = graph.frameAt(mouse.x);
                }
                onPositionChanged: mouse => {
                    if (!pressed)
                        return;
                    if (dragHandle >= 0) {
                        const a = root.selected, b = root.segmentEnd, i = dragHandle * 2;
                        let h = draft.slice();
                        let frame = root.firstFrame + (mouse.x - graph.plotLeft) / (graph.plotRight - graph.plotLeft) * (root.lastFrame - root.firstFrame);
                        h[i] = Math.max(dragHandle === 0 ? 0 : h[0], Math.min(dragHandle === 0 ? h[2] : 1, (frame - a.frame) / (b.frame - a.frame)));
                        h[i + 1] = Math.max(-4, Math.min(4, (graph.valueAt(mouse.y) - a[root.channel]) / (b[root.channel] - a[root.channel])));
                        draft = h;
                        graph.requestPaint();
                        return;
                    }
                    if (dragKey) {
                        if (Math.abs(mouse.x - origin.x) + Math.abs(mouse.y - origin.y) > 3)
                            moved = true;
                        if (moved) {
                            targetFrame = Math.max(0, Math.min(root.controller.duration - 1, dragKey.frame + Math.round((mouse.x - origin.x) / (graph.plotRight - graph.plotLeft) * (root.lastFrame - root.firstFrame))));
                            targetValue = dragKey[root.channel] + (origin.y - mouse.y) / (graph.plotBottom - graph.plotTop) * (root.high - root.low);
                        }
                    } else
                        root.controller.frame = graph.frameAt(mouse.x);
                    graph.requestPaint();
                }
                onDoubleClicked: mouse => {
                    if (!graph.keyAt(mouse.x, mouse.y) && root.tangentAt(mouse.x, mouse.y) < 0)
                        root.controller.addCurveKey(graph.frameAt(mouse.x), root.channel, graph.valueAt(mouse.y));
                }
                onReleased: {
                    if (dragHandle >= 0) {
                        const h = draft.slice(), frame = root.selected.frame;
                        dragHandle = -1;
                        root.controller.setCurveHandles(frame, root.channel, h[0], h[1], h[2], h[3]);
                        graph.requestPaint();
                        return;
                    }
                    const key = dragKey;
                    dragKey = null;
                    if (key && moved)
                        root.controller.updateKey(key.frame, targetFrame, root.channel, targetValue, key.interpolation);
                    graph.requestPaint();
                }
                onCanceled: {
                    dragHandle = -1;
                    dragKey = null;
                    graph.requestPaint();
                }
                Keys.onEscapePressed: event => {
                    if (dragKey || dragHandle >= 0) {
                        dragHandle = -1;
                        dragKey = null;
                        graph.requestPaint();
                        event.accepted = true;
                    }
                }
                Accessible.name: "Animation curve. Use the numeric controls to edit keys without dragging."
            }
            PoseKeyStrip {
                controller: root.controller
                firstFrame: root.firstFrame
                lastFrame: root.lastFrame
                plotLeft: graph.plotLeft
                plotRight: graph.plotRight
                y: graph.height - 42
                width: graph.width
                height: 42
            }
        }
        ScrollBar {
            id: timeScroll
            objectName: "curveTimeScroll"
            visible: root.timeZoom > 1
            orientation: Qt.Horizontal
            Layout.fillWidth: true
            Layout.preferredHeight: 12
            size: Math.min(1, root.timeSpan / Math.max(1, root.controller.duration - 1))
            Binding {
                target: timeScroll
                property: "position"
                value: root.firstFrame / Math.max(1, root.controller.duration - 1)
                when: !timeScroll.pressed
            }
            onPositionChanged: if (pressed)
                root.timeStart = Math.round(position * (root.controller.duration - 1))
        }
        RowLayout {
            visible: root.showNumbers && !root.combined
            Label {
                text: "Key frame"
            }
            CompactSpinBox {
                id: keyFrame
                from: 1
                to: 1000000
                editable: true
                enabled: root.selected !== null
                Accessible.name: "Key frame"
            }
            CompactTextField {
                id: keyValue
                property bool valueEdited: false
                onTextEdited: valueEdited = true
                Layout.preferredWidth: 100
                enabled: root.selected !== null
                validator: DoubleValidator {
                    locale: "C"
                }
                Accessible.name: "Key value"
            }
            CompactComboBox {
                id: easing
                model: ["Base: Linear", "Base: Hold", "Base: Smooth"]
                enabled: root.selected !== null
                Accessible.name: "Base pose interpolation. Changing it resets all channel handles on this key."
            }
            CompactButton {
                text: "Apply"
                enabled: root.selected !== null && keyValue.acceptableInput
                onClicked: root.controller.updateKey(root.selected.frame, keyFrame.value - 1, root.channel, keyValue.valueEdited ? Number(keyValue.text) : root.selected[root.channel], easing.currentIndex)
            }
            CompactButton {
                text: "Add key"
                onClicked: root.controller.addKey(easing.currentIndex)
            }
            CompactButton {
                text: "Delete"
                enabled: root.selected !== null
                onClicked: root.controller.deleteKey()
            }
        }
    }
}
