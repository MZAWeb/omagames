import QtQuick
import QtQuick.Layouts
import OmaGames

// Slim application bar: table size and destructive reset.
RowLayout {
    id: header
    signal newGameRequested()
    spacing: 7 * theme.textScale
    readonly property bool betting: game.phase === "betting"

    Text {
        text: "Black Omack"
        color: theme.foreground
        font.pixelSize: 17 * theme.textScale
        font.bold: true
    }
    Item { Layout.fillWidth: true }

    Text {
        visible: header.width > 680 * theme.textScale
        text: "Table mates"
        color: theme.foreground
        opacity: 0.75
        font.pixelSize: 10 * theme.textScale
    }
    OmaHintButton {
        text: "−"
        hint: "["
        enabled: header.betting && game.botCount > 0
        onClicked: game.setBotCount(game.botCount - 1)
        Layout.minimumWidth: implicitWidth
    }
    Text {
        text: game.botCount + " / " + game.maxBots
        color: theme.foreground
        font.pixelSize: 13 * theme.textScale
        font.bold: true
        horizontalAlignment: Text.AlignHCenter
        Layout.preferredWidth: 34 * theme.textScale
    }
    OmaHintButton {
        text: "+"
        hint: "]"
        enabled: header.betting && game.botCount < game.maxBots
        onClicked: game.setBotCount(game.botCount + 1)
        Layout.minimumWidth: implicitWidth
    }
    OmaHintButton {
        text: "New game"
        hint: "Ctrl+N"
        enabled: header.betting
        onClicked: header.newGameRequested()
        Layout.leftMargin: 6 * theme.textScale
        Layout.minimumWidth: implicitWidth
    }
}
