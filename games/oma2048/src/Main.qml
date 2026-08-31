import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material

ApplicationWindow {
    id: win

    minimumWidth: Math.round(640 * theme.textScale)
    minimumHeight: Math.round(520 * theme.textScale)
    visible: true
    title: qsTr("Oma2048")
    color: theme.background
    Material.theme: theme.darkMode ? Material.Dark : Material.Light
    Material.accent: theme.accent
    Material.foreground: theme.foreground
    Material.background: theme.background

    Shortcut {
        sequences: ["Ctrl+Q"]
        context: Qt.ApplicationShortcut
        onActivated: Qt.quit()
    }

    Label {
        anchors.centerIn: parent
        text: qsTr("Oma2048")
        color: theme.foreground
        font.pixelSize: Math.round(32 * theme.textScale)
    }
}
