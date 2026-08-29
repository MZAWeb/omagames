import QtQuick
import QtQuick.Controls
import OmaGames

// Pre-deal controls only. The dock swaps this whole tray out after the deal.
Item {
    id: bar
    readonly property bool typing: betField.activeFocus
    readonly property real gap: 7 * theme.textScale
    readonly property real minimumFieldWidth: 112 * theme.textScale
    readonly property real fixedButtonsWidth: decrease.implicitWidth + increase.implicitWidth
                                                   + maximum.implicitWidth + deal.implicitWidth
    implicitHeight: controls.childrenRect.height

    function focusBet() {
        betField.forceActiveFocus();
        betField.selectAll();
    }

    Flow {
        id: controls
        width: parent.width
        spacing: bar.gap

        OmaHintButton {
            id: decrease
            width: implicitWidth
            text: "−10"
            hint: "↓"
            enabled: game.bet > game.minBet
            onClicked: game.adjustBet(-10)
        }
        Item {
            width: Math.max(bar.minimumFieldWidth,
                            controls.width - bar.fixedButtonsWidth - 4 * bar.gap)
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
