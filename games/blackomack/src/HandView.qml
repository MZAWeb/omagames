import QtQuick
import OmaGames

// One hand: overlapping cards, total and a result badge. `hand` is a map from
// game.seats[].hands[] (cards, total, soft, bet, active, result, net).
Item {
    id: view
    property var hand
    property real cardScale: 1.0
    property bool highlighted: hand ? hand.active : false

    readonly property int cardCount: hand ? hand.cards.length : 0
    readonly property real cardW: 52 * theme.textScale * cardScale
    readonly property real overlap: cardW * 0.55
    readonly property real badgeH: 20 * theme.textScale

    implicitWidth: Math.max(cardW, cardW + (cardCount - 1) * overlap)
    implicitHeight: cardW * 1.45 + badgeH + 4 * theme.textScale

    function badgeColor(result) {
        switch (result) {
        case "WIN": case "BLACKJACK": return theme.green;
        case "LOSE": case "BUST": return theme.red;
        case "PUSH": return theme.yellow;
        }
        return "transparent";
    }

    Rectangle {
        // Soft glow behind the hand that is currently deciding.
        visible: view.highlighted
        anchors.fill: cards
        anchors.margins: -4 * theme.textScale
        radius: 8
        color: theme.alpha(theme.accent, 0.18)
        border.width: 1
        border.color: theme.accent
    }

    Item {
        id: cards
        width: view.implicitWidth
        height: view.cardW * 1.45
        Repeater {
            model: view.hand ? view.hand.cards : []
            delegate: PlayingCard {
                required property var modelData
                required property int index
                x: index * view.overlap
                rank: modelData.rank
                suit: modelData.suit
                red: modelData.red
                scale_: view.cardScale
            }
        }
    }

    Row {
        anchors.top: cards.bottom
        anchors.topMargin: 4 * theme.textScale
        anchors.horizontalCenter: cards.horizontalCenter
        spacing: 6 * theme.textScale

        Text {
            visible: view.cardCount > 0
            text: view.hand ? (view.hand.soft && view.hand.total < 21 ? "soft " : "") + view.hand.total : ""
            color: theme.foreground
            font.pixelSize: 13 * theme.textScale
            font.bold: true
            anchors.verticalCenter: parent.verticalCenter
        }
        Rectangle {
            visible: view.hand && view.hand.result !== ""
            width: badgeText.implicitWidth + 10 * theme.textScale
            height: view.badgeH
            radius: height / 2
            color: view.badgeColor(view.hand ? view.hand.result : "")
            anchors.verticalCenter: parent.verticalCenter
            Text {
                id: badgeText
                anchors.centerIn: parent
                text: view.hand ? view.hand.result : ""
                color: theme.background
                font.pixelSize: 11 * theme.textScale
                font.bold: true
            }
        }
    }
}
