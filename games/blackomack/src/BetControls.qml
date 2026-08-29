import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OmaGames

// Bottom bar: bankroll, bet stepper with typed entry, and the round buttons.
// Which buttons show depends on the phase; disabled ones stay disabled.
RowLayout {
    id: bar
    spacing: 8 * theme.textScale
    readonly property bool betting: game.phase === "betting"

    Text {
        text: "Ø " + Number(game.bankroll).toLocaleString(Qt.locale(), "f", 0)
        color: theme.accent
        font.pixelSize: 20 * theme.textScale
        font.bold: true
    }

    Item { Layout.fillWidth: true }

    OmaButton { text: "−10"; enabled: bar.betting && game.bet > game.minBet; onClicked: game.adjustBet(-10) }
    TextField {
        id: betField
        Layout.preferredWidth: 90 * theme.textScale
        enabled: bar.betting
        text: game.bet
        color: theme.foreground
        font.pixelSize: 16 * theme.textScale
        font.bold: true
        horizontalAlignment: Text.AlignHCenter
        validator: IntValidator { bottom: 0; top: 1000000 }
        selectByMouse: true
        onEditingFinished: { game.setBet(parseInt(text) || 0); text = game.bet; focus = false; }
        Connections { target: game; function onBetChanged() { if (!betField.activeFocus) betField.text = game.bet; } }
        Keys.onEscapePressed: { text = game.bet; focus = false; }
    }
    OmaButton { text: "+10"; enabled: bar.betting && game.bet < game.maxBet; onClicked: game.adjustBet(10) }
    OmaButton { text: "Max"; enabled: bar.betting && game.bet < game.maxBet; onClicked: game.betMax() }

    Item { Layout.fillWidth: true }

    OmaButton { visible: bar.betting; text: "Deal"; primary: true; enabled: game.canDeal; onClicked: game.dealRound() }
    OmaButton { visible: game.roundOver; text: "Next round"; primary: true; onClicked: game.nextRound() }
    OmaButton { visible: !bar.betting && !game.roundOver; text: "Hit"; primary: true; enabled: game.canHit; onClicked: game.hit() }
    OmaButton { visible: !bar.betting && !game.roundOver; text: "Stand"; enabled: game.canStand; onClicked: game.stand() }
    OmaButton { visible: !bar.betting && !game.roundOver; text: "Double"; enabled: game.canDouble; onClicked: game.doubleDown() }
    OmaButton { visible: !bar.betting && !game.roundOver; text: "Split"; enabled: game.canSplit; onClicked: game.split() }
}
