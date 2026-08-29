import QtQuick
import OmaGames

// An OmaButton with its keyboard shortcut shown as a keycap on the right, so
// every action on screen teaches its key. The badge is not interactive, so
// clicks fall through to the button underneath.
Item {
    id: root

    property alias text: button.text
    property alias primary: button.primary
    property string keyHint: ""
    property bool actionEnabled: true

    signal clicked()

    implicitWidth: button.implicitWidth
    implicitHeight: button.implicitHeight

    OmaButton {
        id: button
        anchors.fill: parent
        enabled: root.actionEnabled
        // Reserve the badge's slot so a narrow button's label never runs
        // underneath it.
        rightPadding: hint.visible ? hint.implicitWidth + 14 * theme.textScale : padding
        onClicked: root.clicked()
    }

    OmaKeyHint {
        id: hint
        anchors.right: parent.right
        anchors.rightMargin: 8 * theme.textScale
        anchors.verticalCenter: parent.verticalCenter
        key: root.keyHint
        onPrimary: button.primary
        active: root.actionEnabled
    }
}
