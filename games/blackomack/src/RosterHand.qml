import QtQuick
import OmaGames

// Horizontal hand summary keeps high-scale roster rows short without dropping
// cards, total, result or payout.
Item {
    id: view
    property var hand
    property int handNumber: 0
    property int handCount: 1
    readonly property int cardCount: hand ? hand.cards.length : 0
    readonly property real cardScale: 0.44
    readonly property real cardWidth: 52 * theme.textScale * cardScale
    readonly property real cardHeight: cardWidth * 1.45
    readonly property real overlap: cardWidth * 0.48
    readonly property int cardsPerRow: Math.min(4, Math.max(1, cardCount))
    readonly property int cardRows: Math.max(1, Math.ceil(cardCount / 4))
    readonly property real cardSpan: cardWidth + (cardsPerRow - 1) * overlap
    readonly property real cardsHeight: cardHeight + (cardRows - 1) * cardHeight * 0.52
    readonly property int net: hand && hand.resolved ? hand.net : 0

    implicitWidth: cards.width + info.implicitWidth + 7 * theme.textScale
    implicitHeight: Math.max(cards.height, info.implicitHeight)

    function resultColor(result) {
        switch (result) {
        case "WIN": case "BJ": return theme.green;
        case "LOSE": case "BUST": return theme.red;
        case "PUSH": return theme.yellow;
        }
        return theme.foreground;
    }

    Item {
        id: cards
        width: view.cardSpan
        height: view.cardsHeight
        Repeater {
            model: view.cardCount
            delegate: PlayingCard {
                required property int index
                readonly property var cardData: view.hand.cards[index]
                x: (index % 4) * view.overlap
                y: Math.floor(index / 4) * view.cardHeight * 0.52
                rank: cardData.rank
                suit: cardData.suit
                red: cardData.red
                animated: index === view.cardCount - 1
                scale_: view.cardScale
            }
        }
    }

    Column {
        id: info
        anchors.left: cards.right
        anchors.leftMargin: 7 * theme.textScale
        anchors.verticalCenter: cards.verticalCenter
        spacing: 2 * theme.textScale
        Text {
            visible: view.handCount > 1
            text: "Hand " + (view.handNumber + 1) + " of " + view.handCount
            color: view.hand && view.hand.active ? theme.accent : theme.foreground
            opacity: view.hand && view.hand.active ? 1.0 : 0.72
            font.pixelSize: 10 * theme.textScale
            font.bold: view.hand && view.hand.active
        }
        Text {
            text: view.hand ? (view.hand.soft && view.hand.total < 21 ? "soft " : "total ") + view.hand.total : ""
            color: theme.foreground
            font.pixelSize: 11 * theme.textScale
            font.bold: true
        }
        Rectangle {
            visible: view.hand && view.hand.result !== ""
            width: result.implicitWidth + 8 * theme.textScale
            height: result.implicitHeight + 2 * theme.textScale
            radius: height / 2
            color: view.resultColor(view.hand ? view.hand.result : "")
            Text {
                id: result
                anchors.centerIn: parent
                text: view.hand ? view.hand.result : ""
                color: theme.background
                font.pixelSize: 11 * theme.textScale
                font.bold: true
            }
        }
        Text {
            visible: view.hand && view.hand.resolved
            text: (view.net >= 0 ? "+Ø" : "−Ø") + Math.abs(view.net)
            color: view.net > 0 ? theme.green : view.net < 0 ? theme.red : theme.yellow
            font.pixelSize: 11 * theme.textScale
            font.bold: true
        }
    }
}
