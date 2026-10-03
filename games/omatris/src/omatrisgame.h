#pragma once

#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>
#include <functional>
#include <memory>
#include <optional>

#include "autoshift.h"
#include "game.h"
#include "handling.h"
#include "modes.h"
#include "pacer.h"
#include "preferences.h"
#include "replaypace.h"
#include "replayplayer.h"
#include "replayplaylist.h"
#include "scoretable.h"

// The only bridge between the engine and QML: state as properties, actions as
// invokables, persistence and pacing. Modes cross to QML as lowercase id
// strings, pieces as their PieceType number, like the other games.
//
// Delayed auto shift lives beside it in AutoShift rather than in the engine,
// because it is the one rule that is about keys being held down: the engine
// only ever moves a piece one cell at a time.
class OmatrisGame : public QObject {
    Q_OBJECT
    // "start" | "playing" | "gameover" | "finished"
    Q_PROPERTY(QString phase READ phase NOTIFY phaseChanged)
    Q_PROPERTY(bool paused READ paused NOTIFY pausedChanged)
    Q_PROPERTY(bool ghostEnabled READ ghostEnabled NOTIFY ghostEnabledChanged)
    Q_PROPERTY(QString mode READ mode NOTIFY modeChanged)
    Q_PROPERTY(QString modeLabel READ modeLabel NOTIFY modeChanged)
    Q_PROPERTY(bool rankByTime READ rankByTime NOTIFY modeChanged)
    Q_PROPERTY(bool ranked READ ranked NOTIFY modeChanged)
    Q_PROPERTY(bool dealtStack READ dealtStack NOTIFY modeChanged)
    Q_PROPERTY(QVariantList modes READ modes CONSTANT)
    Q_PROPERTY(int score READ score NOTIFY scoreChanged)
    Q_PROPERTY(int level READ level NOTIFY levelChanged)
    Q_PROPERTY(int lines READ lines NOTIFY linesChanged)
    Q_PROPERTY(int lineGoal READ lineGoal NOTIFY modeChanged)
    Q_PROPERTY(int linesLeft READ linesLeft NOTIFY linesChanged)
    Q_PROPERTY(int dealtRows READ dealtRows NOTIFY modeChanged)
    Q_PROPERTY(int dealtRowsLeft READ dealtRowsLeft NOTIFY linesChanged)
    Q_PROPERTY(int difficulty READ difficulty NOTIFY difficultyChanged)
    Q_PROPERTY(int dealtDifficulty READ dealtDifficulty NOTIFY modeChanged)
    Q_PROPERTY(int elapsedMs READ elapsedMs NOTIFY elapsedChanged)
    Q_PROPERTY(int combo READ combo NOTIFY comboChanged)
    Q_PROPERTY(bool backToBack READ backToBack NOTIFY comboChanged)
    Q_PROPERTY(int holdPiece READ holdPiece NOTIFY holdChanged)
    Q_PROPERTY(bool holdAvailable READ holdAvailable NOTIFY holdChanged)
    Q_PROPERTY(QVariantList nextQueue READ nextQueue NOTIFY queueChanged)
    Q_PROPERTY(int boardWidth READ boardWidth CONSTANT)
    Q_PROPERTY(int boardHeight READ boardHeight CONSTANT)
    Q_PROPERTY(QVariantList highScores READ highScores NOTIFY highScoresChanged)
    Q_PROPERTY(QVariantMap bests READ bests NOTIFY highScoresChanged)
    Q_PROPERTY(int newHighScoreRank READ newHighScoreRank NOTIFY phaseChanged)
    Q_PROPERTY(QVariantList handling READ handlingRows NOTIFY handlingChanged)
    Q_PROPERTY(bool handlingIsDefault READ handlingIsDefault NOTIFY handlingChanged)
    Q_PROPERTY(int stepInterval READ stepInterval WRITE setStepInterval NOTIFY stepIntervalChanged)
    // Watching a recorded game (--replay) rather than playing one.
    Q_PROPERTY(bool replaying READ replaying NOTIFY replayChanged)
    Q_PROPERTY(QString replayAgent READ replayAgent NOTIFY replayChanged)
    // 1 to 4, slowest first, as keys 1 to 4 pick it; the label says how
    // fast that is against the clock the game was played on.
    Q_PROPERTY(int replaySpeed READ replaySpeed NOTIFY replayChanged)
    Q_PROPERTY(QString replaySpeedLabel READ replaySpeedLabel NOTIFY replayChanged)
    // Every call made: what is on screen is where the recording stops.
    Q_PROPERTY(bool replayEnded READ replayEnded NOTIFY replayChanged)
    // Given several replays, which one this is ("3 of 10", else empty), and
    // whether there is another either side of it.
    Q_PROPERTY(QString replayPosition READ replayPosition NOTIFY replayChanged)
    Q_PROPERTY(bool hasNextReplay READ hasNextReplay NOTIFY replayChanged)
    Q_PROPERTY(bool hasPreviousReplay READ hasPreviousReplay NOTIFY replayChanged)

public:
    // One simulation tick per timer shot: 60 ticks a second.
    static constexpr int kDefaultStepIntervalMs = 16;

