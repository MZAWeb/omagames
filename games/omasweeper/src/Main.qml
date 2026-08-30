import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Window

ApplicationWindow {
    id: win

    minimumWidth: Math.round(480 * theme.textScale)
    minimumHeight: Math.round(360 * theme.textScale)
    visible: true
    title: qsTr("Omasweeper")
    color: theme.background
    Material.theme: theme.darkMode ? Material.Dark : Material.Light
    Material.accent: theme.accent
    Material.foreground: theme.foreground
    Material.background: theme.background

    Label {
        anchors.centerIn: parent
        text: qsTr("Omasweeper")
        color: theme.foreground
        font.pixelSize: Math.round(32 * theme.textScale)
    }

    Shortcut {
        sequences: [StandardKey.Quit, "Ctrl+Q"]
        onActivated: Qt.quit()
    }
}
