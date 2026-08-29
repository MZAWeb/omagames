import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import OmaGames

ApplicationWindow {
    id: window
    width: 1000; height: 720; visible: true
    minimumWidth: 640; minimumHeight: 520
    title: "Black Omack"
    color: theme.background
    Material.theme: theme.darkMode ? Material.Dark : Material.Light
    Material.accent: theme.accent

    // Remember the last windowed geometry, not a maximized/fullscreen one.
    property rect normalGeometry: Qt.rect(x, y, width, height)
    function trackNormalGeometry() {
        if (visibility === Window.Windowed)
            normalGeometry = Qt.rect(x, y, width, height);
    }
    onXChanged: trackNormalGeometry()
    onYChanged: trackNormalGeometry()
    onWidthChanged: trackNormalGeometry()
    onHeightChanged: trackNormalGeometry()
    Component.onCompleted: {
        var g = game.windowGeometry();
        if (g.width > 0 && g.height > 0) { x = g.x; y = g.y; width = g.width; height = g.height; }
        if (game.isBroke) brokeDialog.open();
    }
    onClosing: game.saveWindowGeometry(normalGeometry)

    Shortcut { sequence: "Ctrl+Q"; onActivated: window.close() }
    Shortcut { sequence: "Ctrl+N"; onActivated: if (game.phase === "betting") newGameDialog.open() }
    Shortcut { sequence: "H"; onActivated: game.hit() }
    Shortcut { sequence: "S"; onActivated: game.stand() }
    Shortcut { sequence: "D"; onActivated: game.doubleDown() }
    Shortcut { sequence: "P"; onActivated: game.split() }
    Shortcut { sequences: ["Return", "Enter", "Space"]; onActivated: game.roundOver ? game.nextRound() : game.dealRound() }
    Shortcut { sequences: ["Up", "+"]; onActivated: game.adjustBet(10) }
    Shortcut { sequences: ["Down", "-"]; onActivated: game.adjustBet(-10) }
    Shortcut { sequence: "M"; onActivated: game.betMax() }
    Shortcut { sequence: "Escape"; onActivated: window.contentItem.forceActiveFocus() }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16 * theme.textScale
        spacing: 12 * theme.textScale

        TableHeader { Layout.fillWidth: true; onNewGameRequested: newGameDialog.open() }

        DealerArea { Layout.fillWidth: true }

        // Bots in a row, the human below at center. Wide tables scroll rather
        // than shrink the cards.
        Flickable {
            id: seatArea
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: Math.max(width, seatColumn.width)
            contentHeight: Math.max(height, seatColumn.height)
            clip: true
            Column {
                id: seatColumn
                x: Math.max(0, (seatArea.width - width) / 2)
                y: Math.max(0, (seatArea.height - height) / 2)
                spacing: 16 * theme.textScale
                Row {
                    spacing: 10 * theme.textScale
                    anchors.horizontalCenter: parent.horizontalCenter
                    Repeater {
                        model: game.seats
                        delegate: TableSeat {
                            required property var modelData
                            visible: !modelData.human
                            seatData: modelData
                        }
                    }
                }
                TableSeat {
                    anchors.horizontalCenter: parent.horizontalCenter
                    seatData: game.seats[game.humanSeat]
                }
            }
        }

        MessageLog { Layout.fillWidth: true; Layout.preferredHeight: implicitHeight }

        BetControls { Layout.fillWidth: true }
    }

    ConfirmDialog {
        id: newGameDialog
        title: "Start a new game?"
        body: "Your bankroll goes back to Ø 1,000 and every table mate is replaced."
        acceptText: "New game"
        onAccepted: game.newGame()
    }
    ConfirmDialog {
        id: brokeDialog
        title: "You're out of Omabucks"
        body: "The house thanks you for your patronage. Start over with a fresh Ø 1,000?"
        acceptText: "New game"
        cancellable: false
        onAccepted: game.newGame()
    }
    Connections {
        target: game
        function onStateChanged() { if (game.isBroke && !brokeDialog.opened) brokeDialog.open(); }
    }
}
