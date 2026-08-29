import QtQuick
import QtQuick.Layouts
import OmaGames

// A spatial seat on the oval. The human tray has a fixed accent hierarchy and
// grows upward for split or long hands instead of hiding seat information.
OmaPanel {
    id: seat
    property var seatData
    property real targetWidth: human ? 430 * theme.textScale : 190 * theme.textScale
    property real targetHeight: human ? 180 * theme.textScale : 132 * theme.textScale
    property real cardScale: human ? 1.12 : 0.82

    readonly property bool human: seatData ? seatData.human : false
    readonly property bool active: seatData ? seatData.active : false
    readonly property int handCount: seatData ? seatData.hands.length : 0
    width: targetWidth
    implicitHeight: Math.max(targetHeight, content.implicitHeight + 2 * padding)
    clip: true
    padding: (human ? 12 : 8) * theme.textScale
    border.width: human ? 3 : active ? 2 : 1
    border.color: active ? theme.accent
                               : human ? theme.alpha(theme.accent, 0.7)
                                       : theme.alpha(theme.foreground, 0.18)
    color: human ? theme.mix(theme.darkBackground, theme.accent, 0.12)
                 : theme.mix(theme.darkBackground, theme.foreground, 0.07)

    function totalBet() {
        if (!seatData)
            return 0;
        if (human && game.phase === "betting")
            return game.bet;
        var sum = 0;
        for (var i = 0; i < seatData.hands.length; ++i)
            sum += seatData.hands[i].bet;
        return sum;
    }

    // Only a live hand says anything here: an idle seat is visibly at the
    // table already, so an "at table" / "waiting" caption is pure noise.
    function statusText() {
        if (!active)
            return "";
        return human ? "Your turn" : "Playing";
    }

    Column {
        id: content
        width: parent.width
        spacing: (seat.human ? 6 : 3) * theme.textScale

        RowLayout {
            width: parent.width
            spacing: 6 * theme.textScale
            Rectangle {
                implicitWidth: (seat.human ? 10 : 7) * theme.textScale
                implicitHeight: implicitWidth
                radius: width / 2
                color: seat.active ? theme.accent : seat.human ? theme.alpha(theme.accent, 0.75) : theme.muted
                Layout.alignment: Qt.AlignVCenter
            }
            Text {
                text: seat.seatData ? seat.seatData.name : ""
                color: theme.foreground
                font.pixelSize: (seat.human ? 14 : 11) * theme.textScale
                font.bold: true
                elide: Text.ElideRight
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
            }
            Text {
                visible: seat.statusText() !== ""
                text: seat.statusText()
                color: seat.active ? theme.accent : theme.foreground
                opacity: seat.active ? 1.0 : 0.72
                font.pixelSize: 11 * theme.textScale
                font.bold: seat.active
                elide: Text.ElideRight
                Layout.maximumWidth: 72 * theme.textScale
                Layout.alignment: Qt.AlignVCenter
            }
        }

        Text {
            visible: !seat.human
            width: parent.width
            text: seat.seatData ? seat.seatData.personality : ""
            color: theme.foreground
            opacity: 0.75
            font.pixelSize: 11 * theme.textScale
            elide: Text.ElideRight
        }

        RowLayout {
            width: parent.width
            spacing: 5 * theme.textScale
            Text {
                text: seat.seatData ? "Ø " + Number(seat.seatData.bankroll).toLocaleString(Qt.locale(), "f", 0) : ""
                color: theme.foreground
                opacity: 0.82
                font.pixelSize: (seat.human ? 12 : 11) * theme.textScale
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
            Text {
                text: seat.seatData ? "· " + (seat.seatData.net > 0 ? "+" : seat.seatData.net < 0 ? "−" : "")
                                      + Math.abs(seat.seatData.net) : ""
                color: !seat.seatData || seat.seatData.net === 0 ? theme.foreground
                       : seat.seatData.net > 0 ? theme.green : theme.red
                opacity: 0.9
                font.pixelSize: (seat.human ? 12 : 11) * theme.textScale
                font.bold: seat.seatData && seat.seatData.net !== 0
                elide: Text.ElideRight
                Layout.maximumWidth: (seat.human ? 90 : 70) * theme.textScale
            }
        }

        Flow {
            width: parent.width
            spacing: (seat.human ? 14 : 6) * theme.textScale
            Repeater {
                model: seat.seatData ? seat.seatData.hands : []
                delegate: HandView {
                    required property var modelData
                    required property int index
                    hand: modelData
                    cardScale: seat.cardScale
                    handNumber: index
                    handCount: seat.handCount
                }
            }
            // Empty but present between rounds: it keeps the seat's height
            // stable while the cards are away.
            Text {
                visible: seat.handCount === 0
                text: seat.seatData && seat.seatData.bankroll < game.minBet ? "Broke" : ""
                color: theme.foreground
                opacity: 0.72
                font.pixelSize: 11 * theme.textScale
                width: (seat.human ? 120 : 90) * theme.textScale
                height: (seat.human ? 70 : 50) * theme.textScale
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
                elide: Text.ElideRight
            }

            Rectangle {
                visible: seat.totalBet() > 0
                width: (seat.human ? 56 : 44) * theme.textScale
                height: width
                radius: width / 2
                color: theme.mix(theme.yellow, theme.background, 0.16)
                border.width: 2
                border.color: theme.yellow
                Text {
                    anchors.centerIn: parent
                    width: parent.width - 6 * theme.textScale
                    text: "Bet\nØ" + seat.totalBet()
                    color: theme.background
                    font.pixelSize: 11 * theme.textScale
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    lineHeight: 0.85
                    fontSizeMode: Text.HorizontalFit
                    minimumPixelSize: 7 * theme.textScale
                }
            }
        }
    }
}
