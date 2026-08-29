import QtQuick
import QtQuick.Controls
import OmaGames

// Pre-deal controls only. The dock swaps this whole tray out after the deal.
Item {
    id: bar
    readonly property bool typing: betField.activeFocus
    readonly property real gap: 7 * theme.textScale
    // The typed bet is a slot, not the row's slack: it holds the widest stake
    // anyone can reach plus the badge that sits inside it, and nothing more.
    readonly property real fieldWidth: sample.width + hint.implicitWidth + 26 * theme.textScale
    // Every control at its natural width, on one line, with five gaps. Buttons
    // are never squeezed below this, so no label is ever elided.
    readonly property real rowWidth: Math.ceil(presets.implicitWidth + decrease.implicitWidth
                                               + bar.fieldWidth + increase.implicitWidth
                                               + maximum.implicitWidth + deal.implicitWidth
                                               + 5 * bar.gap)
    // Wide enough for the whole row: centre it in the dock. Otherwise hand the
    // Flow the dock's width and let it break onto a second line.
    readonly property bool oneLine: width >= rowWidth
    implicitHeight: controls.implicitHeight

    function focusBet() {
        betField.forceActiveFocus();
        betField.selectAll();
    }

    TextMetrics {
        id: sample
        font: betField.font
        text: "Ø 10,000"
    }

    Flow {
        id: controls
        width: bar.oneLine ? bar.rowWidth : bar.width
        x: bar.oneLine ? (bar.width - width) / 2 : 0
        spacing: bar.gap

        // The stakes behind 1, 2 and 3, sized to the bankroll by the bridge so
        // the keys, the buttons and the README cannot drift apart. A bankroll
        // too thin to keep three apart simply offers fewer buttons.
        Row {
            id: presets
            spacing: bar.gap
            Repeater {
                model: game.betPresets
                delegate: OmaHintButton {
                    required property var modelData
                    required property int index
                    width: implicitWidth
                    text: "Ø " + Number(modelData).toLocaleString(Qt.locale(), "f", 0)
                    hint: String(index + 1)
                    enabled: game.bet !== modelData && game.maxBet >= modelData
                    onClicked: game.setBetPreset(index)
                }
            }
        }
        OmaHintButton {
            id: decrease
            width: implicitWidth
            text: "−10"
            hint: "↓"
            enabled: game.bet > game.minBet
            onClicked: game.adjustBet(-10)
        }
        Item {
            width: bar.fieldWidth
            height: 40 * theme.textScale
            TextField {
                id: betField
                anchors.fill: parent
                text: game.bet
                color: theme.foreground
                font.pixelSize: 14 * theme.textScale
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                validator: IntValidator { bottom: 0; top: 1000000 }
                selectByMouse: true
                function commit() { game.setBet(parseInt(text) || 0); text = game.bet; }
                onEditingFinished: commit()
                Keys.onReturnPressed: { commit(); focus = false; game.dealRound(); }
                Keys.onEnterPressed: { commit(); focus = false; game.dealRound(); }
                Keys.onEscapePressed: { text = game.bet; focus = false; }
                Connections {
                    target: game
                    function onBetChanged() { if (!betField.activeFocus) betField.text = game.bet; }
                }
            }
            OmaKeyHint {
                id: hint
                key: "B"
                active: true
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 4 * theme.textScale
                visible: !betField.activeFocus
            }
        }
        OmaHintButton {
            id: increase
            width: implicitWidth
            text: "+10"
            hint: "↑"
            enabled: game.bet < game.maxBet
            onClicked: game.adjustBet(10)
        }
        OmaHintButton {
            id: maximum
            width: implicitWidth
            text: "Max"
            hint: "M"
            enabled: game.bet < game.maxBet
            onClicked: game.betMax()
        }
        OmaHintButton {
            id: deal
            width: implicitWidth
            text: "Deal"
            hint: "Enter"
            primary: true
            enabled: game.canDeal
            onClicked: game.dealRound()
        }
    }
}
