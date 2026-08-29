import QtQuick
import OmaGames

// The keys that matter during play, each as a keycap beside its action. It
// takes the width it is given and wraps, staying centred: the flow is as
// wide as its content fits, never wider than the item.
Item {
    id: root

    property bool compact: false

    implicitHeight: flow.implicitHeight

    // What the entries would need on one line; the Repeater itself has no
    // width and is skipped.
    readonly property real naturalWidth: {
        var total = 0;
        for (var i = 0; i < flow.children.length; ++i) {
            var child = flow.children[i];
            if (child.implicitWidth > 0)
                total += child.implicitWidth + flow.spacing;
        }
        return Math.max(0, total - flow.spacing);
    }

    Flow {
        id: flow
        anchors.horizontalCenter: parent.horizontalCenter
        width: Math.min(root.width, root.naturalWidth)
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
}
