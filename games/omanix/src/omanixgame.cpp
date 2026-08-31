#include "omanixgame.h"

#include <QRandomGenerator>
#include <QRect>
#include <QSettings>

#include "difficulties.h"
#include "windowgeometry.h"

namespace {

const auto kDifficultyKey = QStringLiteral("play/difficulty");

const auto kStartId = QStringLiteral("start");
const auto kPlayingId = QStringLiteral("playing");
const auto kLevelCompleteId = QStringLiteral("levelcomplete");
const auto kGameOverId = QStringLiteral("gameover");

// The top ten per difficulty, ranked on the score, with the level reached
// riding along.
OmaGames::ScoreTable scoreTable() {
    QStringList ids;
    for (int i = 0; i < kDifficultyCount; ++i)
        ids << Difficulties::id(Difficulty(i));
    return OmaGames::ScoreTable({QStringLiteral("score"), QStringLiteral("level")}, 10,
                                OmaGames::ScoreTable::sameOrder(ids, OmaGames::ScoreTable::HigherIsBetter));
}

}  // namespace

OmanixGame::OmanixGame(QObject *parent)
    : QObject(parent), m_scores(scoreTable()),
      m_pacer(OmaGames::Pacer::Repeating, [this]() { step(); }, this) {
    m_pacer.setTimerType(Qt::PreciseTimer);
    m_pacer.setInterval(kDefaultStepIntervalMs);
    loadSettings();
}

QString OmanixGame::difficulty() const {
    return Difficulties::id(m_difficulty);
}

void OmanixGame::loadSettings() {
    QSettings settings;
    Difficulties::fromId(settings.value(kDifficultyKey).toString(), &m_difficulty);
    m_scores.load();
}

QString OmanixGame::phase() const {
    if (!m_game)
        return kStartId;
    switch (m_game->phase()) {
    case Phase::LevelComplete:
        return kLevelCompleteId;
    case Phase::GameOver:
        return kGameOverId;
    case Phase::Playing:
        break;
    }
    return kPlayingId;
}

void OmanixGame::setStepInterval(int interval) {
    if (!m_pacer.setInterval(interval))
        return;
    syncTimer();
    emit stepIntervalChanged();
}

void OmanixGame::syncTimer() {
    m_pacer.setRunning(m_game && m_game->phase() == Phase::Playing && !m_game->paused());
}

void OmanixGame::startGame(Difficulty difficulty, quint32 seed) {
    m_difficulty = difficulty;
    QSettings().setValue(kDifficultyKey, Difficulties::id(difficulty));
    m_game = std::make_unique<Game>(difficulty, seed);
    m_newHighScoreRank = -1;
    emit difficultyChanged();
    emit levelChanged();
    emit scoreChanged();
    emit livesChanged();
    emit claimedPercentChanged();
    emit levelSecondsChanged();
    emit phaseChanged();
    emit pausedChanged();
    emit levelIntroChanged();
    syncDerivedState();
    m_lastFrame = m_game ? m_game->frame() : 0;
    emit frameChanged();
    syncTimer();
}

void OmanixGame::newGame(const QString &difficulty) {
    Difficulty level = m_difficulty;
    Difficulties::fromId(difficulty, &level);
    startGame(level, QRandomGenerator::global()->generate());
}

Direction OmanixGame::directionFromId(const QString &id) {
    if (id == QStringLiteral("up"))
        return Direction::Up;
    if (id == QStringLiteral("down"))
        return Direction::Down;
    if (id == QStringLiteral("left"))
        return Direction::Left;
    if (id == QStringLiteral("right"))
        return Direction::Right;
    return Direction::None;
}

void OmanixGame::setDirection(const QString &direction) {
    if (m_game)
        m_game->setDirection(directionFromId(direction));
}

void OmanixGame::releaseDirection(const QString &direction) {
    if (m_game)
        m_game->releaseDirection(directionFromId(direction));
}

void OmanixGame::pause() {
    if (!m_game || m_game->paused() || m_game->phase() != Phase::Playing)
        return;
    m_game->setPaused(true);
    syncTimer();
    emit pausedChanged();
}

void OmanixGame::resume() {
    if (!m_game || !m_game->paused())
        return;
    m_game->setPaused(false);
    syncTimer();
    emit pausedChanged();
}

