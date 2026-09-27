#include "omadropgame.h"

#include <QRandomGenerator>
#include <QRect>
#include <cmath>

#include "windowgeometry.h"

namespace {

const auto kStartId = QStringLiteral("start");
const auto kPlayingId = QStringLiteral("playing");
const auto kGameOverId = QStringLiteral("gameover");
const auto kStandardId = QStringLiteral("standard");

OmaGames::ScoreTable scoreTable() {
    return OmaGames::ScoreTable(
        {QStringLiteral("score"), QStringLiteral("shots")}, 10,
        OmaGames::ScoreTable::sameOrder({kStandardId}, OmaGames::ScoreTable::HigherIsBetter));
}

}  // namespace

OmadropGame::OmadropGame(QObject *parent)
    : QObject(parent), m_scores(scoreTable()),
      m_pacer(OmaGames::Pacer::Repeating, [this]() { step(); }, this) {
    m_pacer.setTimerType(Qt::PreciseTimer);
    m_pacer.setInterval(kDefaultStepIntervalMs);
    m_scores.load();
}

QString OmadropGame::phase() const {
    if (!m_engine)
        return kStartId;
    return m_engine->phase() == DropPhase::GameOver ? kGameOverId : kPlayingId;
}

int OmadropGame::best() const {
    return m_scores.best(kStandardId);
}

QVariantList OmadropGame::highScores() const {
    QVariantList list;
    for (const QVariant &entry : m_scores.toVariantList(kStandardId)) {
        QVariantMap row = entry.toMap();
        row.insert(QStringLiteral("category"), kStandardId);
        list.append(row);
    }
    return list;
}

void OmadropGame::setStepInterval(int interval) {
    if (!m_pacer.setInterval(interval))
        return;
    syncTimer();
    emit stepIntervalChanged();
}

void OmadropGame::startGame(quint32 seed) {
    m_engine = std::make_unique<DropEngine>(seed);
    m_newHighScoreRank = -1;
    emit phaseChanged();
    emit pausedChanged();
    emit readyChanged();
    emit scoreChanged();
    emit shotsChanged();
    emit pegCountChanged();
    emit frameChanged();
    syncTimer();
}

void OmadropGame::newGame() {
    startGame(QRandomGenerator::global()->generate());
}

void OmadropGame::restart() {
    if (m_engine)
        newGame();
}

void OmadropGame::aimAt(double x, double y) {
    if (!m_engine)
        return;
    const double dx = x * DropEngine::kWidth - DropEngine::kLauncherX;
    const double dy = y * DropEngine::kHeight - DropEngine::kLauncherY;
    if (std::hypot(dx, dy) < DropEngine::kBallRadius)
        return;
    m_engine->setAimAngle(std::atan2(dx, dy));
    emit frameChanged();
}

void OmadropGame::nudgeAim(int direction) {
    if (!m_engine)
        return;
    m_engine->nudgeAim(direction);
    emit frameChanged();
}

void OmadropGame::launch() {
    if (!m_engine || !m_engine->launch())
        return;
    emit readyChanged();
    emit shotsChanged();
    emit frameChanged();
}

void OmadropGame::pause() {
    if (!m_engine || m_engine->paused() || m_engine->phase() != DropPhase::Playing)
        return;
    m_engine->setPaused(true);
    syncTimer();
    emit pausedChanged();
}

void OmadropGame::resume() {
    if (!m_engine || !m_engine->paused())
        return;
    m_engine->setPaused(false);
    syncTimer();
    emit pausedChanged();
}

void OmadropGame::togglePause() {
    paused() ? resume() : pause();
}

void OmadropGame::backToStart() {
    if (!m_engine)
        return;
    m_engine.reset();
    m_pacer.stop();
    emit phaseChanged();
    emit pausedChanged();
    emit readyChanged();
    emit scoreChanged();
    emit shotsChanged();
    emit pegCountChanged();
    emit frameChanged();
}

void OmadropGame::step() {
    if (!m_engine || m_engine->phase() != DropPhase::Playing)
        return;
    const int oldScore = score();
    const int oldPegs = pegCount();
    const bool wasReady = ready();
    for (const DropEvent &event : m_engine->tick()) {
        if (event.type == DropEvent::PegHit)
            emit scored(QStringLiteral("+1"), event.at.x(), event.at.y());
        else if (event.type == DropEvent::GameOver) {
            emit crashed();
            finishGame();
        }
    }
    if (score() != oldScore)
        emit scoreChanged();
    if (pegCount() != oldPegs)
        emit pegCountChanged();
    if (ready() != wasReady)
        emit readyChanged();
    emit frameChanged();
}

void OmadropGame::finishGame() {
    m_newHighScoreRank = m_scores.insert(
        kStandardId, {score(), QDate::currentDate(), {{QStringLiteral("shots"), shots()}}});
    if (m_newHighScoreRank >= 0) {
        m_scores.save();
        emit highScoresChanged();
        emit bestChanged();
    }
    emit phaseChanged();
    syncTimer();
}

void OmadropGame::syncTimer() {
    m_pacer.setRunning(m_engine && m_engine->phase() == DropPhase::Playing && !m_engine->paused());
}

QVariantMap OmadropGame::windowGeometry() const {
    return OmaGames::WindowGeometry::toVariantMap();
}

void OmadropGame::saveWindowGeometry(int x, int y, int width, int height, bool maximized) {
    OmaGames::WindowGeometry::save(QRect(x, y, width, height), maximized);
}
