import QtQuick

// The dimmed cover the kept table sits on. Not OmaOverlayPanel: that shell
// brings its own panel, and OmaScoresPanel already is one — nesting them
// would draw a border inside a border.
Rectangle {
    id: root

    signal closeRequested()

    color: theme.alpha(theme.background, 0.82)
    focus: true

    Keys.onPressed: function(event) {
        if (event.key === Qt.Key_Escape || event.key === Qt.Key_S)
            root.closeRequested();
        event.accepted = true;
    }

    MouseArea {
        anchors.fill: parent
        onClicked: root.closeRequested()
    }

    HighScoresPanel {
        anchors.centerIn: parent
        onCloseRequested: root.closeRequested()
    }
}
