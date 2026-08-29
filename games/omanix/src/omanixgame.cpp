#include "omanixgame.h"

#include <QRandomGenerator>
#include <QRect>
#include <QSettings>

namespace {

const auto kDifficultyKey = QStringLiteral("play/difficulty");
const auto kGeometryKey = QStringLiteral("window/geometry");
const auto kMaximizedKey = QStringLiteral("window/maximized");

const auto kStartId = QStringLiteral("start");
const auto kPlayingId = QStringLiteral("playing");
const auto kLevelCompleteId = QStringLiteral("levelcomplete");
const auto kGameOverId = QStringLiteral("gameover");

struct DifficultyInfo {
    Difficulty difficulty;
    QString label;
    QString description;
};

QVector<DifficultyInfo> difficultyInfos() {
    return {
        {Difficulty::Easy, OmanixGame::tr("Easy"), OmanixGame::tr("Two slow balls, a gentle ramp.")},
        {Difficulty::Normal, OmanixGame::tr("Normal"), OmanixGame::tr("Three balls, the classic pace.")},
        {Difficulty::Hard, OmanixGame::tr("Hard"), OmanixGame::tr("Four fast balls, chasers pile up quickly.")},
    };
}

}  // namespace

OmanixGame::OmanixGame(QObject *parent) : QObject(parent) {
    m_timer.setTimerType(Qt::PreciseTimer);
    connect(&m_timer, &QTimer::timeout, this, &OmanixGame::step);
    loadSettings();
}

void OmanixGame::loadSettings() {
    QSettings settings;
    HighScores::difficultyFromId(settings.value(kDifficultyKey).toString(), &m_difficulty);
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

QString OmanixGame::difficultyLabel() const {
    for (const DifficultyInfo &info : difficultyInfos()) {
        if (info.difficulty == m_difficulty)
            return info.label;
    }
    return {};
}

QVariantList OmanixGame::difficulties() {
    QVariantList list;
    for (const DifficultyInfo &info : difficultyInfos()) {
        list.append(QVariantMap {
            {QStringLiteral("id"), HighScores::idFor(info.difficulty)},
            {QStringLiteral("label"), info.label},
            {QStringLiteral("description"), info.description},
        });
    }
    return list;
}

QVariantList OmanixGame::highScores() const {
    QVariantList list;
    for (const DifficultyInfo &info : difficultyInfos()) {
        for (const ScoreEntry &e : m_scores.entries(info.difficulty)) {
            list.append(QVariantMap {
                {QStringLiteral("difficulty"), HighScores::idFor(info.difficulty)},
                {QStringLiteral("label"), info.label},
                {QStringLiteral("score"), e.score},
                {QStringLiteral("level"), e.level},
                {QStringLiteral("date"), e.date.toString(Qt::ISODate)},
            });
        }
    }
    return list;
}

QVariantMap OmanixGame::bests() const {
    QVariantMap map;
    for (int i = 0; i < kDifficultyCount; ++i)
        map.insert(HighScores::idFor(Difficulty(i)), m_scores.best(Difficulty(i)));
    return map;
}

QVariantMap OmanixGame::levelStats() const {
    if (!m_game)
        return {};
    const LevelStats &stats = m_game->lastLevel();
    return {
        {QStringLiteral("percent"), stats.percent},
        {QStringLiteral("bonus"), stats.livesBonus},
        {QStringLiteral("seconds"), stats.ticks / Level::kTicksPerSecond},
    };
}

void OmanixGame::setStepInterval(int interval) {
    if (m_stepInterval == interval)
        return;
    m_stepInterval = interval;
    syncTimer();
    emit stepIntervalChanged();
}

void OmanixGame::syncTimer() {
    const bool running = m_game && m_game->phase() == Phase::Playing && !m_game->paused() && m_stepInterval > 0;
    if (running) {
        m_timer.start(m_stepInterval);
    } else {
        m_timer.stop();
    }
}

void OmanixGame::startGame(Difficulty difficulty, quint32 seed) {
    m_difficulty = difficulty;
    QSettings().setValue(kDifficultyKey, HighScores::idFor(difficulty));
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
    emit frameChanged();
    syncTimer();
}

void OmanixGame::newGame(const QString &difficulty) {
    Difficulty level = m_difficulty;
    HighScores::difficultyFromId(difficulty, &level);
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
    emit frameChanged();
    syncTimer();
}

void OmanixGame::backToStart() {
    if (!m_game)
        return;
    m_game.reset();
    m_timer.stop();
    emit phaseChanged();
    emit pausedChanged();
    emit levelIntroChanged();
    syncDerivedState();
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
    emit frameChanged();
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
    m_newHighScoreRank = m_scores.insert(m_difficulty, {m_game->score(), m_game->level(), QDate::currentDate()});
    if (m_newHighScoreRank >= 0) {
        m_scores.save();
        emit highScoresChanged();
    }
    emit phaseChanged();
    syncTimer();
}

QVariantMap OmanixGame::windowGeometry() const {
    QSettings settings;
    const QRect rect = settings.value(kGeometryKey).toRect();
    return {
        {QStringLiteral("valid"), rect.isValid()},
        {QStringLiteral("x"), rect.x()},
        {QStringLiteral("y"), rect.y()},
        {QStringLiteral("width"), rect.width()},
        {QStringLiteral("height"), rect.height()},
        {QStringLiteral("maximized"), settings.value(kMaximizedKey, false).toBool()},
    };
}

void OmanixGame::saveWindowGeometry(int x, int y, int width, int height, bool maximized) {
    QSettings settings;
    settings.setValue(kGeometryKey, QRect(x, y, width, height));
    settings.setValue(kMaximizedKey, maximized);
}
