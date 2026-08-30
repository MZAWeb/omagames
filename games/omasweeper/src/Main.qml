import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Window
import OmaGames

// The window and the screen it shows. Expert is 30x16 cells, so the smallest
// window still gives every cell 20 px; below that the layout would start
// squeezing the header instead of the board.
ApplicationWindow {
    id: win

    minimumWidth: Math.round(640 * theme.textScale)
    minimumHeight: Math.round(480 * theme.textScale)
    visible: true
    title: qsTr("Omasweeper")
    color: theme.background
    Material.theme: theme.darkMode ? Material.Dark : Material.Light
    Material.accent: theme.accent
    Material.foreground: theme.foreground
    Material.background: theme.background

    readonly property bool inGame: game.phase !== "start"

    // Leaving a board that is still running throws it away, so it is asked
    // about first; a finished board has nothing left to lose.
    function leaveGame() {
        if (game.status === "playing")
            confirmLoader.active = true;
        else
            game.backToStart();
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
        active: game.phase === "won"
        sourceComponent: WinOverlay {}
        onLoaded: (item as Item).forceActiveFocus()
        onActiveChanged: if (!active) win.refocus()
    }

    Loader {
        anchors.fill: parent
        active: game.phase === "lost"
        sourceComponent: LossOverlay {}
        onLoaded: (item as Item).forceActiveFocus()
        onActiveChanged: if (!active) win.refocus()
    }

    Loader {
        id: confirmLoader
        anchors.fill: parent
        active: false
        sourceComponent: OmaConfirmDialog {
            acceptHint: qsTr("Y")
            rejectHint: qsTr("N")
            message: qsTr("Leave this board? It is thrown away, clock and all.")
            acceptText: qsTr("Leave")
            onAccepted: {
                confirmLoader.active = false;
                game.backToStart();
            }
            onRejected: {
                confirmLoader.active = false;
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
            width = Math.round(1000 * theme.textScale);
            height = Math.round(720 * theme.textScale);
        }
    }

    Component.onDestruction: game.saveWindowGeometry(
        normalGeometry.x, normalGeometry.y,
        normalGeometry.width, normalGeometry.height, wasMaximized)
}
