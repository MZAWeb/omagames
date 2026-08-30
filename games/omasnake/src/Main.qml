import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material

ApplicationWindow {
    id: win

    width: Math.round(900 * theme.textScale)
    height: Math.round(700 * theme.textScale)
    minimumWidth: Math.round(640 * theme.textScale)
    minimumHeight: Math.round(520 * theme.textScale)
    visible: true
    title: qsTr("Omasnake")
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

    Text {
        anchors.centerIn: parent
        text: qsTr("Omasnake")
        color: theme.foreground
        font.pixelSize: 40 * theme.textScale
        font.bold: true
    }
}
