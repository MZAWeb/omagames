import QtQuick
import QtQuick.Layouts
import OmaGames

RowLayout {
    spacing: 14 * theme.textScale

    ColumnLayout {
        spacing: 0
        Text {
            text: game.score.toLocaleString(Qt.locale(), "f", 0)
            color: game.score > game.best ? theme.accent : theme.foreground
            font.pixelSize: 22 * theme.textScale
            font.bold: true
        }
        Text {
            text: qsTr("score")
            color: theme.mix(theme.background, theme.foreground, 0.52)
            font.pixelSize: 11 * theme.textScale
        }
    }

    ColumnLayout {
        spacing: 0
        Text {
            text: qsTr("Best %1").arg(game.best.toLocaleString(Qt.locale(), "f", 0))
            color: theme.foreground
            font.pixelSize: 14 * theme.textScale
            font.bold: true
        }
        Text {
            text: qsTr("%1 shots · %2 pegs").arg(game.shots).arg(game.pegCount)
            color: theme.mix(theme.background, theme.foreground, 0.52)
            font.pixelSize: 11 * theme.textScale
        }
    }

    Item { Layout.fillWidth: true }

    Text {
        text: game.ready ? qsTr("Aim and release") : qsTr("Ball in play")
        color: game.ready ? theme.accent : theme.mix(theme.background, theme.foreground, 0.55)
        font.pixelSize: 13 * theme.textScale
        font.bold: game.ready
    }

    OmaHintButton {
        text: qsTr("Pause")
        hint: qsTr("P")
        onClicked: game.pause()
    }
}
