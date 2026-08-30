import QtQuick
import OmaGames

// The keys that matter during play, each as a keycap beside what it does.
// `model` is a list of {key, label}. It takes the width it is given and
// wraps, staying centred: the flow is as wide as its content fits, never
// wider than the item.
Item {
    id: root

    property var model: []
    property bool compact: false
    property real spacing: 14 * theme.textScale

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
        spacing: root.spacing

        Repeater {
            model: root.model

            Row {
                required property var modelData
                spacing: 5 * theme.textScale

                OmaKeyHint {
                    anchors.verticalCenter: parent.verticalCenter
                    key: parent.modelData.key
                }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: parent.modelData.label
                    color: theme.mix(theme.background, theme.foreground, root.compact ? 0.5 : 0.6)
                    font.pixelSize: (root.compact ? 11 : 12) * theme.textScale
                }
            }
        }
    }
}
