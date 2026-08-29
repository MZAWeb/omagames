import QtQuick
import OmaGames

// The dealer's cards along the top, hole card face down until the dealer plays.
Item {
    id: area
    readonly property var dealer: game.dealerHand
    readonly property real cardW: 52 * theme.textScale
    readonly property real overlap: cardW * 0.6
    readonly property int count: dealer.cards ? dealer.cards.length : 0

    implicitHeight: cardW * 1.45 + 30 * theme.textScale

    Column {
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: 4 * theme.textScale

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "Dealer" + (area.count > 0 ? "  ·  " + (area.dealer.blackjack ? "blackjack"
                                                      : area.dealer.bust ? "bust" : area.dealer.total) : "")
            color: area.dealer.bust ? theme.red : theme.foreground
            font.pixelSize: 14 * theme.textScale
            font.bold: true
        }
        Item {
            anchors.horizontalCenter: parent.horizontalCenter
            width: Math.max(area.cardW, area.cardW + (area.count - 1) * area.overlap)
            height: area.cardW * 1.45
            Repeater {
                model: area.dealer.cards ? area.dealer.cards : []
                delegate: PlayingCard {
                    required property var modelData
                    required property int index
                    x: index * area.overlap
                    rank: modelData.rank
                    suit: modelData.suit
                    red: modelData.red
                    faceDown: modelData.hidden
                }
            }
            Text {
                visible: area.count === 0
                anchors.centerIn: parent
                text: game.phase === "betting" ? "Place your bet" : ""
                color: theme.muted
                font.pixelSize: 13 * theme.textScale
            }
        }
    }
}
