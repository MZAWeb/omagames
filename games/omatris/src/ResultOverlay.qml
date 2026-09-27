import QtQuick
import QtQuick.Layouts
import OmaGames

// The end of a run, either way it ended: a Sprint that crossed forty lines, a
// Challenge whose dealt rows are all gone, or a stack that reached the ceiling.
OmaOverlayPanel {
    id: root

    readonly property bool won: game.phase === "finished"
    // A finished Sprint or Challenge is told by the clock, everything else by
    // the score.
    readonly property bool timed: root.won && (game.rankByTime || game.dealtStack)
    readonly property string headline: root.timed
        ? clock.text(game.elapsedMs) : game.score.toLocaleString(Qt.locale(), "f", 0)
    readonly property bool ranked: game.newHighScoreRank >= 0

    TimeFormat { id: clock }

    Keys.onPressed: function(event) {
        if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter || event.key === Qt.Key_Space || event.key === Qt.Key_R)
            game.newGame(game.mode);
        else if (event.key === Qt.Key_Escape)
            game.backToStart();
        else
            return;
        event.accepted = true;
    }

    Text {
        Layout.alignment: Qt.AlignHCenter
        text: !root.won ? qsTr("Game over")
            : game.dealtStack ? qsTr("All clear!") : qsTr("%1 lines!").arg(game.lineGoal)
        color: root.won ? theme.green : theme.red
        font.pixelSize: 30 * theme.textScale
        font.bold: true
    }
    Text {
        Layout.alignment: Qt.AlignHCenter
        text: root.headline
        color: theme.foreground
        font.pixelSize: 34 * theme.textScale
        font.bold: true
    }
    Text {
        Layout.alignment: Qt.AlignHCenter
        text: game.dealtStack
            ? qsTr("%1 · %2 of %3 rows cleared · %4 points").arg(game.modeLabel)
                .arg(game.dealtRows - game.dealtRowsLeft).arg(game.dealtRows)
                .arg(game.score.toLocaleString(Qt.locale(), "f", 0))
            : root.timed
            ? qsTr("%1 · %2 points").arg(game.modeLabel).arg(game.score.toLocaleString(Qt.locale(), "f", 0))
            : qsTr("%1 · level %2 · %3 lines").arg(game.modeLabel).arg(game.level).arg(game.lines)
        color: theme.mix(theme.background, theme.foreground, 0.7)
        font.pixelSize: 14 * theme.textScale
    }
    Text {
        Layout.alignment: Qt.AlignHCenter
        readonly property int best: game.bests[game.mode]
        // A Challenge keeps no table, so there is no best to hold it against.
        visible: game.ranked
        text: root.ranked ? qsTr("New best · #%1").arg(game.newHighScoreRank + 1)
              : best <= 0 ? ""
              : game.rankByTime ? qsTr("Best %1").arg(clock.text(best))
                                : qsTr("Best %1").arg(best.toLocaleString(Qt.locale(), "f", 0))
        color: root.ranked ? theme.yellow : theme.mix(theme.background, theme.foreground, 0.7)
        font.pixelSize: 16 * theme.textScale
        font.bold: root.ranked
    }
    OmaHintButton {
        Layout.fillWidth: true
        Layout.topMargin: 6 * theme.textScale
        text: qsTr("Play again")
        primary: true
        hint: qsTr("Enter")
        onClicked: game.newGame(game.mode)
    }
    OmaHintButton {
        Layout.fillWidth: true
        text: qsTr("Back to start")
        hint: qsTr("Esc")
        onClicked: game.backToStart()
    }
}
