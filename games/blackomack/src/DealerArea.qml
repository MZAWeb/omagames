import QtQuick
import OmaGames

// Dealer anchor shared by the oval and the large high-scale stage.
Item {
    id: area
    property real cardScale: game.botCount <= 2 ? 1.28 : 0.9
    readonly property var dealer: game.dealerHand
    readonly property real cardWidth: 52 * theme.textScale * cardScale
    readonly property real overlap: cardWidth * 0.66
    readonly property int cardCount: dealer.cards ? dealer.cards.length : 0

    implicitWidth: Math.max(label.implicitWidth, cardWidth + Math.max(0, cardCount - 1) * overlap)
    implicitHeight: label.implicitHeight + cardWidth * 1.45 + 5 * theme.textScale

    Column {
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: 4 * theme.textScale

        Text {
            id: label
            anchors.horizontalCenter: parent.horizontalCenter
            text: "Dealer" + (area.cardCount > 0 ? " · " + (area.dealer.blackjack ? "blackjack"
                                                    : area.dealer.bust ? "bust " + area.dealer.total
                                                                       : "showing " + area.dealer.total) : "")
            color: area.dealer.bust ? theme.red : theme.foreground
            font.pixelSize: 11 * theme.textScale
            font.bold: true
        }
        Item {
            anchors.horizontalCenter: parent.horizontalCenter
            width: Math.max(area.cardWidth, area.cardWidth + (area.cardCount - 1) * area.overlap)
            height: area.cardWidth * 1.45
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
                    scale_: area.cardScale
                }
            }
            Text {
                visible: area.cardCount === 0
                anchors.centerIn: parent
                text: game.phase === "betting" ? "Shoe ready" : ""
                color: theme.foreground
                opacity: 0.72
                font.pixelSize: 10 * theme.textScale
            }
        }
    }
}
