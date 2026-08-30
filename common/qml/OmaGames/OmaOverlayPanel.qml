import QtQuick
import QtQuick.Layouts
import OmaGames

// A dimmed cover with a centred panel: the shell of every overlay. It
// swallows clicks and keys so the screen underneath stays untouched. `dim`
// lightens the cover for the overlays that want that screen still readable,
// and `dismissable` lets a click anywhere but the panel back out.
Rectangle {
    id: root

    default property alias content: column.data
    property real dim: 0.82
    property real maxWidth: 340 * theme.textScale
    property bool dismissable: false
    signal dismissed()

    color: theme.alpha(theme.background, dim)
    focus: true

    MouseArea {
        anchors.fill: parent
        onClicked: if (root.dismissable) root.dismissed()
    }

    OmaPanel {
        anchors.centerIn: parent
        width: Math.min(parent.width - 48 * theme.textScale, root.maxWidth)
        height: column.implicitHeight + 2 * padding

        ColumnLayout {
            id: column
            anchors.fill: parent
            spacing: 12 * theme.textScale
        }
    }
}
