import QtQuick
import OmaGames

// An OmaButton that shows its keyboard shortcut as a keycap on the right, so
// every action on screen teaches its key. `showHint: false` hides the keycap
// for a key that is not live right now. The badge is not interactive, so a
// click anywhere on it still presses the button.
OmaButton {
    id: control

    property string hint: ""
    property bool showHint: true

    // Reserve the badge's slot so a narrow button's label never runs
    // underneath it, and so the label stays centred on a button that
    // stretches to fill its row.
    rightPadding: badge.visible ? badge.implicitWidth + padding + 6 * theme.textScale
                                : padding

    OmaKeyHint {
        id: badge
        anchors.right: parent.right
        anchors.rightMargin: 8 * theme.textScale
        anchors.verticalCenter: parent.verticalCenter
        key: control.showHint ? control.hint : ""
        onPrimary: control.primary
        active: control.enabled
    }
}
