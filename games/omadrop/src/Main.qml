import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Window
import OmaGames

ApplicationWindow {
    id: win

    minimumWidth: Math.round(600 * theme.textScale)
    minimumHeight: Math.round(620 * theme.textScale)
    visible: true
    title: qsTr("Omadrop")
    color: theme.background
    Material.theme: theme.darkMode ? Material.Dark : Material.Light
    Material.accent: theme.accent
    Material.foreground: theme.foreground
    Material.background: theme.background

    readonly property bool inGame: game.phase !== "start"
    property bool pausedBeforeLeaving: false

    function refocus() {
        if (screen.item)
            (screen.item as Item).forceActiveFocus();
    }

    function leaveGame() {
        if (game.phase === "playing") {
            pausedBeforeLeaving = game.paused;
            confirmLoader.active = true;
            game.pause();
        } else {
            game.backToStart();
        }
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
        sourceComponent: PauseOverlay { onLeaveRequested: win.leaveGame() }
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
        sourceComponent: OmaConfirmDialog {
            message: qsTr("Leave this run? Your current score will be lost.")
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
            width = Math.round(760 * theme.textScale);
            height = Math.round(820 * theme.textScale);
        }
    }

    Component.onDestruction: game.saveWindowGeometry(
        normalGeometry.x, normalGeometry.y,
        normalGeometry.width, normalGeometry.height, wasMaximized)
}
