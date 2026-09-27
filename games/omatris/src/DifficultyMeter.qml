import QtQuick
import QtQuick.Layouts

// Challenge's difficulty, big: how hard the board left is to finish, 1 to
// 100, rated by the engine each time a piece settles. The colour runs green
// to yellow to red with it, and the small arrow says how far it has moved
// since the deal.
ColumnLayout {
    id: root

    spacing: 0

    // Shown counting rather than jumping, so a change reads as one.
    property int shown: game.difficulty
    Behavior on shown {
        NumberAnimation { duration: 280; easing.type: Easing.OutCubic }
    }
    // Measured from the deal, not the last piece: the arrow says whether the
    // run as a whole is winning or losing ground.
    readonly property int change: game.difficulty - game.dealtDifficulty

    readonly property real level: Math.max(0, Math.min(1, (game.difficulty - 1) / 99))
    readonly property color tone: root.level < 0.5
        ? theme.mix(theme.green, theme.yellow, root.level * 2)
        : theme.mix(theme.yellow, theme.red, (root.level - 0.5) * 2)

    Text {
        text: qsTr("Difficulty")
        color: theme.mix(theme.background, theme.foreground, 0.6)
        font.pixelSize: 12 * theme.textScale
    }
    RowLayout {
        spacing: 8 * theme.textScale
        Text {
            text: root.shown.toString()
            color: root.tone
            font.pixelSize: 54 * theme.textScale
            font.bold: true
        }
        Text {
            Layout.alignment: Qt.AlignBottom
            Layout.bottomMargin: 10 * theme.textScale
            visible: root.change !== 0
            text: root.change > 0 ? qsTr("▲ %1").arg(root.change) : qsTr("▼ %1").arg(-root.change)
            color: root.change > 0 ? theme.red : theme.green
            font.pixelSize: 14 * theme.textScale
            font.bold: true
        }
    }
    Text {
        Layout.fillWidth: true
        text: root.change === 0 ? qsTr("of 100") : qsTr("of 100 · %1 at the start").arg(game.dealtDifficulty)
        wrapMode: Text.WordWrap
        color: theme.mix(theme.background, theme.foreground, 0.45)
        font.pixelSize: 11 * theme.textScale
    }
}
