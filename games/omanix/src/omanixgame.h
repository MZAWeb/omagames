#pragma once

#include <QElapsedTimer>
#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>
#include <memory>

#include "game.h"
#include "pacer.h"
#include "scoretable.h"

// The only bridge between the engine and QML: state as properties, actions as
// invokables, persistence and pacing. Screen state and difficulty cross to
// QML as lowercase id strings, like the other games.
class OmanixGame : public QObject {
    Q_OBJECT
    // "start" | "playing" | "levelcomplete" | "gameover"
    Q_PROPERTY(QString phase READ phase NOTIFY phaseChanged)
    Q_PROPERTY(bool paused READ paused NOTIFY pausedChanged)
    Q_PROPERTY(bool levelIntro READ levelIntro NOTIFY levelIntroChanged)
    Q_PROPERTY(bool trailThreatened READ trailThreatened NOTIFY trailThreatenedChanged)
    Q_PROPERTY(int ballCount READ ballCount NOTIFY levelChanged)
    Q_PROPERTY(int chaserCount READ chaserCount NOTIFY levelChanged)
    Q_PROPERTY(int level READ level NOTIFY levelChanged)
    Q_PROPERTY(int score READ score NOTIFY scoreChanged)
    Q_PROPERTY(int lives READ lives NOTIFY livesChanged)
    Q_PROPERTY(double claimedPercent READ claimedPercent NOTIFY claimedPercentChanged)
    Q_PROPERTY(double goalPercent READ goalPercent CONSTANT)
    Q_PROPERTY(int levelSeconds READ levelSeconds NOTIFY levelSecondsChanged)
    Q_PROPERTY(int fieldWidth READ fieldWidth CONSTANT)
    Q_PROPERTY(int fieldHeight READ fieldHeight CONSTANT)
    Q_PROPERTY(QString difficulty READ difficulty NOTIFY difficultyChanged)
    Q_PROPERTY(QString difficultyLabel READ difficultyLabel NOTIFY difficultyChanged)
    Q_PROPERTY(QVariantList difficulties READ difficulties CONSTANT)
    Q_PROPERTY(QVariantList highScores READ highScores NOTIFY highScoresChanged)
    Q_PROPERTY(QVariantMap bests READ bests NOTIFY highScoresChanged)
    Q_PROPERTY(int newHighScoreRank READ newHighScoreRank NOTIFY phaseChanged)
    Q_PROPERTY(QVariantMap levelStats READ levelStats NOTIFY phaseChanged)
    Q_PROPERTY(int stepInterval READ stepInterval WRITE setStepInterval NOTIFY stepIntervalChanged)

public:
    // One simulation tick per timer shot: 60 ticks a second.
    static constexpr int kDefaultStepIntervalMs = 16;

    explicit OmanixGame(QObject *parent = nullptr);

    QString phase() const;
    bool paused() const { return m_game && m_game->paused(); }
    // The banner moment at the start of a level, while nothing moves.
    bool levelIntro() const { return m_game && m_game->inLevelIntro(); }
    // A ball is close enough to the trail to make the close-call bonus.
    bool trailThreatened() const { return m_trailThreatened; }
    int ballCount() const { return m_game ? m_game->params().balls : 0; }
    int chaserCount() const { return m_game ? m_game->params().chasers : 0; }
    int level() const { return m_game ? m_game->level() : 0; }
    int score() const { return m_game ? m_game->score() : 0; }
    int lives() const { return m_game ? m_game->lives() : 0; }
    double claimedPercent() const { return m_game ? m_game->claimedPercent() : 0.0; }
    static double goalPercent() { return Game::kGoalPercent; }
    int levelSeconds() const { return m_game ? m_game->levelTicks() / Level::kTicksPerSecond : 0; }
    static int fieldWidth() { return Field::kDefaultWidth; }
    static int fieldHeight() { return Field::kDefaultHeight; }
    // "easy" | "normal" | "hard": the one being played, or the last one chosen.
    QString difficulty() const;
    QString difficultyLabel() const;
    // {id, label, description} for the start screen, in play order.
    static QVariantList difficulties();
    // {difficulty, label, score, level, date} for every table, best first.
    QVariantList highScores() const;
    // difficulty id → best score.
    QVariantMap bests() const;
    // Where the finished game landed in its table (0 = top), or -1.
    int newHighScoreRank() const { return m_newHighScoreRank; }
    // {percent, bonus, seconds} of the level just completed.
    QVariantMap levelStats() const;
    // Milliseconds between ticks; 0 stops the timer so tests drive step().
    int stepInterval() const { return m_pacer.interval(); }
    void setStepInterval(int interval);

    // Read-only view for the renderer; null on the start screen.
    const Game *engine() const { return m_game.get(); }
    // Scenario hooks for tests only.
    Game *engineForTests() { return m_game.get(); }
    void startGame(Difficulty difficulty, quint32 seed);

    Q_INVOKABLE void newGame(const QString &difficulty);
    Q_INVOKABLE void setDirection(const QString &direction);
    Q_INVOKABLE void releaseDirection(const QString &direction);
    Q_INVOKABLE void pause();
    Q_INVOKABLE void resume();
    Q_INVOKABLE void togglePause();
    Q_INVOKABLE void restartLevel();
    Q_INVOKABLE void nextLevel();
    Q_INVOKABLE void backToStart();
    Q_INVOKABLE void step();

    Q_INVOKABLE QVariantMap windowGeometry() const;
    Q_INVOKABLE void saveWindowGeometry(int x, int y, int width, int height, bool maximized);

signals:
    void phaseChanged();
    void pausedChanged();
    void levelIntroChanged();
    void trailThreatenedChanged();
    void levelChanged();
    void scoreChanged();
    void livesChanged();
    void claimedPercentChanged();
    void levelSecondsChanged();
    void difficultyChanged();
    void highScoresChanged();
    void stepIntervalChanged();
    // After every tick, for the renderer.
    void frameChanged();
    // Animation cues. Cells are field indices; `x`/`y` are field cells.
    void cellsClaimed(const QVector<int> &cells, int x, int y);
    void trailLost(const QVector<int> &cells);
    void bonusEarned(const QString &text, int x, int y);
    void lifeLost();
    void extraLife();

private:
    static Direction directionFromId(const QString &id);
    void handle(const Event &event);
    void finishGame();
    void syncTimer();
    void syncDerivedState();
    void loadSettings();

    std::unique_ptr<Game> m_game;
    OmaGames::ScoreTable m_scores;
    OmaGames::Pacer m_pacer;
    Difficulty m_difficulty = Difficulty::Normal;
    int m_newHighScoreRank = -1;
    bool m_trailThreatened = false;
};
