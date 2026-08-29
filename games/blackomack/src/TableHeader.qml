import QtQuick
import QtQuick.Layouts

// Title, session stats and the table controls (bot count, new game).
RowLayout {
    id: header
    signal newGameRequested()
    spacing: 8 * theme.textScale
    readonly property bool betting: game.phase === "betting"

    Text {
        text: "Black Omack"
        color: theme.foreground
        font.pixelSize: 20 * theme.textScale
        font.bold: true
    }
    Text {
        text: game.handsPlayed + " hands  ·  " + (game.netResult >= 0 ? "+" : "−") + Math.abs(game.netResult) + " Ø"
        color: game.netResult >= 0 ? theme.green : theme.red
        opacity: 0.9
        font.pixelSize: 12 * theme.textScale
        Layout.leftMargin: 8 * theme.textScale
    }

    Item { Layout.fillWidth: true }

    Text {
        text: "Table mates"
        color: theme.muted
        font.pixelSize: 12 * theme.textScale
    }
    HintButton { text: "−"; hint: "["; enabled: header.betting && game.botCount > 0; onClicked: game.setBotCount(game.botCount - 1) }
    Text {
        text: game.botCount
        color: theme.foreground
        font.pixelSize: 16 * theme.textScale
        font.bold: true
        horizontalAlignment: Text.AlignHCenter
        Layout.preferredWidth: 20 * theme.textScale
    }
    HintButton { text: "+"; hint: "]"; enabled: header.betting && game.botCount < 5; onClicked: game.setBotCount(game.botCount + 1) }
    HintButton { text: "New game"; hint: "Ctrl+N"; enabled: header.betting; onClicked: header.newGameRequested(); Layout.leftMargin: 8 * theme.textScale }
}
