import QtQuick
import QtQuick.Layouts

// Preset, mines left, the clock and the promise this board was drawn under.
RowLayout {
    id: root

    spacing: 16 * theme.textScale

    function formatTime(seconds) {
        var rest = seconds % 60;
        return Math.floor(seconds / 60) + ":" + (rest < 10 ? "0" : "") + rest;
    }

    ColumnLayout {
        spacing: 0
        Text {
            text: game.presetLabel
            color: theme.foreground
            font.pixelSize: 17 * theme.textScale
            font.bold: true
        }
        Text {
            text: qsTr("%1×%2 · %3 mines").arg(game.width).arg(game.height).arg(game.mines)
            color: theme.mix(theme.background, theme.foreground, 0.55)
            font.pixelSize: 11 * theme.textScale
        }
    }

    Item { Layout.fillWidth: true }

    // The generator's verdict, never a guess about it.
    Rectangle {
        id: badge
        Layout.alignment: Qt.AlignVCenter
        implicitWidth: badgeLabel.implicitWidth + 16 * theme.textScale
        implicitHeight: badgeLabel.implicitHeight + 7 * theme.textScale
        radius: height / 2
        color: game.noGuess ? theme.alpha(theme.green, 0.15) : theme.alpha(theme.foreground, 0.08)
        border.width: 1
        border.color: game.noGuess ? theme.alpha(theme.green, 0.45) : theme.alpha(theme.foreground, 0.2)

        Text {
            id: badgeLabel
            anchors.centerIn: parent
            text: game.noGuess ? qsTr("no guessing") : qsTr("may need a guess")
            color: game.noGuess ? theme.green : theme.mix(theme.background, theme.foreground, 0.55)
            font.pixelSize: 11 * theme.textScale
            font.bold: game.noGuess
        }
    }

    Item { Layout.fillWidth: true }

    RowLayout {
        spacing: 6 * theme.textScale
        Text {
            text: "⚑"
            color: theme.mix(theme.background, theme.foreground, 0.6)
            font.pixelSize: 15 * theme.textScale
        }
        Text {
            Layout.minimumWidth: 40 * theme.textScale
            horizontalAlignment: Text.AlignRight
            text: game.remainingMines
            color: game.remainingMines < 0 ? theme.red : theme.foreground
            font.pixelSize: 20 * theme.textScale
            font.bold: true
        }
    }

    Text {
        Layout.minimumWidth: 66 * theme.textScale
        horizontalAlignment: Text.AlignRight
        text: root.formatTime(game.elapsedSeconds)
        color: theme.foreground
        font.pixelSize: 20 * theme.textScale
        font.bold: true
    }
}
