#include "omanixgame.h"

#include <QRandomGenerator>
#include <QRect>
#include <QSettings>

#include "windowgeometry.h"

namespace {

const auto kDifficultyKey = QStringLiteral("play/difficulty");

const auto kEasyId = QStringLiteral("easy");
const auto kNormalId = QStringLiteral("normal");
const auto kHardId = QStringLiteral("hard");

const auto kStartId = QStringLiteral("start");
const auto kPlayingId = QStringLiteral("playing");
const auto kLevelCompleteId = QStringLiteral("levelcomplete");
const auto kGameOverId = QStringLiteral("gameover");

struct DifficultyInfo {
    Difficulty difficulty;
    QString label;
    QString description;
};

// Difficulties cross to QML, and reach the score table, as these ids.
QString difficultyId(Difficulty difficulty) {
    switch (difficulty) {
    case Difficulty::Easy:
        return kEasyId;
    case Difficulty::Hard:
        return kHardId;
    case Difficulty::Normal:
        break;
    }
    return kNormalId;
}

bool difficultyFromId(const QString &id, Difficulty *difficulty) {
    for (int i = 0; i < kDifficultyCount; ++i) {
        if (difficultyId(Difficulty(i)) == id) {
            *difficulty = Difficulty(i);
            return true;
        }
    }
    return false;
}

// The top ten per difficulty, ranked on the score, with the level reached
// riding along.
OmaGames::ScoreTable scoreTable() {
    return OmaGames::ScoreTable(
        {QStringLiteral("score"), QStringLiteral("level")}, 10,
        OmaGames::ScoreTable::sameOrder({kEasyId, kNormalId, kHardId},
                                        OmaGames::ScoreTable::HigherIsBetter));
}

QVector<DifficultyInfo> difficultyInfos() {
    return {
        {Difficulty::Easy, OmanixGame::tr("Easy"), OmanixGame::tr("Two slow balls, one chaser, a gentle ramp.")},
        {Difficulty::Normal, OmanixGame::tr("Normal"), OmanixGame::tr("Three balls, two chasers, a short step up.")},
        {Difficulty::Hard, OmanixGame::tr("Hard"), OmanixGame::tr("Four fast balls, three chasers, no mercy.")},
    };
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
    return difficultyId(m_difficulty);
}

void OmanixGame::loadSettings() {
    QSettings settings;
    difficultyFromId(settings.value(kDifficultyKey).toString(), &m_difficulty);
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
            {QStringLiteral("id"), difficultyId(info.difficulty)},
            {QStringLiteral("label"), info.label},
            {QStringLiteral("description"), info.description},
        });
    }
    return list;
}

QVariantList OmanixGame::highScores() const {
    QVariantList list;
    for (const DifficultyInfo &info : difficultyInfos()) {
        const QString id = difficultyId(info.difficulty);
        for (const QVariant &entry : m_scores.toVariantList(id)) {
            QVariantMap row = entry.toMap();
            row.insert(QStringLiteral("difficulty"), id);
            row.insert(QStringLiteral("label"), info.label);
            list.append(row);
        }
    }
    return list;
}

QVariantMap OmanixGame::bests() const {
    QVariantMap map;
    for (int i = 0; i < kDifficultyCount; ++i)
        map.insert(difficultyId(Difficulty(i)), m_scores.best(difficultyId(Difficulty(i))));
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
    QSettings().setValue(kDifficultyKey, difficultyId(difficulty));
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
    difficultyFromId(difficulty, &level);
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
    m_pacer.stop();
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
    m_newHighScoreRank = m_scores.insert(
        difficultyId(m_difficulty),
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
