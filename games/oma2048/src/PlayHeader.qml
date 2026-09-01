import QtQuick
import QtQuick.Layouts
import OmaGames

// The game's name, the actions as buttons wearing their keys, and the score
// against the best of the kept table.
RowLayout {
    id: root

    signal newGameRequested()
    signal scoresRequested()

    spacing: 12 * theme.textScale

    // The score counts up rather than jumping, so a big merge reads as one move.
    property int shownScore: game.score
    Behavior on shownScore {
        NumberAnimation { duration: 250; easing.type: Easing.OutCubic }
    }
    readonly property bool beatingBest: game.score > game.bestScore

    Text {
        text: qsTr("Oma2048")
        color: theme.foreground
        font.pixelSize: 17 * theme.textScale
        font.bold: true
    }

    Item { Layout.fillWidth: true }

    OmaHintButton {
        text: qsTr("Undo")
        hint: qsTr("U")
        enabled: game.canUndo
        focusPolicy: Qt.NoFocus
        onClicked: game.undo()
    }
    OmaHintButton {
        text: qsTr("New")
        hint: qsTr("N")
        focusPolicy: Qt.NoFocus
        onClicked: root.newGameRequested()
    }
    OmaHintButton {
        text: qsTr("Scores")
        hint: qsTr("S")
        focusPolicy: Qt.NoFocus
        onClicked: root.scoresRequested()
    }

    Item { Layout.fillWidth: true }

    ColumnLayout {
        spacing: 0
        Text {
            Layout.alignment: Qt.AlignRight
            Layout.minimumWidth: 90 * theme.textScale
            horizontalAlignment: Text.AlignRight
            text: root.shownScore.toLocaleString(Qt.locale(), "f", 0)
            color: root.beatingBest ? theme.accent : theme.foreground
            font.pixelSize: 20 * theme.textScale
            font.bold: true
            Behavior on color { ColorAnimation { duration: 200 } }
        }
        Text {
            Layout.alignment: Qt.AlignRight
            text: qsTr("Best %1").arg(game.bestScore.toLocaleString(Qt.locale(), "f", 0))
            color: theme.mix(theme.background, theme.foreground, 0.55)
            font.pixelSize: 11 * theme.textScale
        }
    }
}
