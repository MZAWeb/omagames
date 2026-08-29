import QtQuick
import OmaGames

// A player's spot at the table: name line, bankroll/bet and their hand(s).
// The human's seat is drawn larger with an accent frame.
OmaPanel {
    id: seat
    property var seatData
    readonly property bool human: seatData ? seatData.human : false
    readonly property bool active: seatData ? seatData.active : false
    readonly property real cardScale: human ? 1.15 : 0.85

    padding: 10 * theme.textScale
    implicitWidth: Math.max(column.implicitWidth, 120 * theme.textScale) + 2 * padding
    implicitHeight: column.implicitHeight + 2 * padding
    border.width: human ? 2 : 1
    border.color: active ? theme.accent : (human ? theme.alpha(theme.accent, 0.6) : theme.alpha(theme.foreground, 0.12))
    color: human ? theme.mix(theme.background, theme.accent, 0.08) : theme.mix(theme.background, theme.foreground, 0.05)

    function totalBet() {
        var sum = 0;
        if (!seatData) return 0;
        for (var i = 0; i < seatData.hands.length; ++i)
            sum += seatData.hands[i].bet;
        return sum;
    }

    Column {
        id: column
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: 4 * theme.textScale

        Text {
            text: seat.seatData ? seat.seatData.name : ""
            color: seat.human ? theme.accent : theme.foreground
            font.pixelSize: (seat.human ? 16 : 14) * theme.textScale
            font.bold: true
            anchors.horizontalCenter: parent.horizontalCenter
        }
        Text {
            visible: !seat.human
            text: seat.seatData ? seat.seatData.personality : ""
            color: theme.muted
            font.pixelSize: 11 * theme.textScale
            anchors.horizontalCenter: parent.horizontalCenter
        }
        Text {
            text: seat.seatData ? "Ø " + Number(seat.seatData.bankroll).toLocaleString(Qt.locale(), "f", 0)
                                    + (seat.totalBet() > 0 ? "  ·  bet " + seat.totalBet() : "") : ""
            color: theme.foreground
            opacity: 0.8
            font.pixelSize: 12 * theme.textScale
            anchors.horizontalCenter: parent.horizontalCenter
        }
        Row {
            spacing: 12 * theme.textScale
            anchors.horizontalCenter: parent.horizontalCenter
            Repeater {
                model: seat.seatData ? seat.seatData.hands : []
                delegate: HandView {
                    required property var modelData
                    hand: modelData
                    cardScale: seat.cardScale
                }
            }
            // Keeps the seat's height stable while waiting for the deal.
            Item {
                visible: !seat.seatData || seat.seatData.hands.length === 0
                width: 52 * theme.textScale * seat.cardScale
                height: width * 1.45 + 24 * theme.textScale
                Text {
                    anchors.centerIn: parent
                    text: seat.seatData && seat.seatData.bankroll < game.minBet ? "broke" : "waiting"
                    color: theme.muted
                    font.pixelSize: 12 * theme.textScale
                }
            }
        }
    }
}
