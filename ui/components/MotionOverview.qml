import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import "." as C

Item {
    id: root
    objectName: "motionOverview"
    required property var controller
    required property var evaluateEase
    property int firstFrame: 0
    property int lastFrame: Math.max(1, controller.duration - 1)
    property var keys: controller.animationKeys
    property var curves: []
    property string activeChannel: "x"
    property bool linkEasing: true
    readonly property int curveCount: curves.length
    readonly property var activeCurve: curves.find(c => c.key === activeChannel) || null
    readonly property var selected: keys.find(k => k.frame === controller.frame) || null
    readonly property var segmentEnd: selected ? keys.find(k => k.frame > selected.frame) || null : null
    readonly property var channels: [
        {
            key: "x",
            name: "X"
        },
        {
            key: "y",
            name: "Y"
        },
        {
            key: "rotation",
            name: "Rotation"
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
            key: "opacity",
            name: "Opacity"
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
    function storedHandles(key, channel) {
        return key.easing[channel] || (key.interpolation === 2 ? [1 / 3, 0, 2 / 3, 1] : [1 / 3, 1 / 3, 2 / 3, 2 / 3]);
    }
    function refresh() {
        if (!controller || !graph)
            return;
        let result = [];
        for (const channel of channels) {
            const samples = controller.curveSamples(channel.key);
            let low = Infinity, high = -Infinity;
            for (const p of samples) {
                low = Math.min(low, p.value);
                high = Math.max(high, p.value);
            }
            if (high - low <= 1e-9)
                continue;
            // Include control points so overshoot handles remain reachable after fitting.
            for (let i = 0; i + 1 < keys.length; ++i) {
                const a = keys[i], b = keys[i + 1], h = storedHandles(a, channel.key);
                for (const y of [h[1], h[3]]) {
                    const value = a[channel.key] + y * (b[channel.key] - a[channel.key]);
                    low = Math.min(low, value);
                    high = Math.max(high, value);
                }
            }
            const pad = (high - low) * .12;
            result.push({
                key: channel.key,
                name: channel.name,
                samples: samples,
                low: low - pad,
                high: high + pad
            });
        }
        curves = result;
        if (!curves.some(c => c.key === activeChannel) && curves.length)
            activeChannel = curves[0].key;
        graph.requestPaint();
    }
    function tangent(index) {
        if (!activeCurve || !selected || !segmentEnd || Math.abs(segmentEnd[activeChannel] - selected[activeChannel]) < 1e-9)
            return null;
        const h = pointer.dragHandle >= 0 ? pointer.draft : storedHandles(selected, activeChannel);
        return Qt.point(graph.px(selected.frame + h[index * 2] * (segmentEnd.frame - selected.frame)), graph.py(selected[activeChannel] + h[index * 2 + 1] * (segmentEnd[activeChannel] - selected[activeChannel]), activeCurve));
    }
    function tangentAt(x, y) {
        for (let i = 0; i < 2; ++i) {
            const p = tangent(i);
            if (p && Math.hypot(p.x - x, p.y - y) <= 12)
                return i;
        }
        return -1;
    }
    onActiveChannelChanged: {
        if (pointer)
            pointer.cancel();
        if (graph)
            graph.requestPaint();
    }
    onLinkEasingChanged: if (pointer)
        pointer.cancel()
    onFirstFrameChanged: if (graph) {
        pointer.cancel();
        graph.requestPaint();
    }
    onLastFrameChanged: if (graph) {
        pointer.cancel();
        graph.requestPaint();
    }
    onVisibleChanged: {
        if (pointer)
            pointer.cancel();
        if (visible)
            refresh();
    }
    Component.onCompleted: refresh()
    Connections {
        target: root.controller
        function onChanged() {
            pointer.cancel();
            if (root.visible)
                root.refresh();
        }
        function onFrameChanged() {
            if (pointer.sourceFrame >= 0 && root.controller.frame !== pointer.sourceFrame)
                pointer.cancel();
            graph.requestPaint();
        }
    }
    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        RowLayout {
            spacing: 3
            Label {
                text: "Motion · normalized"
                color: "#888888"
                font.pixelSize: 10
            }
            Repeater {
                model: root.curves
                C.ToolButton {
                    required property var modelData
                    text: modelData.name
                    active: root.activeChannel === modelData.key
                    implicitHeight: 22
                    hint: "Edit " + modelData.name + " keys and handles in All motion"
                    onClicked: root.activeChannel = modelData.key
                }
            }
            C.ToolButton {
                objectName: "linkMotionEasing"
                text: "Link easing"
                active: root.linkEasing
                implicitHeight: 22
                hint: "When enabled, dragging a round handle eases all pose channels together. Off edits only the highlighted channel. Square keys change only its value; diamonds move the full pose in time."
                onClicked: root.linkEasing = !root.linkEasing
            }
            Label {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignRight
                elide: Text.ElideRight
                text: "Squares: values · circles: easing · diamonds: time"
                color: "#888888"
                font.pixelSize: 10
            }
        }
        Canvas {
            id: graph
            objectName: "combinedMotionCanvas"
            Layout.fillWidth: true
            Layout.fillHeight: true
            readonly property real plotLeft: 48
            readonly property real plotRight: width - 20
            readonly property real plotTop: 12
            readonly property real plotBottom: Math.max(plotTop + 10, height - 48)
            readonly property real keyY: height - 25
            function px(frame) {
                return plotLeft + (frame - root.firstFrame) / Math.max(1, root.lastFrame - root.firstFrame) * (plotRight - plotLeft);
            }
            function py(value, curve) {
                return plotBottom - (value - curve.low) / (curve.high - curve.low) * (plotBottom - plotTop);
            }
            function pointFor(frame, value) {
                return Qt.point(px(frame), py(value, root.activeCurve));
            }
            function valueAt(y) {
                const c = root.activeCurve;
                return c.low + (plotBottom - y) / (plotBottom - plotTop) * (c.high - c.low);
            }
            function fractionalFrame(x) {
                return root.firstFrame + (x - plotLeft) / (plotRight - plotLeft) * (root.lastFrame - root.firstFrame);
            }
            function frameAt(x) {
                return Math.max(0, Math.min(root.controller.duration - 1, Math.round(fractionalFrame(x))));
            }
            function keyAt(x, y, lane) {
                if (!lane && !root.activeCurve)
                    return null;
                let nearest = null, distance = 14;
                for (const k of root.keys) {
                    if (k.frame < root.firstFrame || k.frame > root.lastFrame)
                        continue;
                    const d = Math.hypot(px(k.frame) - x, (lane ? keyY : py(k[root.activeChannel], root.activeCurve)) - y);
                    if (d < distance) {
                        nearest = k;
                        distance = d;
                    }
                }
                return nearest;
            }
            function curveAt(x, y) {
                const frame = fractionalFrame(x);
                let nearest = null, distance = 10;
                // Prefer the active channel when normalized curves overlap.
                const ordered = root.curves.filter(c => c.key !== root.activeChannel).concat(root.activeCurve ? [root.activeCurve] : []);
                for (const curve of ordered) {
                    for (let i = 1; i < curve.samples.length; ++i) {
                        const a = curve.samples[i - 1], b = curve.samples[i];
                        if (frame < a.frame || frame > b.frame)
                            continue;
                        const value = a.value + (b.value - a.value) * (frame - a.frame) / Math.max(1, b.frame - a.frame);
                        const d = Math.abs(py(value, curve) - y);
                        if (d <= distance) {
                            distance = d;
                            nearest = curve;
                        }
                        break;
                    }
                }
                return nearest;
            }
            onPaint: {
                const ctx = getContext("2d");
                ctx.reset();
                ctx.fillStyle = "#0b0b0b";
                ctx.fillRect(0, 0, width, height);
                ctx.font = "10px sans-serif";
                for (let i = 0; i <= 2; ++i) {
                    const y = plotBottom - (plotBottom - plotTop) * i / 2;
                    ctx.strokeStyle = "#282828";
                    ctx.beginPath();
                    ctx.moveTo(plotLeft, y);
                    ctx.lineTo(plotRight, y);
                    ctx.stroke();
                    ctx.fillStyle = "#777777";
                    ctx.fillText(String(i * 50) + "%", 3, y + 3);
                }
                ctx.save();
                ctx.beginPath();
                ctx.rect(plotLeft - 7, plotTop - 7, plotRight - plotLeft + 14, keyY - plotTop + 14);
                ctx.clip();
                const ordered = root.curves.filter(c => c.key !== root.activeChannel).concat(root.activeCurve ? [root.activeCurve] : []);
                ordered.forEach((curve, index) => {
                    const active = curve.key === root.activeChannel;
                    ctx.strokeStyle = active ? "#eeeeee" : "#777777";
                    ctx.lineWidth = active ? 1.8 : 1.2;
                    ctx.setLineDash(active ? [] : index % 2 ? [2, 3] : [6, 3]);
                    ctx.beginPath();
                    curve.samples.forEach((p, i) => {
                        let value = p.value;
                        if (pointer.dragHandle >= 0 && root.selected && root.segmentEnd && (root.linkEasing || active) && p.frame >= root.selected.frame && p.frame <= root.segmentEnd.frame) {
                            const a = root.selected, b = root.segmentEnd;
                            value = a[curve.key] + (b[curve.key] - a[curve.key]) * root.evaluateEase((p.frame - a.frame) / (b.frame - a.frame), pointer.draft);
                            if (curve.key === "opacity")
                                value = Math.max(0, Math.min(1, value));
                        }
                        if (i === 0)
                            ctx.moveTo(px(p.frame), py(value, curve));
                        else
                            ctx.lineTo(px(p.frame), py(value, curve));
                    });
                    ctx.stroke();
                });
                ctx.setLineDash([]);
                ctx.lineWidth = 1;
                ctx.strokeStyle = "#555555";
                ctx.beginPath();
                ctx.moveTo(px(root.controller.frame), plotTop);
                ctx.lineTo(px(root.controller.frame), keyY + 7);
                ctx.stroke();
                for (const k of root.keys) {
                    ctx.fillStyle = k.frame === root.controller.frame ? "#ffffff" : "#999999";
                    if (root.activeCurve)
                        ctx.fillRect(px(k.frame) - 5, py(k[root.activeChannel], root.activeCurve) - 5, 10, 10);
                }
                if (root.activeCurve && root.selected && root.segmentEnd) {
                    ctx.strokeStyle = "#cccccc";
                    ctx.fillStyle = "#151515";
                    for (let i = 0; i < 2; ++i) {
                        const h = root.tangent(i), k = i === 0 ? root.selected : root.segmentEnd;
                        if (!h)
                            continue;
                        ctx.beginPath();
                        ctx.moveTo(px(k.frame), py(k[root.activeChannel], root.activeCurve));
                        ctx.lineTo(h.x, h.y);
                        ctx.stroke();
                        ctx.beginPath();
                        ctx.arc(h.x, h.y, 6, 0, 2 * Math.PI);
                        ctx.fill();
                        ctx.stroke();
                    }
                }
                if (pointer.sourceFrame >= 0 && pointer.dragHandle < 0) {
                    ctx.strokeStyle = "#ffffff";
                    if (pointer.dragValue && root.activeCurve)
                        ctx.strokeRect(px(pointer.targetFrame) - 7, py(pointer.targetValue, root.activeCurve) - 7, 14, 14);
                }
                ctx.restore();
                for (let i = 0; i <= 4; ++i) {
                    const f = Math.round(root.firstFrame + (root.lastFrame - root.firstFrame) * i / 4);
                    ctx.fillStyle = "#888888";
                    ctx.fillText(String(f + 1), px(f) - 3, height - 4);
                }
                if (!root.curves.length) {
                    ctx.fillStyle = "#aaaaaa";
                    ctx.fillText("No animated change yet. Move to a frame and pose with Animate (A).", plotLeft + 12, plotTop + 20);
                }
            }
            MouseArea {
                id: pointer
                anchors.fill: parent
                hoverEnabled: true
                preventStealing: true
                cursorShape: sourceFrame >= 0 ? Qt.ClosedHandCursor : root.tangentAt(mouseX, mouseY) >= 0 || graph.keyAt(mouseX, mouseY, true) || graph.keyAt(mouseX, mouseY, false) ? Qt.OpenHandCursor : graph.curveAt(mouseX, mouseY) ? Qt.PointingHandCursor : Qt.CrossCursor
                property int sourceFrame: -1
                property int targetFrame: 0
                property real targetValue: 0
                property int interpolation: 0
                property int dragHandle: -1
                property var draft: []
                property bool dragValue: false
                property bool moved: false
                property point origin
                function cancel() {
                    sourceFrame = -1;
                    dragHandle = -1;
                    dragValue = false;
                    moved = false;
                    graph.requestPaint();
                }
                onPressed: mouse => {
                    forceActiveFocus();
                    origin = Qt.point(mouse.x, mouse.y);
                    moved = false;
                    const handle = root.tangentAt(mouse.x, mouse.y);
                    if (handle >= 0) {
                        draft = root.storedHandles(root.selected, root.activeChannel).slice();
                        sourceFrame = root.selected.frame;
                        dragHandle = handle;
                        return;
                    }
                    const key = graph.keyAt(mouse.x, mouse.y, false);
                    if (key) {
                        root.controller.clearPoseSelection();
                        root.controller.selectPoseKey(key.frame);
                        sourceFrame = key.frame;
                        targetFrame = key.frame;
                        dragValue = true;
                        targetValue = key[root.activeChannel];
                        interpolation = key.interpolation;
                    } else {
                        const curve = graph.curveAt(mouse.x, mouse.y);
                        if (curve) {
                            root.activeChannel = curve.key;
                            const frame = graph.frameAt(mouse.x), previous = root.keys.filter(k => k.frame <= frame);
                            root.controller.frame = previous.length ? previous[previous.length - 1].frame : frame;
                        } else
                            root.controller.frame = graph.frameAt(mouse.x);
                    }
                    graph.requestPaint();
                }
                onPositionChanged: mouse => {
                    if (!pressed)
                        return;
                    if (Math.hypot(mouse.x - origin.x, mouse.y - origin.y) > 3)
                        moved = true;
                    if (sourceFrame >= 0 && moved) {
                        if (dragHandle >= 0) {
                            const a = root.selected, b = root.segmentEnd, i = dragHandle * 2;
                            if (!a || !b)
                                return;
                            let h = draft.slice();
                            h[i] = Math.max(dragHandle === 0 ? 0 : h[0], Math.min(dragHandle === 0 ? h[2] : 1, (graph.fractionalFrame(mouse.x) - a.frame) / (b.frame - a.frame)));
                            h[i + 1] = Math.max(-4, Math.min(4, (graph.valueAt(mouse.y) - a[root.activeChannel]) / (b[root.activeChannel] - a[root.activeChannel])));
                            draft = h;
                        } else {
                            targetFrame = Math.max(0, Math.min(root.controller.duration - 1, sourceFrame + Math.round((mouse.x - origin.x) / (graph.plotRight - graph.plotLeft) * (root.lastFrame - root.firstFrame))));
                            if (dragValue)
                                targetValue = root.selected[root.activeChannel] + (origin.y - mouse.y) / (graph.plotBottom - graph.plotTop) * (root.activeCurve.high - root.activeCurve.low);
                        }
                    } else if (sourceFrame < 0 && moved)
                        root.controller.frame = graph.frameAt(mouse.x);
                    graph.requestPaint();
                }
                onReleased: {
                    const source = sourceFrame, destination = targetFrame, value = targetValue, isValue = dragValue, handle = dragHandle, h = draft.slice(), didMove = moved, mode = interpolation;
                    cancel();
                    if (source < 0 || !didMove)
                        return;
                    if (handle >= 0) {
                        if (root.linkEasing)
                            root.controller.setPoseCurveHandles(source, h[0], h[1], h[2], h[3]);
                        else
                            root.controller.setCurveHandles(source, root.activeChannel, h[0], h[1], h[2], h[3]);
                    } else if (isValue)
                        root.controller.updateKey(source, destination, root.activeChannel, value, mode);
                }
                onCanceled: cancel()
                onDoubleClicked: mouse => {
                    if (root.tangentAt(mouse.x, mouse.y) >= 0 || graph.keyAt(mouse.x, mouse.y, true) || graph.keyAt(mouse.x, mouse.y, false))
                        return;
                    cancel();
                    if (root.activeCurve && mouse.y < graph.plotBottom + 10)
                        root.controller.addCurveKey(graph.frameAt(mouse.x), root.activeChannel, graph.valueAt(mouse.y));
                    else {
                        root.controller.frame = graph.frameAt(mouse.x);
                        root.controller.addKey();
                    }
                }
                Keys.onEscapePressed: event => {
                    cancel();
                    event.accepted = true;
                }
                Accessible.name: "All motion editor. Choose a channel in the legend. Drag square keys to edit, round handles to ease, or diamonds to retime poses."
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
    }
}
