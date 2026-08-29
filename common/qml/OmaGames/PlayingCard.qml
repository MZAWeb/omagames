import QtQuick

// A playing card face (rank + suit glyph) or back, sized from the text scale so
// it stays legible with the rest of the UI. It travels into its slot when
// created, so a freshly dealt card is easy to follow: by default a short drop,
// or from wherever `entryFrom` says the dealer pitched it.
Rectangle {
    id: card
    property string rank: "A"
    property string suit: "♠"
    property bool red: false
    property bool faceDown: false
    property real scale_: 1.0
    property bool animated: true
    // Where the card starts, as an offset from its own slot. The entry moves
    // the card with a transform rather than its x/y, so the slot's bindings
    // survive the trip.
    property point entryFrom: Qt.point(0, -height * 0.4)
    property int entryDuration: 180

    width: 52 * theme.textScale * scale_
    height: width * 1.45
    radius: width * 0.12
    color: faceDown ? theme.accent : (theme.darkMode ? theme.lighterBackground : theme.brightForeground)
    border.width: 1
    border.color: faceDown ? theme.alpha(theme.background, 0.4) : theme.alpha(theme.foreground, 0.3)
    antialiasing: true

    readonly property color inkColor: red ? theme.red : (theme.darkMode ? theme.brightForeground : theme.darkerBackground)

    // Card back: a simple inset frame in the selection tint.
    Rectangle {
        visible: card.faceDown
        anchors.fill: parent
        anchors.margins: card.width * 0.1
        radius: card.radius * 0.6
        color: "transparent"
        border.width: 2
        border.color: theme.alpha(theme.selection, 0.9)
        Text {
            anchors.centerIn: parent
            text: "Ø"
            color: theme.alpha(theme.selection, 0.9)
            font.pixelSize: card.width * 0.4
            font.bold: true
        }
    }

    Text {
        visible: !card.faceDown
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.margins: card.width * 0.08
        text: card.rank
        color: card.inkColor
        font.pixelSize: card.width * 0.32
        font.bold: true
    }
    Text {
        visible: !card.faceDown
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: card.width * 0.08
        text: card.suit
        color: card.inkColor
        font.pixelSize: card.width * 0.32
    }
    Text {
        visible: !card.faceDown
        anchors.centerIn: parent
        text: card.suit
        color: card.inkColor
        opacity: 0.55
        font.pixelSize: card.width * 0.5
    }

    transform: Translate { id: entry }

    ParallelAnimation {
        id: entryAnimation
        NumberAnimation {
            target: entry
            property: "x"
            from: card.entryFrom.x
            to: 0
            duration: card.entryDuration
            easing.type: Easing.OutCubic
        }
        NumberAnimation {
            target: entry
            property: "y"
            from: card.entryFrom.y
            to: 0
            duration: card.entryDuration
            easing.type: Easing.OutCubic
        }
        NumberAnimation {
            target: card
            property: "opacity"
            from: 0
            to: 1
            duration: card.entryDuration
        }
    }

    Component.onCompleted: if (animated) entryAnimation.start()
}
