import QtQuick
import QtQuick.Layouts
import OmaGames

// The top ten of each speed, for one edges setting at a time: the same two
// groups as the start screen, so a table always says which rules it belongs to.
OmaScoresPanel {
    id: root

    title: qsTr("High scores")
    categories: game.difficulties
    entries: game.highScores.filter(function(entry) { return entry.mode === game.mode; })
    detailText: function(entry) { return qsTr("%1 long").arg(entry.length); }

    EdgesSelector {
        Layout.alignment: Qt.AlignHCenter
        Layout.preferredWidth: Math.min(root.width - 2 * root.padding, 320 * theme.textScale)
    }

    SectionHeading {
        Layout.fillWidth: true
        Layout.topMargin: 4 * theme.textScale
        text: qsTr("Speed")
    }
}
