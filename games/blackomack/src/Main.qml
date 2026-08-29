import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

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

    // Single-key shortcuts stay out of the way while a dialog is open or the
    // bet is being typed; Ctrl+Q always works.
    readonly property bool keysActive: !newGameDialog.opened && !brokeDialog.opened && !bets.typing

    Shortcut { sequence: "Ctrl+Q"; onActivated: window.close() }
    Shortcut { enabled: window.keysActive; sequence: "Ctrl+N"; onActivated: if (game.phase === "betting") newGameDialog.open() }
    Shortcut { enabled: window.keysActive; sequence: "H"; onActivated: game.hit() }
    Shortcut { enabled: window.keysActive; sequence: "S"; onActivated: game.stand() }
    Shortcut { enabled: window.keysActive; sequence: "D"; onActivated: game.doubleDown() }
    Shortcut { enabled: window.keysActive; sequence: "P"; onActivated: game.split() }
    Shortcut { enabled: window.keysActive; sequences: ["Return", "Enter", "Space"]; onActivated: game.roundOver ? game.nextRound() : game.dealRound() }
    Shortcut { enabled: window.keysActive; sequences: ["Up", "+"]; onActivated: game.adjustBet(10) }
    Shortcut { enabled: window.keysActive; sequences: ["Down", "-"]; onActivated: game.adjustBet(-10) }
    Shortcut { enabled: window.keysActive; sequence: "M"; onActivated: game.betMax() }
    Shortcut { enabled: window.keysActive; sequence: "B"; onActivated: bets.focusBet() }
    Shortcut { enabled: window.keysActive; sequence: "["; onActivated: game.setBotCount(game.botCount - 1) }
    Shortcut { enabled: window.keysActive; sequence: "]"; onActivated: game.setBotCount(game.botCount + 1) }

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

        BetControls { id: bets; Layout.fillWidth: true }
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
