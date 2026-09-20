import QtQuick
import QtQuick.Controls.Basic

CompactTextField {
    id: root
    required property real number
    required property string label
    signal committed(real value)
    implicitHeight: 26
    selectByMouse: true
    font.family: "Menlo"
    font.pixelSize: 11
    validator: DoubleValidator {
        locale: "C"
    }
    Accessible.name: label
    Binding {
        target: root
        property: "text"
        value: Number(root.number).toFixed(2)
        when: !root.activeFocus
    }
    onEditingFinished: {
        if (acceptableInput)
            committed(Number(text));
        focus = false;
    }
}
