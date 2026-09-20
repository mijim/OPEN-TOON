import QtQuick
import QtQuick.Controls.Basic

Canvas {
    id: root
    objectName: "poseKeyStrip"
    required property var controller
    required property int firstFrame
    required property int lastFrame
    property real plotLeft: 48
    property real plotRight: width - 20
    readonly property real keyY: 17
    readonly property var keys: controller.animationKeys
    readonly property var selection: controller.selectedPoseFrames
    readonly property var selectedSet: new Set(selection)
    readonly property int selectionFirst: selection.length ? selection[0] : -1
    readonly property int selectionLast: selection.length ? selection[selection.length - 1] : -1
    readonly property bool gestureActive: pointer.mode !== ""
    function px(frame) {
        return plotLeft + (frame - firstFrame) / Math.max(1, lastFrame - firstFrame) * (plotRight - plotLeft);
    }
    function frameAt(x) {
        return Math.max(0, Math.min(controller.duration - 1, Math.round(firstFrame + (x - plotLeft) / (plotRight - plotLeft) * (lastFrame - firstFrame))));
    }
    function keyAt(x, y) {
        let nearest = -1, distance = 13;
        for (const key of keys) {
            const d = Math.hypot(px(key.frame) - x, keyY - y);
            if (key.frame >= firstFrame && key.frame <= lastFrame && d < distance) {
                nearest = key.frame;
                distance = d;
            }
        }
        return nearest;
    }
    function stretchAt(x, y) {
        return selection.length > 1 && Math.abs(px(selectionLast) + 10 - x) < 5 && Math.abs(y - keyY) < 12;
    }
    function previewFrames() {
        if (pointer.mode === "move")
            return selection.map(f => f + pointer.offset);
        if (pointer.mode === "stretch")
            return selection.map(f => selectionFirst + Math.round((f - selectionFirst) * (pointer.end - selectionFirst) / (selectionLast - selectionFirst)));
        return [];
    }
    function validPreview() {
        const frames = previewFrames();
        if (!frames.length)
            return true;
        const occupied = new Set(keys.filter(k => pointer.duplicate || !selectedSet.has(k.frame)).map(k => k.frame));
        const seen = new Set();
        for (const frame of frames) {
            if (frame < 0 || frame >= controller.duration || occupied.has(frame) || seen.has(frame))
                return false;
            seen.add(frame);
        }
        return true;
    }
    function cancel() {
        pointer.mode = "";
        pointer.moved = false;
        requestPaint();
    }
    function diamond(ctx, x, y, filled) {
        ctx.beginPath();
        ctx.moveTo(x, y - 5);
        ctx.lineTo(x + 5, y);
        ctx.lineTo(x, y + 5);
        ctx.lineTo(x - 5, y);
        ctx.closePath();
        if (filled)
            ctx.fill();
        else
            ctx.stroke();
    }
    onFirstFrameChanged: cancel()
    onLastFrameChanged: cancel()
    onVisibleChanged: cancel()
    onWidthChanged: requestPaint()
    onHeightChanged: requestPaint()
    onPaint: {
        const ctx = getContext("2d");
        ctx.reset();
        ctx.fillStyle = "#0b0b0b";
        ctx.fillRect(0, 0, width, height);
        ctx.font = "10px sans-serif";
        ctx.fillStyle = "#888888";
        ctx.fillText("Keys", 3, keyY + 3);
        ctx.save();
        ctx.beginPath();
        ctx.rect(plotLeft - 8, 0, plotRight - plotLeft + 23, height);
        ctx.clip();
        if (selection.length > 1) {
            ctx.fillStyle = "#242424";
            ctx.fillRect(px(selectionFirst) - 7, 4, px(selectionLast) - px(selectionFirst) + 21, 25);
            ctx.fillStyle = "#aaaaaa";
            ctx.fillRect(px(selectionLast) + 9, 7, 3, 20);
        }
        if (pointer.mode === "range") {
            ctx.fillStyle = "#343434";
            ctx.fillRect(Math.min(pointer.origin.x, pointer.rangeX), 3, Math.abs(pointer.rangeX - pointer.origin.x), 26);
        }
        for (const key of keys) {
            const selected = selectedSet.has(key.frame);
            ctx.fillStyle = selected ? "#ffffff" : "#777777";
            diamond(ctx, px(key.frame), keyY, true);
            if (key.frame === controller.frame) {
                ctx.strokeStyle = "#eeeeee";
                ctx.strokeRect(px(key.frame) - 8, keyY - 8, 16, 16);
            }
        }
        if (pointer.mode === "move" || pointer.mode === "stretch") {
            ctx.strokeStyle = validPreview() ? "#ffffff" : "#777777";
            for (const frame of previewFrames())
                diamond(ctx, px(frame), keyY, false);
        }
        for (let i = 0; i <= 4; ++i) {
            const f = Math.round(firstFrame + (lastFrame - firstFrame) * i / 4);
            ctx.fillStyle = "#888888";
            ctx.fillText(String(f + 1), px(f) - 3, height - 2);
        }
        ctx.restore();
    }
    MouseArea {
        id: pointer
        objectName: "poseKeyStripInput"
        anchors.fill: parent
        hoverEnabled: true
        preventStealing: true
        cursorShape: (mode === "move" || mode === "stretch") && !root.validPreview() ? Qt.ForbiddenCursor : mode === "stretch" || root.stretchAt(mouseX, mouseY) ? Qt.SizeHorCursor : mode === "move" ? Qt.ClosedHandCursor : root.keyAt(mouseX, mouseY) >= 0 ? Qt.OpenHandCursor : Qt.CrossCursor
        property string mode: ""
        property point origin
        property int offset: 0
        property int end: 0
        property real rangeX: 0
        property bool duplicate: false
        property bool additive: false
        property bool moved: false
        onPressed: mouse => {
            forceActiveFocus();
            origin = Qt.point(mouse.x, mouse.y);
            moved = false;
            offset = 0;
            duplicate = !!(mouse.modifiers & Qt.AltModifier);
            additive = !!(mouse.modifiers & (Qt.ControlModifier | Qt.MetaModifier));
            if (root.stretchAt(mouse.x, mouse.y) && !(mouse.modifiers & Qt.ShiftModifier)) {
                end = root.selectionLast;
                mode = "stretch";
                return;
            }
            const frame = root.keyAt(mouse.x, mouse.y);
            if (frame >= 0) {
                root.controller.selectPoseKey(frame, !!(mouse.modifiers & Qt.ShiftModifier), additive);
                if (root.selectedSet.has(frame))
                    mode = "move";
            } else {
                mode = "range";
                rangeX = mouse.x;
                root.controller.frame = root.frameAt(mouse.x);
            }
            root.requestPaint();
        }
        onPositionChanged: mouse => {
            if (!pressed || mode === "")
                return;
            if (Math.hypot(mouse.x - origin.x, mouse.y - origin.y) > 3)
                moved = true;
            if (mode === "move" && moved) {
                const delta = Math.round((mouse.x - origin.x) / (root.plotRight - root.plotLeft) * (root.lastFrame - root.firstFrame));
                offset = Math.max(-root.selectionFirst, Math.min(root.controller.duration - 1 - root.selectionLast, delta));
            } else if (mode === "stretch" && moved) {
                const delta = Math.round((mouse.x - origin.x) / (root.plotRight - root.plotLeft) * (root.lastFrame - root.firstFrame));
                end = Math.max(root.selectionFirst + 1, Math.min(root.controller.duration - 1, root.selectionLast + delta));
            } else if (mode === "range")
                rangeX = mouse.x;
            root.requestPaint();
        }
        onReleased: {
            const action = mode, delta = offset, last = end, copy = duplicate, changed = moved;
            const from = root.frameAt(origin.x), to = root.frameAt(rangeX), add = additive;
            root.cancel();
            if (action === "move" && changed && delta !== 0)
                root.controller.moveSelectedPoseKeys(delta, copy);
            else if (action === "stretch" && changed)
                root.controller.stretchSelectedPoseKeys(last);
            else if (action === "range") {
                if (changed)
                    root.controller.selectPoseRange(from, to, add);
                else if (!add)
                    root.controller.clearPoseSelection();
            }
        }
        onDoubleClicked: mouse => {
            if (root.keyAt(mouse.x, mouse.y) >= 0 || root.stretchAt(mouse.x, mouse.y))
                return;
            root.cancel();
            root.controller.frame = root.frameAt(mouse.x);
            root.controller.addKey();
            root.controller.clearPoseSelection();
            root.controller.selectPoseKey(root.controller.frame);
        }
        onCanceled: root.cancel()
        Keys.onEscapePressed: event => {
            if (mode !== "")
                root.cancel();
            else
                root.controller.clearPoseSelection();
            event.accepted = true;
        }
        ToolTip.visible: containsMouse && !pressed && root.stretchAt(mouseX, mouseY)
        ToolTip.delay: 500
        ToolTip.text: "Drag to stretch selected keys. The first key stays fixed."
        Accessible.name: "Pose key selection. Shift-click selects a span; Command or Control-click toggles keys. Drag blank space to select. Drag keys to move, Alt-drag to duplicate, or drag the right selection edge to stretch timing."
    }
    Connections {
        target: root.controller
        function onChanged() {
            root.cancel();
        }
        function onFrameChanged() {
            root.requestPaint();
        }
        function onKeySelectionChanged() {
            root.cancel();
        }
    }
}
