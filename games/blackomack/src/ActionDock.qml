import QtQuick
import QtQuick.Layouts
import OmaGames

// Fixed phase dock: context and event line stay put while only valid commands
// for the current phase are instantiated.
OmaPanel {
    id: dock
    readonly property bool typing: bets.typing
    readonly property real gap: 7 * theme.textScale
    padding: 10 * theme.textScale
    implicitHeight: content.implicitHeight + 2 * padding
    color: theme.mix(theme.darkBackground, theme.foreground, 0.035)
    border.color: theme.alpha(theme.foreground, 0.24)

    function focusBet() { bets.focusBet(); }

    Column {
        id: content
        width: parent.width
        spacing: 7 * theme.textScale

        RowLayout {
            width: parent.width
            spacing: 10 * theme.textScale
            // Round, shoe depth and your session net read as one sentence, so
            // they sit in their own tight row rather than spread across the dock.
            Row {
                spacing: 5 * theme.textScale
                Text {
                    text: "Round " + (game.handsPlayed + 1) + " · Shoe " + game.shoePercent + "%"
                    color: theme.foreground
                    opacity: 0.78
                    font.pixelSize: 11 * theme.textScale
                    font.bold: true
                }
                Text {
                    text: "· " + (game.netResult > 0 ? "+Ø" : game.netResult < 0 ? "−Ø" : "Ø")
                          + Math.abs(game.netResult)
                    color: game.netResult > 0 ? theme.green
                           : game.netResult < 0 ? theme.red : theme.foreground
                    opacity: 0.9
                    font.pixelSize: 11 * theme.textScale
                    font.bold: game.netResult !== 0
                }
            }
            Text {
                text: game.message
                color: theme.foreground
                opacity: 0.78
                font.pixelSize: 11 * theme.textScale
                elide: Text.ElideRight
                horizontalAlignment: Text.AlignRight
                Layout.fillWidth: true
            }
        }

        BetControls {
            id: bets
            visible: game.phase === "betting"
            width: parent.width
        }

        Flow {
            id: actionFlow
            visible: game.waitingForHuman
            width: parent.width
            spacing: dock.gap
            readonly property bool fourAcross: width >= 1000
            readonly property real buttonWidth: fourAcross ? (width - 3 * spacing) / 4
                                                           : (width - spacing) / 2

            OmaHintButton { width: Math.max(implicitWidth, actionFlow.buttonWidth); text: "Hit"; hint: "H"; primary: true; enabled: game.canHit; onClicked: game.hit() }
            OmaHintButton { width: Math.max(implicitWidth, actionFlow.buttonWidth); text: "Stand"; hint: "S"; enabled: game.canStand; onClicked: game.stand() }
            OmaHintButton { width: Math.max(implicitWidth, actionFlow.buttonWidth); text: "Double"; hint: "D"; enabled: game.canDouble; onClicked: game.doubleDown() }
            OmaHintButton { width: Math.max(implicitWidth, actionFlow.buttonWidth); text: "Split"; hint: "P"; enabled: game.canSplit; onClicked: game.split() }
        }

        OmaHintButton {
            visible: game.roundOver
            width: Math.max(implicitWidth, Math.min(parent.width, 250 * theme.textScale))
            anchors.right: parent.right
            text: "Next round"
            hint: "Enter / Space"
            primary: true
            onClicked: game.nextRound()
        }

        OmaHintButton {
            visible: game.phase !== "betting" && !game.waitingForHuman && !game.roundOver
            width: Math.max(implicitWidth, parent.width)
            text: game.phase === "dealing" ? "Dealing…" : game.phase === "dealer" ? "Dealer playing…" : "Table mate playing…"
            hint: "Space"
            onClicked: game.skipPacing()
        }
    }
}
