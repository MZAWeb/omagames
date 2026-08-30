import QtQuick

// The insurance side bet as a second, smaller coin beside a seat's bet chip.
// It rides on the table while the stake is down, turns green or red for a
// moment when the peek settles it, and then leaves — the bridge owns every
// number here, this only decides how long the verdict stays on screen.
Rectangle {
    id: chip
    property var seatData
    property real coinSize: 34 * theme.textScale

    readonly property int stake: seatData ? seatData.insuranceBet : 0
    readonly property bool settled: seatData ? seatData.insuranceSettled : false
    readonly property int net: seatData ? seatData.insuranceNet : 0
    readonly property color tint: !settled ? theme.cyan : net > 0 ? theme.green : theme.red
    // Set once the verdict has been read; a new round's stake clears it.
    property bool spent: false

    visible: stake > 0 && !spent
    width: coinSize
    height: coinSize
    radius: width / 2
    color: theme.mix(tint, theme.background, 0.16)
    border.width: 2
    border.color: tint

    // Both handlers are idempotent, so it does not matter which of the two
    // model fields QML reports as changed first when the peek is instant.
    onSettledChanged: if (settled) linger.restart(); else spent = false
    onStakeChanged: if (stake === 0) spent = false

    Timer {
        id: linger
        interval: 1600
        onTriggered: chip.spent = true
    }

    Text {
        anchors.centerIn: parent
        width: parent.width - 6 * theme.textScale
        text: chip.settled ? "Ins\n" + (chip.net > 0 ? "+Ø" : "−Ø") + Math.abs(chip.net)
                           : "Ins\nØ" + chip.stake
        color: theme.background
        font.pixelSize: 10 * theme.textScale
        font.bold: true
        horizontalAlignment: Text.AlignHCenter
        lineHeight: 0.85
        fontSizeMode: Text.HorizontalFit
        minimumPixelSize: 6 * theme.textScale
    }
}
