import QtQuick
import QtQuick.Layouts
import OmaGames

// Minimal themed yes/no prompt (Escape with a game in progress).
OverlayPanel {
    id: root

    property string message: ""
    property string acceptText: qsTr("Yes")
    property string rejectText: qsTr("Cancel")
    signal accepted()
    signal rejected()

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
            hint: qsTr("Esc")
            onClicked: root.rejected()
        }
        OmaHintButton {
            Layout.fillWidth: true
            text: root.acceptText
            primary: true
            hint: qsTr("Enter")
            onClicked: root.accepted()
        }
    }
}
