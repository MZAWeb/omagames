import QtQuick
import QtQuick.Layouts
import OmaGames

// Minimal themed yes/no prompt: Enter or Y confirms, Escape or N backs out.
// The hints name the keys the game wants shown — Omasweeper puts Y and N on
// the buttons, everyone else Enter and Esc.
OmaOverlayPanel {
    id: root

    property string message: ""
    property string acceptText: qsTr("Yes")
    property string rejectText: qsTr("Cancel")
    property string acceptHint: qsTr("Enter")
    property string rejectHint: qsTr("Esc")
    signal accepted()
    signal rejected()

    onDismissed: root.rejected()

    Keys.onPressed: function(event) {
        if (event.key === Qt.Key_Escape || event.key === Qt.Key_N)
            root.rejected();
        else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter || event.key === Qt.Key_Y)
            root.accepted();
        event.accepted = true;
    }

    Text {
        Layout.fillWidth: true
        text: root.message
        color: theme.foreground
        font.pixelSize: 15 * theme.textScale
        wrapMode: Text.WordWrap
        horizontalAlignment: Text.AlignHCenter
    }
    RowLayout {
        Layout.fillWidth: true
        spacing: 8 * theme.textScale
        OmaHintButton {
            Layout.fillWidth: true
            text: root.rejectText
            hint: root.rejectHint
            onClicked: root.rejected()
        }
        OmaHintButton {
            Layout.fillWidth: true
            text: root.acceptText
            primary: true
            hint: root.acceptHint
            onClicked: root.accepted()
        }
    }
}
