import QtQuick
import OmaGames

// One engine-owned hand. Cards wrap after four while preserving every corner;
// the result and net always remain outside the card area.
Item {
    id: view
    property var hand
    property real cardScale: 1.0
    property int handNumber: 0
    property int handCount: 1

    readonly property int cardCount: hand ? hand.cards.length : 0
    readonly property int cardsPerRow: Math.min(4, Math.max(1, cardCount))
    readonly property int cardRows: Math.max(1, Math.ceil(cardCount / 4))
    readonly property real cardWidth: 52 * theme.textScale * cardScale
    readonly property real cardHeight: cardWidth * 1.45
    readonly property real overlap: cardWidth * (cardCount > 4 ? 0.48 : 0.56)
    readonly property real cardSpan: cardWidth + (cardsPerRow - 1) * overlap
    readonly property real cardsHeight: cardHeight + (cardRows - 1) * cardHeight * 0.55
    readonly property int net: hand && hand.resolved ? hand.net : 0

    implicitWidth: Math.max(cardSpan, info.implicitWidth)
    implicitHeight: splitLabel.implicitHeight + cardsHeight + info.implicitHeight + 8 * theme.textScale

    function resultColor(result) {
        switch (result) {
        case "WIN": case "BJ": return theme.green;
        case "LOSE": case "BUST": return theme.red;
        case "PUSH": return theme.yellow;
        }
        return theme.foreground;
    }

    Text {
        id: splitLabel
        visible: view.handCount > 1
        anchors.horizontalCenter: parent.horizontalCenter
        text: "Hand " + (view.handNumber + 1) + " of " + view.handCount
        color: view.hand && view.hand.active ? theme.accent : theme.foreground
        opacity: view.hand && view.hand.active ? 1.0 : 0.72
        font.pixelSize: 10 * theme.textScale
        font.bold: view.hand && view.hand.active
    }

    Rectangle {
        visible: view.hand && view.hand.active
        x: cards.x - 4 * theme.textScale
        y: cards.y - 4 * theme.textScale
        width: cards.width + 8 * theme.textScale
        height: cards.height + 8 * theme.textScale
        radius: 8 * theme.textScale
        color: theme.alpha(theme.accent, 0.14)
        border.width: 2
        border.color: theme.accent
    }

    Item {
        id: cards
        anchors.top: splitLabel.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        width: view.cardSpan
        height: view.cardsHeight

        Repeater {
            model: view.hand ? view.hand.cards : []
            delegate: PlayingCard {
                required property var modelData
                required property int index
                x: (index % 4) * view.overlap
                y: Math.floor(index / 4) * view.cardHeight * 0.55
                rank: modelData.rank
                suit: modelData.suit
                red: modelData.red
                scale_: view.cardScale
            }
        }
    }

    Row {
        id: info
        anchors.top: cards.bottom
        anchors.topMargin: 5 * theme.textScale
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: 6 * theme.textScale

        Text {
            visible: view.cardCount > 0
            text: view.hand ? (view.hand.soft && view.hand.total < 21 ? "soft " : "") + view.hand.total : ""
            color: theme.foreground
            font.pixelSize: 12 * theme.textScale
            font.bold: true
        }
        Rectangle {
            visible: view.hand && view.hand.result !== ""
            width: resultText.implicitWidth + 10 * theme.textScale
            height: resultText.implicitHeight + 3 * theme.textScale
            radius: height / 2
            color: view.resultColor(view.hand ? view.hand.result : "")
            Text {
                id: resultText
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
