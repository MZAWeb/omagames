import QtQuick
import QtQuick.Controls
import OmaGames

// Modal yes/no prompt; Enter or Y confirms, Escape or N cancels. Key handling
// lives on the panel because Keys only attaches to Items, not to the Popup.
Popup {
    id: dialog
    property string title: ""
    property string body: ""
    property string acceptText: "OK"
    property bool cancellable: true
    signal accepted()

    function accept() { accepted(); close(); }

    modal: true
    focus: true
    closePolicy: cancellable ? Popup.CloseOnEscape : Popup.NoAutoClose
    anchors.centerIn: parent
    padding: 0
    background: Item {}

    OmaPanel {
        focus: true
        padding: 24 * theme.textScale
        implicitWidth: content.implicitWidth + 2 * padding
        implicitHeight: content.implicitHeight + 2 * padding
        Keys.onReturnPressed: dialog.accept()
        Keys.onEnterPressed: dialog.accept()
        Keys.onPressed: (event) => {
            if (event.key === Qt.Key_Y) { dialog.accept(); event.accepted = true; }
            else if (event.key === Qt.Key_N && dialog.cancellable) { dialog.close(); event.accepted = true; }
        }
        Column {
            id: content
            spacing: 16 * theme.textScale
            Text { text: dialog.title; color: theme.foreground; font.pixelSize: 20 * theme.textScale; font.bold: true }
            Text { text: dialog.body; color: theme.foreground; opacity: 0.8; font.pixelSize: 14 * theme.textScale; width: 320 * theme.textScale; wrapMode: Text.WordWrap }
            Row {
                spacing: 8 * theme.textScale
                anchors.right: parent.right
                HintButton { visible: dialog.cancellable; text: "Cancel"; hint: "N"; onClicked: dialog.close() }
                HintButton { text: dialog.acceptText; hint: "Y"; primary: true; onClicked: dialog.accept() }
            }
        }
    }
}
