import QtQuick
import QtQuick.Layouts

// Mode on the left, then the numbers that matter: score, level and lines in
// Marathon and Zen; lines left and the clock in Sprint.
RowLayout {
    id: root

    spacing: 16 * theme.textScale

    readonly property int best: game.bests[game.mode]
    // The score counts up rather than jumping, so a Tetris reads as one.
    property int shownScore: game.score
    Behavior on shownScore {
        NumberAnimation { duration: 320; easing.type: Easing.OutCubic }
    }

    readonly property var stats: game.rankByTime
        ? [{ label: qsTr("Lines left"), value: game.linesLeft.toString(), lead: true },
           { label: qsTr("Time"), value: clock.text(game.elapsedMs), lead: true },
           { label: qsTr("Score"), value: root.shownScore.toLocaleString(Qt.locale(), "f", 0), lead: false }]
        : [{ label: qsTr("Score"), value: root.shownScore.toLocaleString(Qt.locale(), "f", 0), lead: true },
           { label: qsTr("Level"), value: game.level.toString(), lead: false },
           { label: qsTr("Lines"), value: game.lines.toString(), lead: false }]

    readonly property string bestText: root.best <= 0 ? ""
        : game.rankByTime ? clock.text(root.best) : root.best.toLocaleString(Qt.locale(), "f", 0)

    TimeFormat { id: clock }

    ColumnLayout {
        spacing: 0
        Text {
            text: game.modeLabel
            color: theme.foreground
            font.pixelSize: 17 * theme.textScale
            font.bold: true
        }
        Text {
            visible: root.bestText !== ""
            text: qsTr("Best %1").arg(root.bestText)
            color: theme.mix(theme.background, theme.foreground, 0.55)
            font.pixelSize: 11 * theme.textScale
        }
    }

    Item { Layout.fillWidth: true }

    // Back-to-back and the combo count are the state a player is nursing, so
    // they sit in the header while they last.
    Row {
        spacing: 6 * theme.textScale

        Rectangle {
            visible: game.backToBack
            width: b2b.implicitWidth + 12 * theme.textScale
            height: b2b.implicitHeight + 5 * theme.textScale
            radius: 4
            color: theme.alpha(theme.yellow, 0.18)
            border.width: 1
            border.color: theme.alpha(theme.yellow, 0.5)
            Text {
                id: b2b
                anchors.centerIn: parent
                text: qsTr("B2B")
                color: theme.yellow
                font.pixelSize: 11 * theme.textScale
                font.bold: true
            }
        }
        Rectangle {
            visible: game.combo >= 1
            width: combo.implicitWidth + 12 * theme.textScale
            height: combo.implicitHeight + 5 * theme.textScale
            radius: 4
            color: theme.alpha(theme.accent, 0.18)
            border.width: 1
            border.color: theme.alpha(theme.accent, 0.5)
            Text {
                id: combo
                anchors.centerIn: parent
                text: qsTr("Combo x%1").arg(game.combo)
                color: theme.accent
                font.pixelSize: 11 * theme.textScale
                font.bold: true
            }
        }
    }

    Repeater {
        model: root.stats

        ColumnLayout {
            id: stat
            required property var modelData
            spacing: 0
            Layout.alignment: Qt.AlignRight

            Text {
                Layout.alignment: Qt.AlignRight
                text: stat.modelData.value
                color: theme.foreground
                font.pixelSize: (stat.modelData.lead ? 22 : 16) * theme.textScale
                font.bold: stat.modelData.lead
            }
            Text {
                Layout.alignment: Qt.AlignRight
                text: stat.modelData.label
                color: theme.mix(theme.background, theme.foreground, 0.55)
                font.pixelSize: 11 * theme.textScale
            }
        }
    }
}
