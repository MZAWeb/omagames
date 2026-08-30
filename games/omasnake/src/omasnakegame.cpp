#include "omasnakegame.h"

#include <QRandomGenerator>
#include <QRect>
#include <QSettings>

#include "choices.h"
#include "windowgeometry.h"

namespace {

const auto kModeKey = QStringLiteral("play/mode");
const auto kDifficultyKey = QStringLiteral("play/difficulty");

const auto kStartId = QStringLiteral("start");
const auto kPlayingId = QStringLiteral("playing");
const auto kGameOverId = QStringLiteral("gameover");

// The top ten of every walls-and-speed pair, ranked on the score, with the
// length the snake reached beside it.
OmaGames::ScoreTable scoreTable() {
    QStringList ids;
    for (int m = 0; m < kModeCount; ++m) {
        for (int d = 0; d < kDifficultyCount; ++d)
            ids << tableId(Mode(m), Difficulty(d));
    }
    return OmaGames::ScoreTable(
        {QStringLiteral("score"), QStringLiteral("length")}, 10,
        OmaGames::ScoreTable::sameOrder(ids, OmaGames::ScoreTable::HigherIsBetter));
}

}  // namespace

OmasnakeGame::OmasnakeGame(QObject *parent)
    : QObject(parent), m_scores(scoreTable()),
      m_pacer(OmaGames::Pacer::Repeating, [this]() { step(); }, this) {
    m_pacer.setTimerType(Qt::PreciseTimer);
    m_pacer.setInterval(kDefaultStepIntervalMs);
    loadSettings();
}

int OmasnakeGame::best() const {
    return m_scores.best(tableId(m_mode, m_difficulty));
}

QString OmasnakeGame::mode() const {
    return Modes::id(m_mode);
}

QString OmasnakeGame::difficulty() const {
    return Difficulties::id(m_difficulty);
}

void OmasnakeGame::loadSettings() {
    QSettings settings;
    Modes::fromId(settings.value(kModeKey).toString(), &m_mode);
    Difficulties::fromId(settings.value(kDifficultyKey).toString(), &m_difficulty);
    m_scores.load();
}

QString OmasnakeGame::phase() const {
    if (!m_game)
        return kStartId;
    return m_game->phase() == Phase::GameOver ? kGameOverId : kPlayingId;
}

QString OmasnakeGame::gameOverReason() const {
    if (!m_game || m_game->phase() != Phase::GameOver)
        return {};
    switch (m_game->death()) {
    case Death::Self:
        return QStringLiteral("self");
    case Death::Filled:
        return QStringLiteral("filled");
    case Death::Wall:
        break;
    }
    return QStringLiteral("wall");
}

void OmasnakeGame::setStepInterval(int interval) {
    if (!m_pacer.setInterval(interval))
        return;
    syncTimer();
    emit stepIntervalChanged();
}

void OmasnakeGame::syncTimer() {
    m_pacer.setRunning(m_game && m_game->phase() == Phase::Playing && !m_game->paused());
}

void OmasnakeGame::startGame(Mode mode, Difficulty difficulty, quint32 seed) {
    m_mode = mode;
    m_difficulty = difficulty;
    QSettings settings;
    settings.setValue(kModeKey, Modes::id(mode));
    settings.setValue(kDifficultyKey, Difficulties::id(difficulty));

    m_game = std::make_unique<Game>(mode, difficulty, seed);
    m_newHighScoreRank = -1;
    emit modeChanged();
    emit difficultyChanged();
    emit bestChanged();
    emit scoreChanged();
    emit lengthChanged();
    emit speedChanged();
    emit phaseChanged();
    emit pausedChanged();
    emit readyChanged();
    emit frameChanged();
    syncTimer();
}

void OmasnakeGame::newGame(const QString &difficulty) {
    Difficulty speed = m_difficulty;
    Difficulties::fromId(difficulty, &speed);
    startGame(m_mode, speed, QRandomGenerator::global()->generate());
}

