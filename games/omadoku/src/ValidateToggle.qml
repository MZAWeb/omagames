import QtQuick
import OmaGames

// The one setting that belongs beside the board, drawn as a button that is
// filled while on and outlined while off, so its state reads at a glance among
// the plain actions. It says what it does in full wherever there is room.
OmaHintButton {
    id: toggle

    property bool compact: false

    text: compact ? qsTr("Validate") : qsTr("Validate as I go")
    hint: qsTr("V")
    primary: game.validateAsYouGo
    onClicked: game.validateAsYouGo = !game.validateAsYouGo
}
