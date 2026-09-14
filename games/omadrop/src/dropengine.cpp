#include "dropengine.h"

#include <algorithm>
#include <cmath>

namespace {

constexpr double kAimLimit = 1.08;
constexpr double kAimStep = 0.07;
constexpr double kLaunchSpeed = 0.76;
constexpr double kGravity = 0.56;
constexpr double kWallRestitution = 0.48;
constexpr double kPegRestitution = 0.32;
constexpr double kBallDamping = 0.992;
constexpr double kRisePerShot = 0.073;
constexpr double kRiseSpeed = 0.34;
constexpr double kMinDistance = 0.000001;
constexpr int kMaxTrail = 15;

double length(QPointF value) {
    return std::hypot(value.x(), value.y());
}

QPointF normalised(QPointF value) {
    const double size = length(value);
    return size > kMinDistance ? value / size : QPointF(1.0, 0.0);
}

double dot(QPointF a, QPointF b) {
    return a.x() * b.x() + a.y() * b.y();
}

}  // namespace

DropEngine::DropEngine(quint32 seed) : m_random(seed) {
    addPegForTests({kWidth * 0.25, 0.93}, 1);
    addPegForTests({kWidth * 0.50, 0.93}, 1);
    addPegForTests({kWidth * 0.75, 0.93}, 1);
}

bool DropEngine::launch() {
    if (!ready() || m_paused)
        return false;
    m_ball.active = true;
    m_ball.position = {kLauncherX, kLauncherY + kBallRadius};
    m_ball.velocity = {std::sin(m_aimAngle) * kLaunchSpeed,
                       std::cos(m_aimAngle) * kLaunchSpeed};
    m_trail = {m_ball.position};
    m_ballContacts.clear();
    ++m_shots;
    return true;
}

void DropEngine::setAimAngle(double angle) {
    if (ready() && !m_paused)
        m_aimAngle = std::clamp(angle, -kAimLimit, kAimLimit);
}

void DropEngine::nudgeAim(int direction) {
    if (direction != 0)
        setAimAngle(m_aimAngle + (direction < 0 ? -kAimStep : kAimStep));
}

QVector<DropEvent> DropEngine::tick(double seconds) {
    QVector<DropEvent> events;
    if (m_phase != DropPhase::Playing || m_paused || seconds <= 0.0)
        return events;
    seconds = std::min(seconds, 0.05);
    updateRise(seconds);
    if (m_ball.active)
        updateBall(seconds, &events);
    checkGameOver(&events);
    return events;
}

void DropEngine::updateRise(double seconds) {
    if (!m_rising)
        return;
    bool complete = true;
    for (DropPeg &peg : m_pegs) {
        if (peg.position.y() <= peg.targetY)
            continue;
        peg.position.setY(std::max(peg.targetY, peg.position.y() - kRiseSpeed * seconds));
        complete = complete && peg.position.y() <= peg.targetY;
    }
    m_rising = !complete;
}

void DropEngine::updateBall(double seconds, QVector<DropEvent> *events) {
    m_ball.velocity.ry() += kGravity * seconds;
    m_ball.position += m_ball.velocity * seconds;
    m_ball.velocity *= kBallDamping;

    if (m_ball.position.x() < kBallRadius) {
        m_ball.position.setX(kBallRadius);
        m_ball.velocity.setX(std::abs(m_ball.velocity.x()) * kWallRestitution);
    } else if (m_ball.position.x() > kWidth - kBallRadius) {
        m_ball.position.setX(kWidth - kBallRadius);
        m_ball.velocity.setX(-std::abs(m_ball.velocity.x()) * kWallRestitution);
    }
    if (m_ball.position.y() < kBallRadius) {
        m_ball.position.setY(kBallRadius);
        m_ball.velocity.setY(std::abs(m_ball.velocity.y()) * kWallRestitution);
    }

    hitPegs(events);
    m_trail.prepend(m_ball.position);
    while (m_trail.size() > kMaxTrail)
        m_trail.removeLast();
    if (m_ball.position.y() - kBallRadius > kHeight)
        finishShot(events);
}