    explicit OmatrisGame(QObject *parent = nullptr);

    QString phase() const;
    bool paused() const { return m_game && m_game->paused(); }
    // Whether the well outlines where the falling piece would land.
    bool ghostEnabled() const { return m_preferences.ghost(); }
    // "marathon" | "sprint" | "zen" | "challenge": the one being played, or
    // the last chosen.
    QString mode() const { return Modes::id(shownMode()); }
    QString modeLabel() const { return Modes::label(shownMode()); }
    bool rankByTime() const { return Rules::params(shownMode()).rankByTime; }
    bool ranked() const { return Modes::ranked(shownMode()); }
    // Whether the run began on a mess to clear: Challenge.
    bool dealtStack() const { return Rules::params(shownMode()).dealtStack; }
    // {id, label, description, goal, ranked} for the start screen, in play order.
    static QVariantList modes() { return Modes::list(); }
    int score() const { return m_game ? m_game->score() : 0; }
    int level() const { return m_game ? m_game->level() : 0; }
    int lines() const { return m_game ? m_game->lines() : 0; }
    int lineGoal() const { return Rules::params(shownMode()).lineGoal; }
    int linesLeft() const { return m_game ? m_game->linesLeft() : lineGoal(); }
    // The rows the dealt stack covered, and how many are still to clear.
    int dealtRows() const { return m_game ? m_game->dealtRows() : 0; }
    int dealtRowsLeft() const { return m_game ? m_game->dealtRowsLeft() : 0; }
    // 1-100, how hard the board left is to finish; 0 outside Challenge.
    int difficulty() const { return m_game ? m_game->difficulty() : 0; }
    // What it was on the deal, for the change since.
    int dealtDifficulty() const { return m_game ? m_game->dealtDifficulty() : 0; }
    int elapsedMs() const { return m_game ? m_game->elapsedMs() : 0; }
    int combo() const { return m_game ? m_game->combo() : -1; }
    bool backToBack() const { return m_game && m_game->backToBack(); }
    // PieceType as a number, kPieceCount for an empty hold box.
    int holdPiece() const { return m_game ? int(m_game->heldPiece()) : int(PieceType::None); }
    bool holdAvailable() const { return m_game && m_game->holdAvailable(); }
    QVariantList nextQueue() const;
    static int boardWidth() { return Board::kWidth; }
    static int boardHeight() { return Board::kVisibleHeight; }
    // {mode, label, score, lines, level, millis, date} for every table, best
    // first, and mode id -> the number that mode is judged on.
    QVariantList highScores() const { return Modes::scoreRows(m_scores); }
    QVariantMap bests() const { return Modes::bests(m_scores); }
    int newHighScoreRank() const { return m_newHighScoreRank; }
    const Handling &handling() const { return m_preferences.handling(); }
    // {id, label, description, value, canLower, canRaise, isDefault} for the
    // handling panel.
    QVariantList handlingRows() const { return handling().rows(); }
    bool handlingIsDefault() const { return handling().isDefault(); }
    int stepInterval() const { return m_pacer.interval(); }
    void setStepInterval(int interval);

    bool replaying() const { return m_replay.has_value(); }
    QString replayAgent() const { return m_replay ? m_replay->agent() : QString(); }
    int replaySpeed() const { return m_pace.speed(); }
    QString replaySpeedLabel() const { return m_pace.label(); }
    bool replayEnded() const { return m_replay && m_replay->done(); }
    QString replayPosition() const { return m_replay ? m_playlist.position() : QString(); }
    bool hasNextReplay() const { return m_replay && m_playlist.hasNext(); }
    bool hasPreviousReplay() const { return m_replay && m_playlist.hasPrevious(); }
    // Starts watching the first of `paths`, to be stepped through in order;
    // false and the reason when any of them is not one this game can play.
    bool loadReplays(const QStringList &paths, QString *error);
    bool loadReplay(const QString &path, QString *error) { return loadReplays({path}, error); }

    // Read-only view for the renderer; null on the start screen.
    const Game *engine() const { return m_game.get(); }
    // Scenario hook for tests only.
    Game *engineForTests() { return m_game.get(); }
    void startGame(Mode mode, quint32 seed);

