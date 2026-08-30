import QtQuick
import QtQuick.Layouts
import OmaGames

// The end of a run, either way it ended: a Sprint that crossed forty lines,
// or a stack that reached the ceiling.
OmaOverlayPanel {
    id: root

    readonly property bool won: game.phase === "finished"
    // Sprint is judged on the clock, everything else on the score.
    readonly property string headline: game.rankByTime && root.won
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
        text: root.won ? qsTr("%1 lines!").arg(game.lineGoal) : qsTr("Game over")
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
        text: game.rankByTime && root.won
            ? qsTr("%1 · %2 points").arg(game.modeLabel).arg(game.score.toLocaleString(Qt.locale(), "f", 0))
            : qsTr("%1 · level %2 · %3 lines").arg(game.modeLabel).arg(game.level).arg(game.lines)
        color: theme.mix(theme.background, theme.foreground, 0.7)
        font.pixelSize: 14 * theme.textScale
    }
    Text {
        Layout.alignment: Qt.AlignHCenter
        readonly property int best: game.bests[game.mode]
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
