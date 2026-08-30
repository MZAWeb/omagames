import QtQuick
import QtQuick.Layouts

// Mode and difficulty, the snake's length, the speed it is running at, and
// the score against the best of this table.
RowLayout {
    id: root

    spacing: 18 * theme.textScale

    // The score counts up rather than jumping, so a bonus reads as one move.
    property int shownScore: game.score
    Behavior on shownScore {
        NumberAnimation { duration: 300; easing.type: Easing.OutCubic }
    }
    readonly property bool beatingBest: game.score > game.best

    Text {
        text: game.modeLabel
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
        spacing: 0
        Text {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("%1 long").arg(game.length)
            color: theme.foreground
            font.pixelSize: 15 * theme.textScale
            font.bold: true
        }
        Text {
            Layout.alignment: Qt.AlignHCenter
            text: game.multiplier > 1 ? qsTr("×%1 a dot").arg(game.multiplier) : qsTr("×1 a dot")
            color: game.multiplier > 1 ? theme.accent : theme.mix(theme.background, theme.foreground, 0.55)
            font.pixelSize: 11 * theme.textScale
        }
    }

    ColumnLayout {
        spacing: 0
        Text {
            id: speed
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("%1 cells/s").arg(game.cellsPerSecond.toLocaleString(Qt.locale(), "f", 1))
            color: theme.foreground
            font.pixelSize: 15 * theme.textScale
            font.bold: true

            // A step up on the ladder flashes, so it is felt as well as seen.
            SequentialAnimation {
                id: speedFlash
                ColorAnimation { target: speed; property: "color"; to: theme.green; duration: 120 }
                ColorAnimation { target: speed; property: "color"; to: theme.foreground; duration: 600 }
            }
            Connections {
                target: game
                function onSpedUp() { speedFlash.restart(); }
            }
        }
        Text {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("speed")
            color: theme.mix(theme.background, theme.foreground, 0.55)
            font.pixelSize: 11 * theme.textScale
        }
    }

    Item { Layout.fillWidth: true }

    ColumnLayout {
        spacing: 0
        Text {
            Layout.alignment: Qt.AlignRight
            Layout.minimumWidth: 90 * theme.textScale
            horizontalAlignment: Text.AlignRight
            text: root.shownScore.toLocaleString(Qt.locale(), "f", 0)
            color: root.beatingBest ? theme.accent : theme.foreground
            font.pixelSize: 20 * theme.textScale
            font.bold: true
            Behavior on color { ColorAnimation { duration: 200 } }
        }
        Text {
            Layout.alignment: Qt.AlignRight
            text: qsTr("Best %1").arg(game.best.toLocaleString(Qt.locale(), "f", 0))
            color: theme.mix(theme.background, theme.foreground, 0.55)
            font.pixelSize: 11 * theme.textScale
        }
    }
}