void OmanixGame::togglePause() {
    if (paused())
        resume();
    else
        pause();
}

void OmanixGame::restartLevel() {
    if (!m_game || m_game->phase() != Phase::Playing)
        return;
    m_game->setPaused(false);
    m_game->restartLevel();
    emit pausedChanged();
    emit claimedPercentChanged();
    emit levelSecondsChanged();
    emit levelIntroChanged();
    syncDerivedState();
    m_lastFrame = m_game ? m_game->frame() : 0;
    emit frameChanged();
    syncTimer();
}

void OmanixGame::nextLevel() {
    if (!m_game || m_game->phase() != Phase::LevelComplete)
        return;
    m_game->nextLevel();
    emit levelChanged();
    emit claimedPercentChanged();
    emit levelSecondsChanged();
    emit phaseChanged();
    emit levelIntroChanged();
    syncDerivedState();
    m_lastFrame = m_game ? m_game->frame() : 0;
    emit frameChanged();
    syncTimer();
}

void OmanixGame::backToStart() {
    if (!m_game)
        return;
    m_game.reset();
    m_pacer.stop();
    emit phaseChanged();
    emit pausedChanged();
    emit levelIntroChanged();
    syncDerivedState();
    m_lastFrame = 0;
    emit frameChanged();
}

void OmanixGame::step() {
    if (!m_game || m_game->phase() != Phase::Playing)
        return;
    const int score = m_game->score();
    const int lives = m_game->lives();
    const int seconds = levelSeconds();
    const bool intro = m_game->inLevelIntro();
    const std::vector<Event> events = m_game->tick();
    for (const Event &event : events)
        handle(event);
    if (m_game->score() != score)
        emit scoreChanged();
    if (m_game->lives() != lives)
        emit livesChanged();
    if (levelSeconds() != seconds)
        emit levelSecondsChanged();
    if (m_game->inLevelIntro() != intro)
        emit levelIntroChanged();
    syncDerivedState();
    // A tick between movement periods changes nothing on screen; skipping the
    // signal is what keeps the renderer from rasterising 60 identical frames
    // a second.
    if (m_game->frame() != m_lastFrame) {
        m_lastFrame = m_game->frame();
        emit frameChanged();
    }
}

// State the engine derives on demand, sampled once per tick so QML only
// hears about changes.
void OmanixGame::syncDerivedState() {
    const bool threatened = m_game && m_game->trailThreatened();
    if (threatened == m_trailThreatened)
        return;
    m_trailThreatened = threatened;
    emit trailThreatenedChanged();
}

void OmanixGame::handle(const Event &event) {
    switch (event.type) {
    case Event::Claimed: {
        emit cellsClaimed(QVector<int>(event.cells.begin(), event.cells.end()), event.at.x(), event.at.y());
        emit claimedPercentChanged();
        if (event.multiplier > 1)
            emit bonusEarned(tr("Big cut ×%1").arg(event.multiplier), event.at.x(), event.at.y());
        if (event.closeCall)
            emit bonusEarned(tr("Close call +%1").arg(Game::kCloseCallBonus), event.at.x(), event.at.y());
        break;
    }
    case Event::LifeLost:
        emit trailLost(QVector<int>(event.cells.begin(), event.cells.end()));
        emit lifeLost();
        break;
    case Event::ExtraLife:
        emit extraLife();
        break;
    case Event::LevelComplete:
        emit phaseChanged();
        syncTimer();
        break;
    case Event::GameOver:
        finishGame();
        break;
    case Event::TrailStarted:
    case Event::Paused:
    case Event::Resumed:
        break;
    }
}

void OmanixGame::finishGame() {
    m_newHighScoreRank = m_scores.insert(
        Difficulties::id(m_difficulty),
        {m_game->score(), QDate::currentDate(), {{QStringLiteral("level"), m_game->level()}}});
    if (m_newHighScoreRank >= 0) {
        m_scores.save();
        emit highScoresChanged();
    }
    emit phaseChanged();
    syncTimer();
}

QVariantMap OmanixGame::windowGeometry() const {
    return OmaGames::WindowGeometry::toVariantMap();
}

void OmanixGame::saveWindowGeometry(int x, int y, int width, int height, bool maximized) {
    OmaGames::WindowGeometry::save(QRect(x, y, width, height), maximized);
}
