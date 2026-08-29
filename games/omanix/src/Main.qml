import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material

ApplicationWindow {
    visible: true
    title: qsTr("Omanix")
    color: theme.background
    Material.theme: theme.darkMode ? Material.Dark : Material.Light
    Material.accent: theme.accent
}
