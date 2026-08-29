import QtQuick
import QtQuick.Layouts
import OmaGames

// High-scale semantic fallback: one full-information bot row in deal order.
OmaPanel {
    id: row
    property var seatData
    readonly property int handCount: seatData ? seatData.hands.length : 0
    padding: 2 * theme.textScale
    implicitHeight: Math.max(42 * theme.textScale, content.implicitHeight + 2 * padding)
    clip: true
    border.width: seatData && seatData.active ? 2 : 1
    border.color: seatData && seatData.active ? theme.accent : theme.alpha(theme.foreground, 0.16)
    color: seatData && seatData.active ? theme.mix(theme.darkBackground, theme.accent, 0.12)
                                          : theme.mix(theme.darkBackground, theme.foreground, 0.07)

    function totalBet() {
        var sum = 0;
        if (!seatData)
            return sum;
        for (var i = 0; i < seatData.hands.length; ++i)
            sum += seatData.hands[i].bet;
        return sum;
    }

    Row {
        id: content
        width: parent.width
        spacing: 5 * theme.textScale

        Column {
            width: Math.min(132 * theme.textScale, content.width * 0.5)
            spacing: 2 * theme.textScale
            RowLayout {
                width: parent.width
                spacing: 4 * theme.textScale
                Rectangle {
                    implicitWidth: 6 * theme.textScale
                    implicitHeight: implicitWidth
                    radius: width / 2
                    color: row.seatData && row.seatData.active ? theme.accent : theme.muted
                    Layout.alignment: Qt.AlignVCenter
                }
                Text {
                    text: row.seatData ? row.seatData.name : ""
                    color: theme.foreground
                    font.pixelSize: 11 * theme.textScale
                    font.bold: true
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
                Text {
                    visible: row.seatData && row.seatData.active   // live status only
                    text: "Playing"
                    color: theme.accent
                    font.pixelSize: 11 * theme.textScale
                    font.bold: true
                    elide: Text.ElideRight
                    Layout.maximumWidth: 54 * theme.textScale
                }
            }
            Text {
                width: parent.width
                text: row.seatData ? row.seatData.personality : ""
                color: theme.foreground
                opacity: 0.75
                font.pixelSize: 11 * theme.textScale
                elide: Text.ElideRight
            }
            RowLayout {
                width: parent.width
                spacing: 4 * theme.textScale
                Text {
                    text: row.seatData ? "Ø " + Number(row.seatData.bankroll).toLocaleString(Qt.locale(), "f", 0) : ""
                    color: theme.foreground
                    opacity: 0.82
                    font.pixelSize: 11 * theme.textScale
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
                Text {
                    text: row.seatData ? "· " + (row.seatData.net > 0 ? "+" : row.seatData.net < 0 ? "−" : "")
                                          + Math.abs(row.seatData.net) : ""
                    color: !row.seatData || row.seatData.net === 0 ? theme.foreground
                           : row.seatData.net > 0 ? theme.green : theme.red
                    opacity: 0.9
                    font.pixelSize: 11 * theme.textScale
                    font.bold: row.seatData && row.seatData.net !== 0
                    elide: Text.ElideRight
                    Layout.maximumWidth: 62 * theme.textScale
                }
            }
            Text {
                visible: row.totalBet() > 0
                width: parent.width
                text: "Bet Ø" + row.totalBet()
                color: theme.foreground
                opacity: 0.75
                font.pixelSize: 11 * theme.textScale
                elide: Text.ElideRight
            }
        }

        Column {
            width: Math.max(0, content.width - x)
            spacing: 2 * theme.textScale
            Repeater {
                model: row.seatData ? row.seatData.hands : []
                delegate: RosterHand {
                    required property var modelData
                    required property int index
                    hand: modelData
                    handNumber: index
                    handCount: row.handCount
                    width: Math.min(implicitWidth, parent.width)
                }
            }
            Text {
                visible: row.handCount === 0
                width: 70 * theme.textScale
                height: 40 * theme.textScale
                text: row.seatData && row.seatData.bankroll < game.minBet ? "Broke" : ""
                color: theme.foreground
                opacity: 0.72
                font.pixelSize: 11 * theme.textScale
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }
    }
}
