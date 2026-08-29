import QtQuick
import QtQuick.Layouts
import OmaGames

// Wide, shallow human tray for the high-scale stage. Metadata shares the
// header so the cards can stay large without colliding with the dealer.
OmaPanel {
    id: seat
    property var seatData
    readonly property int handCount: seatData ? seatData.hands.length : 0
    padding: 8 * theme.textScale
    implicitHeight: content.implicitHeight + 2 * padding
    clip: true
    border.width: 3
    border.color: seatData && seatData.active ? theme.accent : theme.alpha(theme.accent, 0.72)
    color: theme.mix(theme.darkBackground, theme.accent, 0.12)

    function totalBet() {
        if (game.phase === "betting")
            return game.bet;
        var sum = 0;
        if (!seatData)
            return sum;
        for (var i = 0; i < seatData.hands.length; ++i)
            sum += seatData.hands[i].bet;
        return sum;
    }

    // Live status only: an idle seat says nothing rather than "waiting".
    function statusText() {
        return seatData && seatData.active ? "Your turn" : "";
    }

    Column {
        id: content
        width: parent.width
        spacing: 5 * theme.textScale

        RowLayout {
            width: parent.width
            spacing: 6 * theme.textScale
            Rectangle {
                implicitWidth: 9 * theme.textScale
                implicitHeight: implicitWidth
                radius: width / 2
                color: seat.seatData && seat.seatData.active ? theme.accent : theme.alpha(theme.accent, 0.75)
                Layout.alignment: Qt.AlignVCenter
            }
            Text {
                text: seat.seatData ? seat.seatData.name : "You"
                color: theme.foreground
                font.pixelSize: 11 * theme.textScale
                font.bold: true
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
            Text {
                visible: seat.statusText() !== ""
                text: seat.statusText()
                color: seat.seatData && seat.seatData.active ? theme.accent : theme.foreground
                opacity: seat.seatData && seat.seatData.active ? 1.0 : 0.72
                font.pixelSize: 11 * theme.textScale
                font.bold: true
                elide: Text.ElideRight
                Layout.maximumWidth: 90 * theme.textScale
            }
            Text {
                text: seat.seatData ? "Ø " + Number(seat.seatData.bankroll).toLocaleString(Qt.locale(), "f", 0) : ""
                color: theme.foreground
                opacity: 0.82
                font.pixelSize: 11 * theme.textScale
                elide: Text.ElideRight
                Layout.maximumWidth: 110 * theme.textScale
            }
            Text {
                text: seat.seatData ? "· " + (seat.seatData.net > 0 ? "+" : seat.seatData.net < 0 ? "−" : "")
                                      + Math.abs(seat.seatData.net) : ""
                color: !seat.seatData || seat.seatData.net === 0 ? theme.foreground
                       : seat.seatData.net > 0 ? theme.green : theme.red
                opacity: 0.9
                font.pixelSize: 11 * theme.textScale
                font.bold: seat.seatData && seat.seatData.net !== 0
                elide: Text.ElideRight
                Layout.maximumWidth: 90 * theme.textScale
            }
        }

        Flow {
            width: parent.width
            spacing: 10 * theme.textScale
            Repeater {
                model: seat.seatData ? seat.seatData.hands : []
                delegate: HandView {
                    required property var modelData
                    required property int index
                    hand: modelData
                    cardScale: 0.78
                    handNumber: index
                    handCount: seat.handCount
                }
            }
            Text {
                visible: seat.handCount === 0
                width: 120 * theme.textScale
                height: 72 * theme.textScale
                text: game.isBroke ? "Broke" : ""
                color: theme.foreground
                opacity: 0.72
                font.pixelSize: 11 * theme.textScale
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }
            Rectangle {
                visible: seat.totalBet() > 0
                width: 42 * theme.textScale
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
