import QtQuick
import OmaGames

// The second setting beside the board, drawn exactly like the validation one:
// filled while on, outlined while off. On, the board pencils every empty cell
// with the digits it still allows and the marks you made wait underneath.
OmaHintButton {
    id: toggle

    text: qsTr("Auto-notes")
    hint: qsTr("A")
    primary: game.autoNotes
    onClicked: game.autoNotes = !game.autoNotes
}
