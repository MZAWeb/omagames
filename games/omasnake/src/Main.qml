import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Window

ApplicationWindow {
    id: win

    minimumWidth: Math.round(640 * theme.textScale)
    minimumHeight: Math.round(520 * theme.textScale)
    visible: true
    title: qsTr("Omasnake")
    color: theme.background
    Material.theme: theme.darkMode ? Material.Dark : Material.Light
    Material.accent: theme.accent
    Material.foreground: theme.foreground
    Material.background: theme.background

    readonly property bool inGame: game.phase !== "start"
    // Whether the player had paused before asking to leave, so cancelling
    // returns to the pause overlay rather than straight into play.
    property bool pausedBeforeLeaving: false

    // Leaving mid-game throws the run away, so it is confirmed; the snake
    // holds still while the question is up. The dialog goes up before the
    // pause, so the pause overlay never flickers in between and hands the
    // focus back to the field the moment it disappears again.
    function leaveGame() {
        if (game.phase === "playing") {
            pausedBeforeLeaving = game.paused;
            confirmLoader.active = true;
            game.pause();
        } else {
            game.backToStart();
        }
    }

    function refocus() {
        if (screen.item)
            (screen.item as Item).forceActiveFocus();
    }

    Shortcut {
        sequences: ["Ctrl+Q"]
        context: Qt.ApplicationShortcut
        onActivated: Qt.quit()
    }

    Loader {
        id: screen
        anchors.fill: parent
        sourceComponent: win.inGame ? playScreen : startScreen
        onLoaded: win.refocus()
    }

    Component { id: startScreen; StartScreen {} }
    Component {
        id: playScreen
        PlayScreen { onLeaveRequested: win.leaveGame() }
    }

    Loader {
        anchors.fill: parent
        active: game.paused && !confirmLoader.active
        sourceComponent: PauseOverlay {
            onLeaveRequested: win.leaveGame()
        }
        onLoaded: (item as Item).forceActiveFocus()
        onActiveChanged: if (!active) win.refocus()
    }

    Loader {
        anchors.fill: parent
        active: game.phase === "gameover"
        sourceComponent: GameOverOverlay {}
        onLoaded: (item as Item).forceActiveFocus()
        onActiveChanged: if (!active) win.refocus()
    }

    Loader {
        id: confirmLoader
        anchors.fill: parent
        active: false
        sourceComponent: ConfirmDialog {
            message: qsTr("Leave this game? The score is lost unless it makes the high scores.")
            acceptText: qsTr("Leave")
            onAccepted: {
                confirmLoader.active = false;
                game.backToStart();
            }
            onRejected: {
                confirmLoader.active = false;
                if (!win.pausedBeforeLeaving)
                    game.resume();
                win.refocus();
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
            width = Math.round(900 * theme.textScale);
            height = Math.round(700 * theme.textScale);
        }
    }

    Component.onDestruction: game.saveWindowGeometry(
        normalGeometry.x, normalGeometry.y,
        normalGeometry.width, normalGeometry.height, wasMaximized)
}
