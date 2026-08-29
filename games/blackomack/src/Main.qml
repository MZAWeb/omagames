import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import OmaGames

// Placeholder — replaced by the game agent. Proves the shared theme + import work.
ApplicationWindow {
    width: 480; height: 640; visible: true
    title: "Black Omack"
    color: theme.background
    Material.theme: theme.darkMode ? Material.Dark : Material.Light
    Material.accent: theme.accent

    Column {
        anchors.centerIn: parent
        spacing: 16
        Text { text: "Black Omack"; color: theme.foreground; font.pixelSize: 32 * theme.textScale; anchors.horizontalCenter: parent.horizontalCenter }
        OmaButton { text: "Primary"; primary: true; anchors.horizontalCenter: parent.horizontalCenter }
        OmaButton { text: "Ghost"; anchors.horizontalCenter: parent.horizontalCenter }
    }
}
