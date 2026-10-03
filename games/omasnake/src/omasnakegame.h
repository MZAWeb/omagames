#pragma once

#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <functional>
#include <memory>
#include <optional>

#include "game.h"
#include "pacer.h"
#include "replaypace.h"
#include "replayplayer.h"
#include "scoretable.h"

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
    // Watching a recorded game (--replay) rather than playing one.
    Q_PROPERTY(bool replaying READ replaying NOTIFY replayChanged)
    Q_PROPERTY(QString replayAgent READ replayAgent NOTIFY replayChanged)
    // 1 to 4, slowest first, as keys 1 to 4 pick it, and how fast that is
    // against the clock the game was played on.
    Q_PROPERTY(int replaySpeed READ replaySpeed NOTIFY replayChanged)
    Q_PROPERTY(QString replaySpeedLabel READ replaySpeedLabel NOTIFY replayChanged)
    // Every turn made: what is on screen is where the recording stops.
    Q_PROPERTY(bool replayEnded READ replayEnded NOTIFY replayChanged)

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
    int best() const;
    static int fieldWidth() { return Game::kWidth; }
    static int fieldHeight() { return Game::kHeight; }
    // "classic" | "wrap".
    QString mode() const;
    QString modeLabel() const;
    // {id, label, description} for both walls rules, and for the three speeds.
    static QVariantList modes();
    // "slow" | "normal" | "fast".
    QString difficulty() const;
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
    int stepInterval() const { return m_pacer.interval(); }
    void setStepInterval(int interval);

    bool replaying() const { return m_replay.has_value(); }
    QString replayAgent() const { return m_replay ? m_replay->agent() : QString(); }
    int replaySpeed() const { return m_pace.speed(); }
    QString replaySpeedLabel() const { return m_pace.label(); }
    bool replayEnded() const { return m_replay && m_replay->done(); }
    // Starts watching the replay in `path`; false and the reason when it is
    // not one this game can play.
    bool loadReplay(const QString &path, QString *error);

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
    Q_INVOKABLE void setReplaySpeed(int speed);
    // Plays the replay on until the snake has moved once more, paused or not.
    Q_INVOKABLE void replayNextMove();
    Q_INVOKABLE void restartReplay();

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
    void replayChanged();
    // After every tick, for the renderer.
    void frameChanged();
    // A floating label at a field cell: what a dot was worth.
    void scored(const QString &text, int x, int y);
    // The step rate just went up, and the run just ended.
    void spedUp();
    void crashed();

private:
    static bool directionFromId(const QString &id, Direction *direction);
    // The walls and speed of the game on screen, which a replay's need not
    // be the player's last choice.
    Mode shownMode() const { return m_game ? m_game->mode() : m_mode; }
    Difficulty shownDifficulty() const { return m_game ? m_game->difficulty() : m_difficulty; }
    // Puts `game` on screen, fresh, and tells QML everything changed.
    void show(std::unique_ptr<Game> game);
    // Makes `calls` on the game and tells QML what they changed.
    void advance(const std::function<std::vector<Event>()> &calls);
    void endReplay();
    void replayFrame();
    // Plays `calls` from the replay and, if that was the last of it, says so.
    void playReplay(const std::function<std::vector<Event>()> &calls);
    void handle(const Event &event);
    void finishGame();
    void syncTimer();
    void loadSettings();

    std::unique_ptr<Game> m_game;
    OmaGames::ScoreTable m_scores;
    OmaGames::Pacer m_pacer;
    Mode m_mode = Mode::Classic;
    Difficulty m_difficulty = Difficulty::Normal;
    int m_newHighScoreRank = -1;
    std::optional<ReplayPlayer> m_replay;
    OmaGames::ReplayPace m_pace;
};
