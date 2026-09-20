import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

Dialog {
    id: root
    objectName: "curveEditorDialog"
    required property var controller
    title: "Animation curves"
    width: Math.min(960, parent.width - 40)
    height: Math.min(680, parent.height - 40)
    anchors.centerIn: parent
    modal: true
    standardButtons: Dialog.Close
    property var keys: controller.animationKeys
    property var samples: []
    property string channel: channels.currentValue || "x"
    property real low: -1
    property real high: 1
    property var selected: null
    function refresh() {
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
        let pad = Math.max((hi - lo) * 0.15, channel === "opacity" ? 0.1 : 1);
        low = lo - pad;
        high = hi + pad;
        selectCurrent();
        graph.requestPaint();
    }
    function selectCurrent() {
        selected = keys.find(k => k.frame === controller.frame) || null;
        if (selected) {
            keyFrame.value = selected.frame + 1;
            keyValue.text = String(selected[channel]);
            easing.currentIndex = selected.interpolation;
        }
    }
    onOpened: refresh()
    onChannelChanged: refresh()
    Connections {
        target: root.controller
        function onChanged() {
            if (root.visible)
                root.refresh();
        }
        function onFrameChanged() {
            root.selectCurrent();
            graph.requestPaint();
        }
    }
    contentItem: ColumnLayout {
        spacing: 12
        RowLayout {
            ComboBox {
                id: channels
                textRole: "name"
                valueRole: "key"
                model: [
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
                Layout.preferredWidth: 220
                Accessible.name: "Curve channel"
            }
            Button {
                text: "Previous key"
                onClicked: root.controller.nextKey(-1)
            }
            Button {
                text: "Next key"
                onClicked: root.controller.nextKey(1)
            }
            Button {
                text: "Fit values"
                onClicked: root.refresh()
            }
            Label {
                text: "Frame " + (root.controller.frame + 1)
                Layout.fillWidth: true
            }
        }
        Label {
            text: "Click to scrub. Drag a key to change its time and this value. Each key stores a full pose; timing and interpolation affect all channels."
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
            color: "#aaaaaa"
        }
        Canvas {
            id: graph
            objectName: "animationCurveCanvas"
            Layout.fillWidth: true
            Layout.fillHeight: true
            property real plotLeft: 76
            property real plotRight: width - 20
            property real plotTop: 20
            property real plotBottom: height - 30
            function px(frame) {
                return plotLeft + frame / Math.max(1, root.controller.duration - 1) * (plotRight - plotLeft);
            }
            function py(value) {
                return plotBottom - (value - root.low) / (root.high - root.low) * (plotBottom - plotTop);
            }
            function frameAt(x) {
                return Math.round(Math.max(0, Math.min(1, (x - plotLeft) / (plotRight - plotLeft))) * Math.max(0, root.controller.duration - 1));
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
                for (let i = 0; i <= 4; ++i) {
                    let y = plotTop + (plotBottom - plotTop) * i / 4;
                    ctx.strokeStyle = "#292929";
                    ctx.beginPath();
                    ctx.moveTo(plotLeft, y);
                    ctx.lineTo(plotRight, y);
                    ctx.stroke();
                    ctx.fillStyle = "#aaaaaa";
                    ctx.fillText((root.high - (root.high - root.low) * i / 4).toFixed(2), 4, y + 4);
                    let f = Math.round((root.controller.duration - 1) * i / 4), x = px(f);
                    ctx.fillText(String(f + 1), x - 5, height - 8);
                }
                ctx.strokeStyle = "#dddddd";
                ctx.lineWidth = 1.5;
                ctx.beginPath();
                root.samples.forEach((p, i) => {
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
                    ctx.fillRect(x - 4, y - 4, 8, 8);
                }
                if (pointer.dragKey) {
                    ctx.strokeStyle = "#ffffff";
                    ctx.strokeRect(px(pointer.targetFrame) - 6, py(pointer.targetValue) - 6, 12, 12);
                }
            }
            MouseArea {
                id: pointer
                anchors.fill: parent
                property var dragKey: null
                property int targetFrame: 0
                property real targetValue: 0
                property bool moved: false
                property point origin
                onPressed: mouse => {
                    forceActiveFocus();
                    moved = false;
                    origin = Qt.point(mouse.x, mouse.y);
                    dragKey = root.keys.find(k => Math.abs(graph.px(k.frame) - mouse.x) < 9 && Math.abs(graph.py(k[root.channel]) - mouse.y) < 9) || null;
                    if (dragKey) {
                        targetFrame = dragKey.frame;
                        targetValue = dragKey[root.channel];
                        root.controller.frame = dragKey.frame;
                    } else
                        root.controller.frame = graph.frameAt(mouse.x);
                }
                onPositionChanged: mouse => {
                    if (!pressed)
                        return;
                    if (dragKey) {
                        if (Math.abs(mouse.x - origin.x) + Math.abs(mouse.y - origin.y) > 3)
                            moved = true;
                        if (moved) {
                            targetFrame = graph.frameAt(mouse.x);
                            targetValue = graph.valueAt(mouse.y);
                        }
                    } else
                        root.controller.frame = graph.frameAt(mouse.x);
                    graph.requestPaint();
                }
                onReleased: {
                    const key = dragKey;
                    dragKey = null;
                    if (key && moved)
                        root.controller.updateKey(key.frame, targetFrame, root.channel, targetValue, key.interpolation);
                    graph.requestPaint();
                }
                onCanceled: {
                    dragKey = null;
                    graph.requestPaint();
                }
                Keys.onEscapePressed: event => {
                    if (dragKey) {
                        dragKey = null;
                        graph.requestPaint();
                        event.accepted = true;
                    } else
                        root.close();
                }
                Accessible.name: "Animation curve. Use the numeric controls to edit keys without dragging."
            }
        }
        RowLayout {
            Label {
                text: "Key frame"
            }
            SpinBox {
                id: keyFrame
                from: 1
                to: 1000000
                editable: true
                enabled: root.selected !== null
                Accessible.name: "Key frame"
            }
            TextField {
                id: keyValue
                Layout.preferredWidth: 120
                enabled: root.selected !== null
                validator: DoubleValidator {
                    locale: "C"
                }
                Accessible.name: "Key value"
            }
            ComboBox {
                id: easing
                model: ["Linear", "Hold", "Smooth"]
                enabled: root.selected !== null
                Accessible.name: "Outgoing interpolation"
            }
            Button {
                text: "Apply key"
                enabled: root.selected !== null && keyValue.acceptableInput
                onClicked: root.controller.updateKey(root.selected.frame, keyFrame.value - 1, root.channel, Number(keyValue.text), easing.currentIndex)
            }
            Button {
                text: "Add key"
                onClicked: root.controller.addKey(easing.currentIndex)
            }
            Button {
                text: "Delete"
                enabled: root.selected !== null
                onClicked: root.controller.deleteKey()
            }
        }
        Label {
            text: "Retime timeline selection: frames " + (root.controller.rangeStart + 1) + "–" + root.controller.rangeEnd + " · " + root.controller.selectedLayers.length + " layers"
        }
        RowLayout {
            Label {
                text: "Destination"
            }
            SpinBox {
                id: destination
                from: 1
                to: 1000000
                value: root.controller.rangeStart + 1
                editable: true
                Accessible.name: "Key range destination"
            }
            Label {
                text: "Length"
            }
            SpinBox {
                id: length
                from: 1
                to: 1000000
                value: root.controller.rangeEnd - root.controller.rangeStart
                editable: true
                Accessible.name: "Key range length"
            }
            Button {
                text: "Retime keys only"
                onClicked: root.controller.retimeSelectedKeys(destination.value - 1, length.value)
            }
        }
        Label {
            text: root.controller.status
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            color: "#bbbbbb"
        }
    }
}