    Q_INVOKABLE void newGame(const QString &mode);
    Q_INVOKABLE void restart();
    Q_INVOKABLE void backToStart();
    Q_INVOKABLE void pressLeft() { press(-1); }
    Q_INVOKABLE void releaseLeft() { release(-1); }
    Q_INVOKABLE void pressRight() { press(1); }
    Q_INVOKABLE void releaseRight() { release(1); }
    Q_INVOKABLE void setSoftDrop(bool on);
    Q_INVOKABLE void hardDrop();
    Q_INVOKABLE void rotateCw() { turn(1); }
    Q_INVOKABLE void rotateCcw() { turn(-1); }
    Q_INVOKABLE void swapHold();
    Q_INVOKABLE void pause();
    Q_INVOKABLE void resume();
    Q_INVOKABLE void togglePause();
    Q_INVOKABLE void toggleGhost();
    // One step of a handling setting ("das" | "arr" | "softDrop"), +1 or -1;
    // false at the end of its range. Takes effect at once, mid-run included.
    Q_INVOKABLE bool adjustHandling(const QString &setting, int delta);
    Q_INVOKABLE void resetHandling();
    Q_INVOKABLE void step();
    Q_INVOKABLE void setReplaySpeed(int speed);
    // Plays the replay on to the next piece that locks, paused or not.
    Q_INVOKABLE void replayNextPiece();
    Q_INVOKABLE void restartReplay();
    // The next or previous of the replays given, at the speed picked.
    Q_INVOKABLE void nextReplay();
    Q_INVOKABLE void previousReplay();
    // {cells: [{x, y}], width, height} of a piece in its spawn orientation,
    // for the hold box and the next queue.
    Q_INVOKABLE QVariantMap pieceShape(int piece) const { return Piece::spawnBoxMap(piece); }

    Q_INVOKABLE QVariantMap windowGeometry() const;
    Q_INVOKABLE void saveWindowGeometry(int x, int y, int width, int height, bool maximized);

signals:
    void phaseChanged();
    void pausedChanged();
    void ghostEnabledChanged();
    void modeChanged();
    void scoreChanged();
    void levelChanged();
    void linesChanged();
    void elapsedChanged();
    void comboChanged();
    void holdChanged();
    void queueChanged();
    void highScoresChanged();
    void difficultyChanged();
    void stepIntervalChanged();
    void replayChanged();
    void handlingChanged();
    // After anything that moved a piece, for the renderer.
    void frameChanged();
    // Animation cues. Cells and rows are board coordinates.
    void pieceLocked(const QVector<int> &cells);
    void linesCleared(const QVector<int> &rows);
    void bonusEarned(const QString &text, int x, int y);
    void levelReached(int level);

private:
    // Everything QML watches that the engine only computes on demand.
    struct Snapshot {
        int score = 0;
        int level = 0;
        int lines = 0;
        int elapsed = 0;
        int combo = -1;
        int hold = kPieceCount;
        bool holdAvailable = false;
        bool backToBack = false;
        int difficulty = 0;
        QVariantList queue;
    };

    bool playing() const { return m_game && m_game->phase() == Phase::Playing && !m_game->paused(); }
    // The game on screen's, which a replay's need not be the player's last.
    Mode shownMode() const { return m_game ? m_game->mode() : m_preferences.mode(); }
    // Puts `game` on screen, fresh, and tells QML everything changed.
    void show(std::unique_ptr<Game> game);
    // The keys move the piece: a game is on, and it is the player's.
    bool playerInControl() const { return playing() && !m_replay; }
    void press(int direction);
    void release(int direction);
    void turn(int quarters);
    void apply(const std::vector<Event> &events);
    void handle(const Event &event);
    void announce(const ClearInfo &clear, QPoint where);
    Snapshot snapshot() const;
    void publish(const Snapshot &before);
    void finishGame();
    // The pacer runs while, and only while, a piece can fall: in a replay,
    // while there is something left to show.
    void syncTimer() { m_pacer.setRunning(playing() && !replayEnded()); }
    // Past real speed a replay locks pieces faster than their popups can
    // rise and fade, and they would only pile up over the well.
    bool replayTooFastToRead() const;
    void replayBeats();
    // Makes `calls` on the replay's game and shows what they did.
    void playReplay(const std::function<std::vector<Event>()> &calls);
    void endReplay();
    // Puts the replay in `path` on screen from its start.
    bool watch(const QString &path, QString *error);
    void setHandling(const Handling &handling);
    void applyHandling();

    std::unique_ptr<Game> m_game;
    OmaGames::ScoreTable m_scores;
    OmaGames::Pacer m_pacer;
    AutoShift m_shift;
    Preferences m_preferences;
    int m_newHighScoreRank = -1;
    std::optional<ReplayPlayer> m_replay;
    OmaGames::ReplayPlaylist m_playlist;
    OmaGames::ReplayPace m_pace;
};
