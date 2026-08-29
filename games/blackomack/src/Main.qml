import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

ApplicationWindow {
    id: window
    width: 1280
    height: 800
    visible: true
    minimumWidth: 640
    minimumHeight: 520
    title: "Black Omack"
    color: theme.background
    Material.theme: theme.darkMode ? Material.Dark : Material.Light
    Material.accent: theme.accent

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
        var geometry = game.windowGeometry();
        if (geometry.width > 0 && geometry.height > 0) {
            x = geometry.x;
            y = geometry.y;
            width = geometry.width;
            height = geometry.height;
        }
        if (game.isBroke)
            brokeDialog.open();
    }
    onClosing: game.saveWindowGeometry(normalGeometry)

    readonly property bool keysActive: !newGameDialog.opened && !brokeDialog.opened && !dock.typing
    readonly property bool betKeysActive: keysActive && game.phase === "betting"

    Shortcut { sequence: "Ctrl+Q"; onActivated: window.close() }
    Shortcut { enabled: window.keysActive; sequence: "Ctrl+N"; onActivated: if (game.phase === "betting") newGameDialog.open() }
    Shortcut { enabled: window.keysActive; sequence: "H"; onActivated: game.hit() }
    Shortcut { enabled: window.keysActive; sequence: "S"; onActivated: game.stand() }
    Shortcut { enabled: window.keysActive; sequence: "D"; onActivated: game.doubleDown() }
    Shortcut { enabled: window.keysActive; sequence: "P"; onActivated: game.split() }
    Shortcut { enabled: window.keysActive; sequence: "I"; onActivated: game.insurance() }
    Shortcut { enabled: window.keysActive; sequence: "N"; onActivated: game.declineInsurance() }
    Shortcut { enabled: window.keysActive; sequence: "C"; onActivated: game.toggleCoach() }
    Shortcut {
        enabled: window.keysActive
        sequences: ["Return", "Enter"]
        onActivated: game.roundOver ? game.nextRound() : game.dealRound()
    }
    Shortcut {
        enabled: window.keysActive
        sequence: "Space"
        onActivated: {
            if (game.roundOver)
                game.nextRound();
            else if (game.phase === "betting")
                game.dealRound();
            else if (!game.waitingForHuman)
                game.skipPacing();
        }
    }
    Shortcut { enabled: window.betKeysActive; sequences: ["Up", "+"]; onActivated: game.adjustBet(10) }
    Shortcut { enabled: window.betKeysActive; sequences: ["Down", "-"]; onActivated: game.adjustBet(-10) }
    Shortcut { enabled: window.betKeysActive; sequence: "M"; onActivated: game.betMax() }
    Shortcut { enabled: window.betKeysActive; sequence: "1"; onActivated: game.setBetPreset(0) }
    Shortcut { enabled: window.betKeysActive; sequence: "2"; onActivated: game.setBetPreset(1) }
    Shortcut { enabled: window.betKeysActive; sequence: "3"; onActivated: game.setBetPreset(2) }
    Shortcut { enabled: window.betKeysActive; sequence: "B"; onActivated: dock.focusBet() }
    Shortcut { enabled: window.betKeysActive && game.botCount > 0; sequence: "["; onActivated: game.setBotCount(game.botCount - 1) }
    Shortcut { enabled: window.betKeysActive && game.botCount < game.maxBots; sequence: "]"; onActivated: game.setBotCount(game.botCount + 1) }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12 * theme.textScale
        spacing: 8 * theme.textScale

        TableHeader {
            Layout.fillWidth: true
            onNewGameRequested: newGameDialog.open()
        }
        HouseTable {
            id: table
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
        ActionDock {
            id: dock
            Layout.fillWidth: true
        }
    }

    // Only the window knows how much room the table has; the bridge turns that
    // into the mate cap the header and the `]` key obey.
    Binding {
        target: game
        property: "compactLayout"
        value: table.compact
        restoreMode: Binding.RestoreNone
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
        function onStateChanged() {
            if (game.isBroke && !brokeDialog.opened)
                brokeDialog.open();
        }
    }
}
