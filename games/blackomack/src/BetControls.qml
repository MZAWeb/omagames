import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OmaGames

// Bottom bar: bankroll with the session stats, bet stepper with typed entry,
// and the round buttons.
// Which buttons show depends on the phase; disabled ones stay disabled.
RowLayout {
    id: bar
    spacing: 8 * theme.textScale
    readonly property bool betting: game.phase === "betting"
    // While the bet is being typed the single-letter shortcuts stay quiet.
    readonly property bool typing: betField.activeFocus

    function focusBet() {
        if (!bar.betting)
            return;
        betField.forceActiveFocus();
        betField.selectAll();
    }

    Column {
        spacing: 2 * theme.textScale
        Layout.alignment: Qt.AlignVCenter

        Text {
            text: "Ø " + Number(game.bankroll).toLocaleString(Qt.locale(), "f", 0)
            color: theme.accent
            font.pixelSize: 20 * theme.textScale
            font.bold: true
        }
        Row {
            spacing: 5 * theme.textScale
            Text {
                text: game.handsPlayed + " hands  ·"
                color: theme.muted
                font.pixelSize: 11 * theme.textScale
            }
            Text {
                text: (game.netResult >= 0 ? "+" : "−") + Math.abs(game.netResult) + " Ø"
                color: game.netResult >= 0 ? theme.green : theme.red
                opacity: 0.9
                font.pixelSize: 11 * theme.textScale
            }
        }
    }

    Item { Layout.fillWidth: true }

    OmaHintButton {
        text: "−10"; hint: "↓"; showHint: bar.betting
        enabled: bar.betting && game.bet > game.minBet
        onClicked: game.adjustBet(-10)
    }
    Item {
        Layout.preferredWidth: betField.width
        Layout.preferredHeight: betField.height
        // Greyed out like the buttons once the stake is locked.
        opacity: bar.betting ? 1.0 : 0.5
        TextField {
            id: betField
            width: 100 * theme.textScale
            enabled: bar.betting
            text: game.bet
            color: theme.foreground
            font.pixelSize: 16 * theme.textScale
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
            validator: IntValidator { bottom: 0; top: 1000000 }
            selectByMouse: true
            function commit() { game.setBet(parseInt(text) || 0); text = game.bet; }
            onEditingFinished: commit()
            Keys.onReturnPressed: { commit(); focus = false; game.dealRound(); }
            Keys.onEnterPressed: { commit(); focus = false; game.dealRound(); }
            Keys.onEscapePressed: { text = game.bet; focus = false; }
            Connections { target: game; function onBetChanged() { if (!betField.activeFocus) betField.text = game.bet; } }
        }
        OmaKeyHint {
            key: bar.betting ? "B" : ""
            active: bar.betting
            anchors.right: betField.right
            anchors.top: betField.top
            anchors.margins: 4 * theme.textScale
            visible: !betField.activeFocus
        }
    }
    OmaHintButton {
        text: "+10"; hint: "↑"; showHint: bar.betting
        enabled: bar.betting && game.bet < game.maxBet
        onClicked: game.adjustBet(10)
    }
    OmaHintButton {
        text: "Max"; hint: "M"; showHint: bar.betting
        enabled: bar.betting && game.bet < game.maxBet
        onClicked: game.betMax()
    }

    Item { Layout.fillWidth: true }

    OmaHintButton { visible: bar.betting; text: "Deal"; hint: "Enter"; primary: true; enabled: game.canDeal; onClicked: game.dealRound() }
    OmaHintButton { visible: game.roundOver; text: "Next round"; hint: "Enter"; primary: true; onClicked: game.nextRound() }
    OmaHintButton { visible: !bar.betting && !game.roundOver; text: "Hit"; hint: "H"; primary: true; enabled: game.canHit; onClicked: game.hit() }
    OmaHintButton { visible: !bar.betting && !game.roundOver; text: "Stand"; hint: "S"; enabled: game.canStand; onClicked: game.stand() }
    OmaHintButton { visible: !bar.betting && !game.roundOver; text: "Double"; hint: "D"; enabled: game.canDouble; onClicked: game.doubleDown() }
    OmaHintButton { visible: !bar.betting && !game.roundOver; text: "Split"; hint: "P"; enabled: game.canSplit; onClicked: game.split() }
}