void OmasnakeGame::restart() {
    if (!m_game)
        return;
    startGame(m_mode, m_difficulty, QRandomGenerator::global()->generate());
}

// The walls rule belongs to a whole run, so it only moves between games.
void OmasnakeGame::setMode(const QString &mode) {
    Mode wanted = m_mode;
    if (m_game || !Modes::fromId(mode, &wanted) || wanted == m_mode)
        return;
    m_mode = wanted;
    QSettings().setValue(kModeKey, Modes::id(m_mode));
    emit modeChanged();
    emit bestChanged();
}

void OmasnakeGame::toggleMode() {
    setMode(Modes::id(m_mode == Mode::Classic ? Mode::Wrap : Mode::Classic));
}

bool OmasnakeGame::directionFromId(const QString &id, Direction *direction) {
    if (id == QStringLiteral("up"))
        *direction = Direction::Up;
    else if (id == QStringLiteral("down"))
        *direction = Direction::Down;
    else if (id == QStringLiteral("left"))
        *direction = Direction::Left;
    else if (id == QStringLiteral("right"))
        *direction = Direction::Right;
    else
        return false;
    return true;
}

void OmasnakeGame::turn(const QString &direction) {
    Direction heading = Direction::Right;
    if (m_game && directionFromId(direction, &heading))
        m_game->turn(heading);
}

void OmasnakeGame::pause() {
    if (!m_game || m_game->paused() || m_game->phase() != Phase::Playing)
        return;
    m_game->setPaused(true);
    syncTimer();
    emit pausedChanged();
}

void OmasnakeGame::resume() {
    if (!m_game || !m_game->paused())
        return;
    m_game->setPaused(false);
    syncTimer();
    emit pausedChanged();
}

void OmasnakeGame::togglePause() {
    if (paused())
        resume();
    else
        pause();
}

void OmasnakeGame::backToStart() {
    if (!m_game)
        return;
    m_game.reset();
    m_pacer.stop();
    emit phaseChanged();
    emit pausedChanged();
    emit readyChanged();
    emit scoreChanged();
    emit lengthChanged();
    emit speedChanged();
    emit frameChanged();
}

void OmasnakeGame::step() {
    if (!m_game || m_game->phase() != Phase::Playing)
        return;
    const int score = m_game->score();
    const int length = m_game->length();
    const int moveTicks = m_game->moveTicks();
    const bool wasReady = m_game->ready();

    for (const Event &event : m_game->tick())
        handle(event);

    if (m_game->score() != score)
        emit scoreChanged();
    if (m_game->length() != length)
        emit lengthChanged();
    if (m_game->moveTicks() != moveTicks)
        emit speedChanged();
    if (m_game->ready() != wasReady)
        emit readyChanged();
    emit frameChanged();
}

void OmasnakeGame::handle(const Event &event) {
    switch (event.type) {
    case Event::Ate:
        emit scored(tr("+%1").arg(event.points), event.at.x(), event.at.y());
        break;
    case Event::BonusEaten:
        emit scored(tr("Bonus +%1").arg(event.points), event.at.x(), event.at.y());
        break;
    case Event::SpeedUp:
        emit spedUp();
        break;
    case Event::GameOver:
        emit crashed();
        finishGame();
        break;
    case Event::BonusSpawned:
    case Event::BonusExpired:
        break;
    }
}

void OmasnakeGame::finishGame() {
    m_newHighScoreRank = m_scores.insert(
        tableId(m_mode, m_difficulty),
        {m_game->score(), QDate::currentDate(), {{QStringLiteral("length"), m_game->length()}}});
    if (m_newHighScoreRank >= 0) {
        m_scores.save();
        emit highScoresChanged();
        emit bestChanged();
    }
    emit phaseChanged();
    syncTimer();
}

QVariantMap OmasnakeGame::windowGeometry() const {
    return OmaGames::WindowGeometry::toVariantMap();
}

void OmasnakeGame::saveWindowGeometry(int x, int y, int width, int height, bool maximized) {
    OmaGames::WindowGeometry::save(QRect(x, y, width, height), maximized);
}
