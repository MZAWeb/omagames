import QtQuick
import QtQuick.Layouts

// The whole game screen: header, the board centred in the room that is left,
// and the key legend. All rules live in `game`; this only renders state and
// forwards keys.
FocusScope {
    id: root

    // Off while an overlay or dialog is up, so the board underneath holds still.
    property bool inputEnabled: true
    signal newGameRequested()

    Keys.onPressed: function(event) {
        if (!root.inputEnabled)
            return;
        switch (event.key) {
        case Qt.Key_Left: case Qt.Key_H: game.moveLeft(); break;
        case Qt.Key_Right: case Qt.Key_L: game.moveRight(); break;
        case Qt.Key_Up: case Qt.Key_K: game.moveUp(); break;
        case Qt.Key_Down: case Qt.Key_J: game.moveDown(); break;
        case Qt.Key_U: game.undo(); break;
        case Qt.Key_N: root.newGameRequested(); break;
        default: return;
        }
        event.accepted = true;
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 14 * theme.textScale
        spacing: 10 * theme.textScale

        PlayHeader {
            Layout.fillWidth: true
            onNewGameRequested: root.newGameRequested()
        }

        BoardView {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        KeyLegend {
            Layout.fillWidth: true
            compact: true
        }
    }
}
