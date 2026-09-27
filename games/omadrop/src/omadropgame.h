#pragma once

#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <memory>

#include "dropengine.h"
#include "pacer.h"
#include "scoretable.h"

class OmadropGame : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString phase READ phase NOTIFY phaseChanged)
    Q_PROPERTY(bool paused READ paused NOTIFY pausedChanged)
    Q_PROPERTY(bool ready READ ready NOTIFY readyChanged)
    Q_PROPERTY(int score READ score NOTIFY scoreChanged)
    Q_PROPERTY(int shots READ shots NOTIFY shotsChanged)
    Q_PROPERTY(int pegCount READ pegCount NOTIFY pegCountChanged)
    Q_PROPERTY(int best READ best NOTIFY bestChanged)
    Q_PROPERTY(double aimAngle READ aimAngle NOTIFY frameChanged)
    Q_PROPERTY(QVariantList highScores READ highScores NOTIFY highScoresChanged)
    Q_PROPERTY(int newHighScoreRank READ newHighScoreRank NOTIFY phaseChanged)
    Q_PROPERTY(int stepInterval READ stepInterval WRITE setStepInterval NOTIFY stepIntervalChanged)

public:
    static constexpr int kDefaultStepIntervalMs = 1000 / 60;

    explicit OmadropGame(QObject *parent = nullptr);

    QString phase() const;
    bool paused() const { return m_engine && m_engine->paused(); }
    bool ready() const { return m_engine && m_engine->ready(); }
    int score() const { return m_engine ? m_engine->score() : 0; }
    int shots() const { return m_engine ? m_engine->shots() : 0; }
    int pegCount() const { return m_engine ? m_engine->pegs().size() : 0; }
    int best() const;
    double aimAngle() const { return m_engine ? m_engine->aimAngle() : 0.0; }
    QVariantList highScores() const;
    int newHighScoreRank() const { return m_newHighScoreRank; }
    int stepInterval() const { return m_pacer.interval(); }
    void setStepInterval(int interval);

    const DropEngine *engine() const { return m_engine.get(); }
    DropEngine *engineForTests() { return m_engine.get(); }
    void startGame(quint32 seed);

    Q_INVOKABLE void newGame();
    Q_INVOKABLE void restart();
    Q_INVOKABLE void aimAt(double x, double y);
    Q_INVOKABLE void nudgeAim(int direction);
    Q_INVOKABLE void launch();
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
    void shotsChanged();
    void pegCountChanged();
    void bestChanged();
    void highScoresChanged();
    void stepIntervalChanged();
    void frameChanged();
    void scored(const QString &text, double x, double y);
    void crashed();

private:
    void syncTimer();
    void finishGame();

    std::unique_ptr<DropEngine> m_engine;
    OmaGames::ScoreTable m_scores;
    OmaGames::Pacer m_pacer;
    int m_newHighScoreRank = -1;
};
