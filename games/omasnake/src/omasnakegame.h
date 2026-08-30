#pragma once

#include <QObject>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>
#include <memory>

#include "game.h"
#include "highscores.h"

// The only bridge between the engine and QML: state as properties, actions as
// invokables, persistence and pacing. Screen state, mode and difficulty cross
// to QML as lowercase id strings, like the other games.
class OmasnakeGame : public QObject {
    Q_OBJECT
    // "start" | "playing" | "gameover"
    Q_PROPERTY(QString phase READ phase NOTIFY phaseChanged)
    Q_PROPERTY(bool paused READ paused NOTIFY pausedChanged)
    Q_PROPERTY(bool ready READ ready NOTIFY readyChanged)
    Q_PROPERTY(int score READ score NOTIFY scoreChanged)
    Q_PROPERTY(int length READ length NOTIFY lengthChanged)
    Q_PROPERTY(int multiplier READ multiplier NOTIFY lengthChanged)
    Q_PROPERTY(double cellsPerSecond READ cellsPerSecond NOTIFY speedChanged)
    Q_PROPERTY(int best READ best NOTIFY bestChanged)
    Q_PROPERTY(int fieldWidth READ fieldWidth CONSTANT)
    Q_PROPERTY(int fieldHeight READ fieldHeight CONSTANT)
    Q_PROPERTY(QString mode READ mode NOTIFY modeChanged)
    Q_PROPERTY(QString modeLabel READ modeLabel NOTIFY modeChanged)
    Q_PROPERTY(QVariantList modes READ modes CONSTANT)
    Q_PROPERTY(QString difficulty READ difficulty NOTIFY difficultyChanged)
    Q_PROPERTY(QString difficultyLabel READ difficultyLabel NOTIFY difficultyChanged)
    Q_PROPERTY(QVariantList difficulties READ difficulties CONSTANT)
    Q_PROPERTY(QVariantList highScores READ highScores NOTIFY highScoresChanged)
    Q_PROPERTY(QVariantMap bests READ bests NOTIFY highScoresChanged)
    Q_PROPERTY(int newHighScoreRank READ newHighScoreRank NOTIFY phaseChanged)
    // "wall" | "self" | "filled": what ended the run on screen now.
    Q_PROPERTY(QString gameOverReason READ gameOverReason NOTIFY phaseChanged)
    Q_PROPERTY(int stepInterval READ stepInterval WRITE setStepInterval NOTIFY stepIntervalChanged)

public:
    // One simulation tick per timer shot: 60 ticks a second.
    static constexpr int kDefaultStepIntervalMs = 1000 / Rules::kTicksPerSecond;

    explicit OmasnakeGame(QObject *parent = nullptr);

    QString phase() const;
    bool paused() const { return m_game && m_game->paused(); }
    // The opening beat, while the field is up and nothing moves yet.
    bool ready() const { return m_game && m_game->ready(); }
    int score() const { return m_game ? m_game->score() : 0; }
    int length() const { return m_game ? m_game->length() : 0; }
    int multiplier() const { return m_game ? m_game->multiplier() : 1; }
    double cellsPerSecond() const { return m_game ? m_game->cellsPerSecond() : 0.0; }
    // The best score of the mode and difficulty in play, or last chosen.
    int best() const { return m_scores.best(m_mode, m_difficulty); }
    static int fieldWidth() { return Game::kWidth; }
    static int fieldHeight() { return Game::kHeight; }
    // "classic" | "wrap".
    QString mode() const { return HighScores::modeId(m_mode); }
    QString modeLabel() const;
    // {id, label, description} for both walls rules, and for the three speeds.
    static QVariantList modes();
    // "slow" | "normal" | "fast".
    QString difficulty() const { return HighScores::difficultyId(m_difficulty); }
    QString difficultyLabel() const;
    static QVariantList difficulties();
    // {table, mode, difficulty, label, score, length, date} for every table.
    QVariantList highScores() const;
    // "<mode>-<difficulty>" → best score.
    QVariantMap bests() const;
    // Where the finished run landed in its table (0 = top), or -1.
    int newHighScoreRank() const { return m_newHighScoreRank; }
    QString gameOverReason() const;
    // Milliseconds between ticks; 0 stops the timer so tests drive step().
    int stepInterval() const { return m_stepInterval; }
    void setStepInterval(int interval);

    // Read-only view for the renderer; null on the start screen.
    const Game *engine() const { return m_game.get(); }
    // Scenario hooks for tests only.
    Game *engineForTests() { return m_game.get(); }
    void startGame(Mode mode, Difficulty difficulty, quint32 seed);

    Q_INVOKABLE void newGame(const QString &difficulty);
    Q_INVOKABLE void restart();
    Q_INVOKABLE void setMode(const QString &mode);
    Q_INVOKABLE void toggleMode();
    Q_INVOKABLE void turn(const QString &direction);
    Q_INVOKABLE void pause();
    Q_INVOKABLE void resume();
    Q_INVOKABLE void togglePause();
    Q_INVOKABLE void backToStart();
    Q_INVOKABLE void step();

    Q_INVOKABLE QVariantMap windowGeometry() const;
    Q_INVOKABLE void saveWindowGeometry(int x, int y, int width, int height, bool maximized);

signals:
    void phaseChanged();
    void pausedChanged();
    void readyChanged();
    void scoreChanged();
    void lengthChanged();
    void speedChanged();
    void bestChanged();
    void modeChanged();
    void difficultyChanged();
    void highScoresChanged();
    void stepIntervalChanged();
    // After every tick, for the renderer.
    void frameChanged();
    // A floating label at a field cell: what a dot was worth.
    void scored(const QString &text, int x, int y);
    // The step rate just went up, and the run just ended.
    void spedUp();
    void crashed();

private:
    static bool directionFromId(const QString &id, Direction *direction);
    void handle(const Event &event);
    void finishGame();
    void syncTimer();
    void loadSettings();

    std::unique_ptr<Game> m_game;
    HighScores m_scores;
    QTimer m_timer {this};
    Mode m_mode = Mode::Classic;
    Difficulty m_difficulty = Difficulty::Normal;
    int m_stepInterval = kDefaultStepIntervalMs;
    int m_newHighScoreRank = -1;
};
