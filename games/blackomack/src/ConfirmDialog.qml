import QtQuick
import QtQuick.Controls
import OmaGames

// Modal yes/no prompt; Escape cancels, Enter confirms.
Popup {
    id: dialog
    property string title: ""
    property string body: ""
    property string acceptText: "OK"
    property bool cancellable: true
    signal accepted()

    modal: true
    focus: true
    closePolicy: cancellable ? Popup.CloseOnEscape : Popup.NoAutoClose
    anchors.centerIn: parent
    padding: 0
    background: Item {}

    OmaPanel {
        padding: 24 * theme.textScale
        implicitWidth: content.implicitWidth + 2 * padding
        implicitHeight: content.implicitHeight + 2 * padding
        Column {
            id: content
            spacing: 16 * theme.textScale
            Text { text: dialog.title; color: theme.foreground; font.pixelSize: 20 * theme.textScale; font.bold: true }
            Text { text: dialog.body; color: theme.foreground; opacity: 0.8; font.pixelSize: 14 * theme.textScale; width: 320 * theme.textScale; wrapMode: Text.WordWrap }
            Row {
                spacing: 8 * theme.textScale
                anchors.right: parent.right
                OmaButton { visible: dialog.cancellable; text: "Cancel"; onClicked: dialog.close() }
                OmaButton { text: dialog.acceptText; primary: true; onClicked: { dialog.accepted(); dialog.close(); } }
            }
        }
    }
    Keys.onReturnPressed: { accepted(); close(); }
    Keys.onEnterPressed: { accepted(); close(); }
}
