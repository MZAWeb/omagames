import QtQuick
import OmaGames

// The keys that matter during play, each as a keycap beside its action.
Flow {
    id: root

    property bool compact: false

    spacing: 14 * theme.textScale

    Repeater {
        model: [
            { key: qsTr("←↑↓→"), action: qsTr("or hjkl to move") },
            { key: qsTr("Space"), action: qsTr("pause") },
            { key: qsTr("R"), action: qsTr("restart level") },
            { key: qsTr("Esc"), action: qsTr("leave") },
            { key: qsTr("Ctrl+Q"), action: qsTr("quit") }
        ]

        Row {
            required property var modelData
            spacing: 5 * theme.textScale

            OmaKeyHint {
                anchors.verticalCenter: parent.verticalCenter
                key: parent.modelData.key
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: parent.modelData.action
                color: theme.mix(theme.background, theme.foreground, root.compact ? 0.5 : 0.6)
                font.pixelSize: (root.compact ? 11 : 12) * theme.textScale
            }
        }
    }
}
