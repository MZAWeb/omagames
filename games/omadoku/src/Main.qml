import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Window

ApplicationWindow {
    id: win

    // Enough for the board beside its rail; below this the rail folds under
    // the board instead, and the board would start losing more than it gains.
    minimumWidth: Math.round(700 * theme.textScale)
    minimumHeight: Math.round(560 * theme.textScale)
    visible: true
    title: qsTr("Omadoku")
    color: theme.background
    Material.theme: theme.darkMode ? Material.Dark : Material.Light
    Material.accent: theme.accent
    Material.foreground: theme.foreground
    Material.background: theme.background

    // Escape unwinds one step at a time. The bridge owns the order — a
    // multi-cell selection, then the digit highlight — and says so by
    // returning false once only the puzzle itself is left to leave.
    function backOut() {
        if (!game.backOut())
            leaveGame();
    }

    // Leaving mid-puzzle is confirmed, because it looks destructive even though
    // the game is saved.
    function leaveGame() {
        if (game.inProgress)
            confirmLoader.active = true;
        else
            game.backToStart();
    }

    Shortcut {
        sequences: ["Ctrl+Q"]
        context: Qt.ApplicationShortcut
        onActivated: Qt.quit()
    }
    Shortcut {
        sequences: [StandardKey.Undo]
        context: Qt.ApplicationShortcut
        onActivated: game.undo()
    }
    Shortcut {
        sequences: ["Escape"]
        context: Qt.ApplicationShortcut
        enabled: game.state !== "start" && !confirmLoader.active
        onActivated: win.backOut()
    }

    Loader {
        id: screen
        anchors.fill: parent
        sourceComponent: game.state === "start" ? startScreen : boardScreen
        onLoaded: (item as Item).forceActiveFocus()
    }

    Component { id: startScreen; StartScreen {} }
    Component {
        id: boardScreen
        BoardScreen { onLeaveRequested: win.leaveGame() }
    }

    Loader {
        anchors.fill: parent
        active: game.state === "won"
        sourceComponent: WonOverlay {
            difficultyName: game.difficultyLabel
            seconds: game.elapsedSeconds
            rank: game.newBestRank
            best: game.bests[game.difficulty]
            onNewGameRequested: game.newGame(game.difficulty)
            onBackRequested: game.backToStart()
        }
        onLoaded: (item as Item).forceActiveFocus()
    }

    Loader {
        id: confirmLoader
        anchors.fill: parent
        active: false
        sourceComponent: ConfirmDialog {
            message: qsTr("Leave this puzzle? Your progress is saved and you can resume it later.")
            acceptText: qsTr("Leave")
            onAccepted: {
                confirmLoader.active = false;
                game.backToStart();
            }
            onRejected: {
                confirmLoader.active = false;
                (screen.item as Item).forceActiveFocus();
            }
        }
        onLoaded: (item as Item).forceActiveFocus()
    }

    // Remember the last windowed geometry rather than whatever the window
    // happens to measure at teardown: a maximized window reports screen-sized
    // dimensions, and the close sequence hides the window before destruction.
    property rect normalGeometry: Qt.rect(x, y, width, height)
    property bool wasMaximized: false

    function trackNormalGeometry() {
        if (visibility === Window.Windowed)
            normalGeometry = Qt.rect(x, y, width, height);
    }

    onXChanged: trackNormalGeometry()
    onYChanged: trackNormalGeometry()
    onWidthChanged: trackNormalGeometry()
    onHeightChanged: trackNormalGeometry()

    onVisibilityChanged: {
        if (win.visibility === Window.Maximized || win.visibility === Window.FullScreen)
            wasMaximized = true;
        else if (win.visibility === Window.Windowed)
            wasMaximized = false;
    }

    Component.onCompleted: {
        var geometry = game.windowGeometry();
        if (geometry.valid) {
            x = geometry.x;
            y = geometry.y;
            width = geometry.width;
            height = geometry.height;
            if (geometry.maximized)
                showMaximized();
        } else {
            // First run: open at the design size, grown by the desktop text scale.
            width = Math.round(880 * theme.textScale);
            height = Math.round(640 * theme.textScale);
        }
    }

    Component.onDestruction: game.saveWindowGeometry(
        normalGeometry.x, normalGeometry.y,
        normalGeometry.width, normalGeometry.height, wasMaximized)
}
