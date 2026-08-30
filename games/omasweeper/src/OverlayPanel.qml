import QtQuick
import QtQuick.Layouts
import OmaGames

// A dimmed cover with a centred panel: the shell of every overlay. It
// swallows clicks and keys so the board underneath stays untouched. `dim`
// lightens the cover for the overlays that want the board still readable.
Rectangle {
    id: root

    default property alias content: column.data
    property real dim: 0.82

    color: theme.alpha(theme.background, dim)
    focus: true

    MouseArea { anchors.fill: parent }

    OmaPanel {
        anchors.centerIn: parent
        width: Math.min(parent.width - 48 * theme.textScale, 340 * theme.textScale)
        height: column.implicitHeight + 2 * padding

        ColumnLayout {
            id: column
            anchors.fill: parent
            spacing: 12 * theme.textScale
        }
    }
}
