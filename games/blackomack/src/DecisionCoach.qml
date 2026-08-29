import QtQuick
import OmaGames

// Text-only learning aid. The play itself is decided by BlackjackGame; this
// component only chooses between "off", "one live suggestion" and "on, but
// nothing to suggest right now". Its box never changes size, so the panel
// stays in one place whatever the table is doing.
Item {
    id: coach
    property bool compact: false

    readonly property bool live: game.coachAction !== ""
    implicitWidth: (coach.compact ? 210 : 250) * Math.min(theme.textScale, 1.4)
    implicitHeight: 84 * theme.textScale

    OmaHintButton {
        visible: !game.coachEnabled
        text: "Coach"
        hint: "C"
        fontScale: 0.82
        onClicked: game.toggleCoach()
    }

    OmaPanel {
        id: panel
        visible: game.coachEnabled
        width: coach.width
        implicitHeight: content.implicitHeight + 2 * padding
        padding: 9 * theme.textScale
        color: theme.mix(theme.darkBackground, theme.yellow, 0.035)
        border.color: theme.alpha(theme.yellow, coach.live ? 0.55 : 0.25)

        // Turning the coach back off stays reachable with the mouse, and the
        // key that does it is on the panel in both states.
        MouseArea {
            anchors.fill: parent
            onClicked: game.toggleCoach()
        }

        Column {
            id: content
            width: parent.width
            spacing: 3 * theme.textScale

            // A caption names the box, so a glance (or a screenshot) says what
            // the big verb is: advice, not a command.
            Item {
                visible: coach.live
                width: parent.width
                height: caption.implicitHeight
                Text {
                    id: caption
                    text: "Coach"
                    color: theme.foreground
                    opacity: 0.75
                    font.pixelSize: 11 * theme.textScale
                }
                OmaKeyHint {
                    key: "C"
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
            Text {
                visible: coach.live
                width: parent.width
                text: game.coachAction
                color: theme.yellow
                font.pixelSize: (coach.compact ? 19 : 22) * theme.textScale
                font.bold: true
                elide: Text.ElideRight
            }
            Text {
                visible: coach.live
                width: parent.width
                text: game.coachSituation
                color: theme.foreground
                opacity: 0.8
                font.pixelSize: 11 * theme.textScale
                wrapMode: Text.WordWrap
            }

            Row {
                visible: !coach.live
                spacing: 6 * theme.textScale
                Text {
                    text: "Coach on"
                    color: theme.foreground
                    opacity: 0.75
                    font.pixelSize: 11 * theme.textScale
                    anchors.verticalCenter: parent.verticalCenter
                }
                OmaKeyHint {
                    key: "C"
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
        }
    }
}
