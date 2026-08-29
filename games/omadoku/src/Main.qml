import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Window
import Omadoku

ApplicationWindow {
    id: win

    minimumWidth: Math.round(360 * theme.textScale)
    minimumHeight: Math.round(560 * theme.textScale)
    visible: true
    title: qsTr("Omadoku")
    color: theme.background
    Material.theme: theme.darkMode ? Material.Dark : Material.Light
    Material.accent: theme.accent
    Material.foreground: theme.foreground
    Material.background: theme.background

    readonly property var difficultyNames: [qsTr("Easy"), qsTr("Medium"), qsTr("Hard")]

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
        enabled: game.state !== SudokuGame.Start && !confirmLoader.active
        onActivated: win.leaveGame()
    }

    Loader {
        id: screen
        anchors.fill: parent
        sourceComponent: game.state === SudokuGame.Start ? startScreen : boardScreen
        onLoaded: item.forceActiveFocus()
    }

    Component { id: startScreen; StartScreen {} }
    Component {
        id: boardScreen
        BoardScreen {
            difficultyNames: win.difficultyNames
            onLeaveRequested: win.leaveGame()
        }
    }

    Loader {
        anchors.fill: parent
        active: game.state === SudokuGame.Won
        sourceComponent: WonOverlay {
            difficultyName: win.difficultyNames[game.difficulty]
            seconds: game.elapsedSeconds
            onNewGameRequested: game.newGame(game.difficulty)
            onBackRequested: game.backToStart()
        }
        onLoaded: item.forceActiveFocus()
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
                screen.item.forceActiveFocus();
            }
        }
        onLoaded: item.forceActiveFocus()
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
        if (visibility === Window.Maximized || visibility === Window.FullScreen)
            wasMaximized = true;
        else if (visibility === Window.Windowed)
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
            width = Math.round(420 * theme.textScale);
            height = Math.round(680 * theme.textScale);
        }
    }

    Component.onDestruction: game.saveWindowGeometry(
        normalGeometry.x, normalGeometry.y,
        normalGeometry.width, normalGeometry.height, wasMaximized)
}
