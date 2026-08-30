#pragma once

#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>
#include <memory>

#include "autoshift.h"
#include "game.h"
#include "modes.h"
#include "pacer.h"
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
    Q_PROPERTY(QVariantList modes READ modes CONSTANT)
    Q_PROPERTY(int score READ score NOTIFY scoreChanged)
    Q_PROPERTY(int level READ level NOTIFY levelChanged)
    Q_PROPERTY(int lines READ lines NOTIFY linesChanged)
    Q_PROPERTY(int lineGoal READ lineGoal NOTIFY modeChanged)
    Q_PROPERTY(int linesLeft READ linesLeft NOTIFY linesChanged)
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
    Q_PROPERTY(int stepInterval READ stepInterval WRITE setStepInterval NOTIFY stepIntervalChanged)

public:
    // One simulation tick per timer shot: 60 ticks a second.
    static constexpr int kDefaultStepIntervalMs = 16;

    explicit OmatrisGame(QObject *parent = nullptr);

    QString phase() const;
    bool paused() const { return m_game && m_game->paused(); }
    // Whether the well outlines where the falling piece would land.
    bool ghostEnabled() const { return m_ghostEnabled; }
    // "marathon" | "sprint" | "zen": the one being played, or the last chosen.
    QString mode() const { return Modes::id(m_mode); }
    QString modeLabel() const { return Modes::label(m_mode); }
    bool rankByTime() const { return Rules::params(m_mode).rankByTime; }
    // {id, label, description, goal} for the start screen, in play order.
    static QVariantList modes() { return Modes::list(); }
    int score() const { return m_game ? m_game->score() : 0; }
    int level() const { return m_game ? m_game->level() : 0; }
    int lines() const { return m_game ? m_game->lines() : 0; }
    int lineGoal() const { return Rules::params(m_mode).lineGoal; }
    int linesLeft() const { return m_game ? m_game->linesLeft() : lineGoal(); }
    int elapsedMs() const { return m_game ? m_game->elapsedMs() : 0; }
    int combo() const { return m_game ? m_game->combo() : -1; }
    bool backToBack() const { return m_game && m_game->backToBack(); }
    // PieceType as a number, kPieceCount for an empty hold box.
    int holdPiece() const;
    bool holdAvailable() const { return m_game && m_game->holdAvailable(); }
    QVariantList nextQueue() const;
    static int boardWidth() { return Board::kWidth; }
    static int boardHeight() { return Board::kVisibleHeight; }
    // {mode, label, score, lines, level, millis, date} for every table, best
    // first, and mode id -> the number that mode is judged on.
    QVariantList highScores() const { return Modes::scoreRows(m_scores); }
    QVariantMap bests() const { return Modes::bests(m_scores); }
    int newHighScoreRank() const { return m_newHighScoreRank; }
    int stepInterval() const { return m_pacer.interval(); }
    void setStepInterval(int interval);

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
    Q_INVOKABLE void step();
    // {cells: [{x, y}], width, height} of a piece in its spawn orientation,
    // for the hold box and the next queue.
    Q_INVOKABLE QVariantMap pieceShape(int piece) const;

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
    void stepIntervalChanged();
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
        QVariantList queue;
    };

    bool playing() const;
    void press(int direction);
    void release(int direction);
    void turn(int quarters);
    void apply(const std::vector<Event> &events);
    void handle(const Event &event);
    void announce(const ClearInfo &clear, QPoint where);
    Snapshot snapshot() const;
    void publish(const Snapshot &before);
    void finishGame();
    void syncTimer();
    void loadSettings();

    std::unique_ptr<Game> m_game;
    OmaGames::ScoreTable m_scores;
    OmaGames::Pacer m_pacer;
    Mode m_mode = Mode::Marathon;
    AutoShift m_shift;
    int m_newHighScoreRank = -1;
    bool m_ghostEnabled = true;
};
