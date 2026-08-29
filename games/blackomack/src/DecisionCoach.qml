import QtQuick
import QtQuick.Layouts
import OmaGames

// Text-only learning aid. Strategy stays in BlackjackGame; this component
// only presents Off, On and Resolved states.
Item {
    id: coach
    property bool compact: false
    implicitWidth: game.coachEnabled ? panel.implicitWidth : offButton.implicitWidth
    implicitHeight: game.coachEnabled ? panel.implicitHeight : offButton.implicitHeight

    OmaHintButton {
        id: offButton
        visible: !game.coachEnabled
        text: "Coach off"
        hint: "C"
        fontScale: 0.82
        onClicked: game.toggleCoach()
    }

    OmaPanel {
        id: panel
        visible: game.coachEnabled
        padding: 9 * theme.textScale
        implicitWidth: 270 * Math.min(theme.textScale, 1.4)
        implicitHeight: content.implicitHeight + 2 * padding
        color: theme.mix(theme.darkBackground, theme.yellow, 0.035)
        border.color: theme.alpha(theme.yellow, game.coachAdvice === "" ? 0.28 : 0.55)

        Column {
            id: content
            width: panel.implicitWidth - 2 * panel.padding
            spacing: 5 * theme.textScale

            RowLayout {
                width: parent.width
                Text {
                    text: game.coachAdvice === "" ? "Decision coach · Resolved" : "Decision coach · On"
                    color: game.coachAdvice === "" ? theme.foreground : theme.yellow
                    opacity: game.coachAdvice === "" ? 0.72 : 1.0
                    font.pixelSize: 10 * theme.textScale
                    font.bold: true
                    Layout.fillWidth: true
                }
                OmaHintButton {
                    text: "On"
                    hint: "C"
                    fontScale: 0.7
                    implicitHeight: 28 * theme.textScale
                    onClicked: game.toggleCoach()
                }
            }
            Text {
                width: parent.width
                text: game.coachAdvice === "" ? "Hand resolved · no recommendation shown" : game.coachAdvice
                color: game.coachAdvice === "" ? theme.foreground : theme.yellow
                opacity: game.coachAdvice === "" ? 0.72 : 1.0
                font.pixelSize: (coach.compact ? 11 : 12) * theme.textScale
                font.bold: game.coachAdvice !== ""
                wrapMode: Text.WordWrap
            }
            Text {
                visible: game.coachAdvice !== ""
                width: parent.width
                text: "Basic strategy · no odds or promises"
                color: theme.foreground
                opacity: 0.72
                font.pixelSize: 11 * theme.textScale
                wrapMode: Text.WordWrap
            }
        }
    }
}
