#pragma once

#include <QPointF>
#include <QRandomGenerator>
#include <QSet>
#include <QVector>

enum class DropPhase { Playing, GameOver };
enum class PegShape { Circle, Square };

struct DropPeg {
    int id = 0;
    QPointF position;
    int hits = 1;
    PegShape shape = PegShape::Circle;
    double angle = 0.0;
    double targetY = 0.0;
};

struct DropBall {
    QPointF position;
    QPointF velocity;
    bool active = false;
};

struct DropEvent {
    enum Type { PegHit, PegRemoved, ShotFinished, GameOver };
    Type type;
    int pegId = -1;
    QPointF at;
};

class DropEngine {
public:
    static constexpr double kWidth = 0.62;
    static constexpr double kHeight = 1.0;
    static constexpr double kLauncherX = kWidth / 2.0;
    static constexpr double kLauncherY = 0.035;
    static constexpr double kDangerY = 0.0;
    static constexpr double kBallRadius = 0.021;
    static constexpr double kPegRadius = 0.039;
    static constexpr double kStepSeconds = 1.0 / 60.0;

    explicit DropEngine(quint32 seed);

    DropPhase phase() const { return m_phase; }
    bool paused() const { return m_paused; }
    void setPaused(bool paused) { m_paused = paused; }
    bool ready() const { return !m_ball.active && m_phase == DropPhase::Playing; }
    int score() const { return m_score; }
    int shots() const { return m_shots; }
    double aimAngle() const { return m_aimAngle; }
    const DropBall &ball() const { return m_ball; }
    const QVector<QPointF> &trail() const { return m_trail; }
    const QVector<DropPeg> &pegs() const { return m_pegs; }

    bool launch();
    void setAimAngle(double angle);
    void nudgeAim(int direction);
    QVector<QPointF> guide() const;
    QVector<DropEvent> tick(double seconds = kStepSeconds);

    // Deterministic scenario hooks used by the headless tests.
    void clearPegsForTests() { m_pegs.clear(); }
    int addPegForTests(QPointF position, int hits, PegShape shape = PegShape::Circle);
    void setBallForTests(QPointF position, QPointF velocity);
    void setAimAngleForTests(double angle) { m_aimAngle = angle; }

private:
    void updateRise(double seconds);
    void updateBall(double seconds, QVector<DropEvent> *events);
    void hitPegs(QVector<DropEvent> *events);
    void finishShot(QVector<DropEvent> *events);
    void spawnPegs();
    void spawnPeg(int index, int count);
    void checkGameOver(QVector<DropEvent> *events);
    int randomHits();

    QRandomGenerator m_random;
    DropPhase m_phase = DropPhase::Playing;
    bool m_paused = false;
    int m_score = 0;
    int m_shots = 0;
    int m_nextPegId = 1;
    double m_aimAngle = 0.0;
    DropBall m_ball;
    QVector<QPointF> m_trail;
    QVector<DropPeg> m_pegs;
    QSet<int> m_ballContacts;
    bool m_rising = false;
};
