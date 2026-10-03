import QtQuick
import QtQuick.Layouts
import OmaGames

// The end of a run, either way it ended: a Sprint that crossed forty lines, a
// Challenge whose dealt rows are all gone, or a stack that reached the ceiling;
// or the end of a replay, which may stop before its game did.
OmaOverlayPanel {
    id: root

    readonly property bool won: game.phase === "finished"
    // A finished Sprint or Challenge is told by the clock, everything else by
    // the score.
    readonly property bool timed: root.won && (game.rankByTime || game.dealtStack)
    readonly property string headline: root.timed
        ? clock.text(game.elapsedMs) : game.score.toLocaleString(Qt.locale(), "f", 0)
    readonly property bool ranked: game.newHighScoreRank >= 0
    // A replay can stop with its game still going: it was cut short there.
    readonly property bool cutShort: game.replaying && game.phase === "playing"

    function again() {
        if (game.replaying)
            game.restartReplay();
        else
            game.newGame(game.mode);
    }

    TimeFormat { id: clock }

    Keys.onPressed: function(event) {
        if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter || event.key === Qt.Key_Space || event.key === Qt.Key_R)
            root.again();
        else if (event.key === Qt.Key_Escape)
            game.backToStart();
        else
            return;
        event.accepted = true;
    }

    Text {
        Layout.alignment: Qt.AlignHCenter
        text: root.cutShort ? qsTr("End of the replay")
            : !root.won ? qsTr("Game over")
            : game.dealtStack ? qsTr("All clear!") : qsTr("%1 lines!").arg(game.lineGoal)
        color: root.cutShort ? theme.accent : root.won ? theme.green : theme.red
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
        // A Challenge keeps no table, so there is no best to hold it against,
        // and a replay is not the player's to hold against one.
        visible: game.ranked && !game.replaying
        text: root.ranked ? qsTr("New best · #%1").arg(game.newHighScoreRank + 1)
              : best <= 0 ? ""
              : game.rankByTime ? qsTr("Best %1").arg(clock.text(best))
                                : qsTr("Best %1").arg(best.toLocaleString(Qt.locale(), "f", 0))
        color: root.ranked ? theme.yellow : theme.mix(theme.background, theme.foreground, 0.7)
        font.pixelSize: 16 * theme.textScale
        font.bold: root.ranked
    }
    Text {
        Layout.alignment: Qt.AlignHCenter
        visible: game.replaying && game.replayAgent !== ""
        text: qsTr("Played by %1").arg(game.replayAgent)
        color: theme.mix(theme.background, theme.foreground, 0.7)
        font.pixelSize: 14 * theme.textScale
    }
    OmaHintButton {
        Layout.fillWidth: true
        Layout.topMargin: 6 * theme.textScale
        text: game.replaying ? qsTr("Watch again") : qsTr("Play again")
        primary: true
        hint: qsTr("Enter")
        onClicked: root.again()
    }
    OmaHintButton {
        Layout.fillWidth: true
        text: qsTr("Back to start")
        hint: qsTr("Esc")
        onClicked: game.backToStart()
    }
}
