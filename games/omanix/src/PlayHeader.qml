import QtQuick
import QtQuick.Layouts

// Level, score, lives, progress toward the goal and the difficulty in play.
RowLayout {
    id: root

    spacing: 18 * theme.textScale

    // The score counts up rather than jumping, so a big claim reads as one.
    property int shownScore: game.score
    Behavior on shownScore {
        NumberAnimation { duration: 350; easing.type: Easing.OutCubic }
    }

    Text {
        text: qsTr("Level %1").arg(game.level)
        color: theme.foreground
        font.pixelSize: 17 * theme.textScale
        font.bold: true
    }
    Text {
        text: game.difficultyLabel
        color: theme.mix(theme.background, theme.foreground, 0.6)
        font.pixelSize: 14 * theme.textScale
    }

    Item { Layout.fillWidth: true }

    ColumnLayout {
        spacing: 3 * theme.textScale
        Layout.preferredWidth: 180 * theme.textScale

        RowLayout {
            Layout.fillWidth: true
            Text {
                text: qsTr("%1%").arg(Math.floor(game.claimedPercent))
                color: game.claimedPercent >= game.goalPercent ? theme.green : theme.foreground
                font.pixelSize: 14 * theme.textScale
                font.bold: true
            }
            Item { Layout.fillWidth: true }
            Text {
                text: qsTr("goal %1%").arg(game.goalPercent)
                color: theme.mix(theme.background, theme.foreground, 0.5)
                font.pixelSize: 11 * theme.textScale
            }
        }
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 6 * theme.textScale
            radius: height / 2
            color: theme.alpha(theme.foreground, 0.12)
            Rectangle {
                width: parent.width * Math.min(1, game.claimedPercent / game.goalPercent)
                height: parent.height
                radius: parent.radius
                color: game.claimedPercent >= game.goalPercent ? theme.green : theme.accent
                Behavior on width { NumberAnimation { duration: 250 } }
            }
        }
    }

    Item { Layout.fillWidth: true }

    Row {
        spacing: 4 * theme.textScale
        Repeater {
            model: game.lives
            Rectangle {
                width: 12 * theme.textScale
                height: width
                radius: 3
                color: theme.brightForeground
                border.width: 1
                border.color: theme.alpha(theme.background, 0.6)
            }
        }
    }

    Text {
        Layout.minimumWidth: 90 * theme.textScale
        horizontalAlignment: Text.AlignRight
        text: root.shownScore.toLocaleString(Qt.locale(), "f", 0)
        color: theme.foreground
        font.pixelSize: 20 * theme.textScale
        font.bold: true
    }
}
