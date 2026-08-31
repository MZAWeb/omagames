import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Window
import OmaGames

ApplicationWindow {
    id: win

    minimumWidth: Math.round(480 * theme.textScale)
    minimumHeight: Math.round(560 * theme.textScale)
    visible: true
    title: qsTr("Oma2048")
    color: theme.background
    Material.theme: theme.darkMode ? Material.Dark : Material.Light
    Material.accent: theme.accent
    Material.foreground: theme.foreground
    Material.background: theme.background

    // While any of these is up, the board ignores the movement keys.
    readonly property bool overlayOpen: game.won || game.over
        || scoresLoader.active || confirmLoader.active

    // Hands the keyboard to whatever is on top; called whenever an overlay
    // comes or goes, so closing the scores over the game-over overlay lands
    // the focus back on that overlay, not on the board.
    function refocus() {
        if (confirmLoader.item)
            (confirmLoader.item as Item).forceActiveFocus();
        else if (scoresLoader.item)
            (scoresLoader.item as Item).forceActiveFocus();
        else if (gameOverLoader.item)
            (gameOverLoader.item as Item).forceActiveFocus();
        else if (winLoader.item)
            (winLoader.item as Item).forceActiveFocus();
        else
            playScreen.forceActiveFocus();
    }

    // Starting over mid-run abandons a score nothing will keep, so it is
    // confirmed — but only once there is anything to lose: a merge scored or
    // a move to undo. A fresh board restarts silently.
    function requestNewGame() {
        if (!game.over && (game.score > 0 || game.canUndo))
            confirmLoader.active = true;
        else
            game.newGame();
    }

    Shortcut {
        sequences: ["Ctrl+Q"]
        context: Qt.ApplicationShortcut
        onActivated: Qt.quit()
    }

    PlayScreen {
        id: playScreen
        anchors.fill: parent
        focus: true
        inputEnabled: !win.overlayOpen
        onNewGameRequested: win.requestNewGame()
        onScoresRequested: scoresLoader.active = true
    }

    Loader {
        id: winLoader
        anchors.fill: parent
        active: game.won
        sourceComponent: WinOverlay {}
        onLoaded: win.refocus()
        onActiveChanged: win.refocus()
    }

    Loader {
        id: gameOverLoader
        anchors.fill: parent
        active: game.over
        sourceComponent: GameOverOverlay {
            onScoresRequested: scoresLoader.active = true
        }
        onLoaded: win.refocus()
        onActiveChanged: win.refocus()
    }

    Loader {
        id: scoresLoader
        anchors.fill: parent
        active: false
        sourceComponent: ScoresOverlay {
            onCloseRequested: scoresLoader.active = false
        }
        onLoaded: win.refocus()
        onActiveChanged: win.refocus()
    }

    Loader {
        id: confirmLoader
        anchors.fill: parent
        active: false
        sourceComponent: OmaConfirmDialog {
            message: qsTr("Start a new game? This run is abandoned and its score is not kept.")
            acceptText: qsTr("New game")
            onAccepted: {
                confirmLoader.active = false;
                game.newGame();
            }
            onRejected: confirmLoader.active = false
        }
        onLoaded: win.refocus()
        onActiveChanged: win.refocus()
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
            width = Math.round(560 * theme.textScale);
            height = Math.round(680 * theme.textScale);
        }
    }

    Component.onDestruction: game.saveWindowGeometry(
        normalGeometry.x, normalGeometry.y,
        normalGeometry.width, normalGeometry.height, wasMaximized)
}
