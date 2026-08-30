#include "omasnakegame.h"

#include <QRandomGenerator>
#include <QRect>
#include <QSettings>
#include <QVector>

namespace {

const auto kModeKey = QStringLiteral("play/mode");
const auto kDifficultyKey = QStringLiteral("play/difficulty");
const auto kGeometryKey = QStringLiteral("window/geometry");
const auto kMaximizedKey = QStringLiteral("window/maximized");

const auto kStartId = QStringLiteral("start");
const auto kPlayingId = QStringLiteral("playing");
const auto kGameOverId = QStringLiteral("gameover");

QVector<QPair<Mode, QString>> modeLabels() {
    return {{Mode::Classic, OmasnakeGame::tr("Classic")}, {Mode::Wrap, OmasnakeGame::tr("Wrap")}};
}

QString modeDescription(Mode mode) {
    return mode == Mode::Wrap ? OmasnakeGame::tr("The edges carry you across; only your own tail can stop you.")
                              : OmasnakeGame::tr("The walls kill. No grace, no second chance.");
}

QVector<QPair<Difficulty, QString>> difficultyLabels() {
    return {{Difficulty::Slow, OmasnakeGame::tr("Slow")},
            {Difficulty::Normal, OmasnakeGame::tr("Normal")},
            {Difficulty::Fast, OmasnakeGame::tr("Fast")}};
}

// Spelled out from the ladder itself, so the start screen can never drift
// from what the engine actually does.
QString difficultyDescription(Difficulty difficulty) {
    const SpeedParams speed = Rules::params(difficulty);
    const double from = double(Rules::kTicksPerSecond) / speed.startMoveTicks;
    const double to = double(Rules::kTicksPerSecond) / speed.minMoveTicks;
    return OmasnakeGame::tr("%1 to %2 cells a second, one step faster every %3 foods.")
        .arg(from, 0, 'f', 1)
        .arg(to, 0, 'f', 1)
        .arg(speed.foodsPerSpeedUp);
}

}  // namespace

OmasnakeGame::OmasnakeGame(QObject *parent) : QObject(parent) {
    m_timer.setTimerType(Qt::PreciseTimer);
    connect(&m_timer, &QTimer::timeout, this, &OmasnakeGame::step);
    loadSettings();
}

void OmasnakeGame::loadSettings() {
    QSettings settings;
    HighScores::modeFromId(settings.value(kModeKey).toString(), &m_mode);
    HighScores::difficultyFromId(settings.value(kDifficultyKey).toString(), &m_difficulty);
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

QString OmasnakeGame::modeLabel() const {
    for (const auto &[mode, label] : modeLabels()) {
        if (mode == m_mode)
            return label;
    }
    return {};
}

QString OmasnakeGame::difficultyLabel() const {
    for (const auto &[difficulty, label] : difficultyLabels()) {
        if (difficulty == m_difficulty)
            return label;
    }
    return {};
}

QVariantList OmasnakeGame::modes() {
    QVariantList list;
    for (const auto &[mode, label] : modeLabels()) {
        list.append(QVariantMap {
            {QStringLiteral("id"), HighScores::modeId(mode)},
            {QStringLiteral("label"), label},
            {QStringLiteral("description"), modeDescription(mode)},
        });
    }
    return list;
}

QVariantList OmasnakeGame::difficulties() {
    QVariantList list;
    for (const auto &[difficulty, label] : difficultyLabels()) {
        list.append(QVariantMap {
            {QStringLiteral("id"), HighScores::difficultyId(difficulty)},
            {QStringLiteral("label"), label},
            {QStringLiteral("description"), difficultyDescription(difficulty)},
        });
    }
    return list;
}

QVariantList OmasnakeGame::highScores() const {
    QVariantList list;
    for (const auto &[mode, modeName] : modeLabels()) {
        for (const auto &[difficulty, difficultyName] : difficultyLabels()) {
            for (const ScoreEntry &e : m_scores.entries(mode, difficulty)) {
                list.append(QVariantMap {
                    {QStringLiteral("table"), HighScores::idFor(mode, difficulty)},
                    {QStringLiteral("mode"), HighScores::modeId(mode)},
                    {QStringLiteral("difficulty"), HighScores::difficultyId(difficulty)},
                    {QStringLiteral("label"), difficultyName},
                    {QStringLiteral("score"), e.score},
                    {QStringLiteral("length"), e.length},
                    {QStringLiteral("date"), e.date.toString(Qt::ISODate)},
                });
            }
        }
    }
    return list;
}

QVariantMap OmasnakeGame::bests() const {
    QVariantMap map;
    for (int m = 0; m < kModeCount; ++m) {
        for (int d = 0; d < kDifficultyCount; ++d)
            map.insert(HighScores::idFor(Mode(m), Difficulty(d)), m_scores.best(Mode(m), Difficulty(d)));
    }
    return map;
}

void OmasnakeGame::setStepInterval(int interval) {
    if (m_stepInterval == interval)
        return;
    m_stepInterval = interval;
    syncTimer();
    emit stepIntervalChanged();
}

void OmasnakeGame::syncTimer() {
    const bool running = m_game && m_game->phase() == Phase::Playing && !m_game->paused() && m_stepInterval > 0;
    if (running) {
        m_timer.start(m_stepInterval);
    } else {
        m_timer.stop();
    }
}

void OmasnakeGame::startGame(Mode mode, Difficulty difficulty, quint32 seed) {
    m_mode = mode;
    m_difficulty = difficulty;
    QSettings settings;
    settings.setValue(kModeKey, HighScores::modeId(mode));
    settings.setValue(kDifficultyKey, HighScores::difficultyId(difficulty));

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
    HighScores::difficultyFromId(difficulty, &speed);
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
    if (m_game || !HighScores::modeFromId(mode, &wanted) || wanted == m_mode)
        return;
    m_mode = wanted;
    QSettings().setValue(kModeKey, HighScores::modeId(m_mode));
    emit modeChanged();
    emit bestChanged();
}

void OmasnakeGame::toggleMode() {
    setMode(HighScores::modeId(m_mode == Mode::Classic ? Mode::Wrap : Mode::Classic));
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
    m_timer.stop();
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
    m_newHighScoreRank = m_scores.insert(m_mode, m_difficulty, {m_game->score(), m_game->length(), QDate::currentDate()});
    if (m_newHighScoreRank >= 0) {
        m_scores.save();
        emit highScoresChanged();
        emit bestChanged();
    }
    emit phaseChanged();
    syncTimer();
}

QVariantMap OmasnakeGame::windowGeometry() const {
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

void OmasnakeGame::saveWindowGeometry(int x, int y, int width, int height, bool maximized) {
    QSettings settings;
    settings.setValue(kGeometryKey, QRect(x, y, width, height));
    settings.setValue(kMaximizedKey, maximized);
}