void DropEngine::hitPegs(QVector<DropEvent> *events) {
    QSet<int> contacts;
    QVector<int> removed;
    for (DropPeg &peg : m_pegs) {
        const QPointF delta = m_ball.position - peg.position;
        const double minimum = kBallRadius + kPegRadius;
        const double distance = length(delta);
        if (distance >= minimum)
            continue;
        contacts.insert(peg.id);
        const QPointF normal = normalised(delta);
        m_ball.position = peg.position + normal * minimum;
        const double toward = dot(m_ball.velocity, normal);
        if (toward < 0.0)
            m_ball.velocity -= normal * ((1.0 + kPegRestitution) * toward);
        if (m_ballContacts.contains(peg.id))
            continue;
        --peg.hits;
        ++m_score;
        events->append({DropEvent::PegHit, peg.id, peg.position});
        if (peg.hits == 0) {
            removed.append(peg.id);
            events->append({DropEvent::PegRemoved, peg.id, peg.position});
        }
    }
    m_ballContacts = contacts;
    for (int id : removed)
        m_pegs.removeIf([id](const DropPeg &peg) { return peg.id == id; });
}

void DropEngine::finishShot(QVector<DropEvent> *events) {
    m_ball = {};
    m_trail.clear();
    m_ballContacts.clear();
    for (DropPeg &peg : m_pegs)
        peg.targetY -= kRisePerShot;
    spawnPegs();
    m_rising = true;
    events->append({DropEvent::ShotFinished, -1, {}});
}

int DropEngine::randomHits() {
    const int ceiling = std::clamp(1 + m_score / 24, 1, 7);
    if (ceiling == 1)
        return 1;
    return 1 + int(std::min(m_random.bounded(ceiling), m_random.bounded(ceiling)));
}

void DropEngine::spawnPegs() {
    const int roll = m_random.bounded(100);
    const int count = roll < 55 ? 1 : (roll < 85 ? 2 : 3);
    for (int i = 0; i < count; ++i)
        spawnPeg(i, count);
}

void DropEngine::spawnPeg(int index, int count) {
    const double slot = kWidth * double(index + 1) / double(count + 1);
    const double x = std::clamp(slot + (m_random.generateDouble() - 0.5) * 0.15,
                                0.07, kWidth - 0.07);
    DropPeg peg;
    peg.id = m_nextPegId++;
    peg.position = {x, kHeight + kPegRadius};
    peg.hits = randomHits();
    peg.shape = m_random.bounded(4) == 0 ? PegShape::Square : PegShape::Circle;
    peg.angle = m_random.generateDouble() * 6.283185307179586;
    peg.targetY = kHeight - kPegRadius;
    m_pegs.append(peg);
}

void DropEngine::checkGameOver(QVector<DropEvent> *events) {
    for (const DropPeg &peg : m_pegs) {
        if (peg.position.y() - kPegRadius <= kDangerY) {
            m_phase = DropPhase::GameOver;
            m_ball.active = false;
            events->append({DropEvent::GameOver, peg.id, peg.position});
            return;
        }
    }
}

int DropEngine::addPegForTests(QPointF position, int hits, PegShape shape) {
    DropPeg peg;
    peg.id = m_nextPegId++;
    peg.position = position;
    peg.hits = std::clamp(hits, 1, 7);
    peg.shape = shape;
    peg.angle = m_random.generateDouble() * 6.283185307179586;
    peg.targetY = position.y();
    m_pegs.append(peg);
    return peg.id;
}

void DropEngine::setBallForTests(QPointF position, QPointF velocity) {
    m_ball = {position, velocity, true};
    m_ballContacts.clear();
    m_trail = {position};
}
