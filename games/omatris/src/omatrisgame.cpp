#include "omatrisgame.h"

#include <QRandomGenerator>
#include <QRect>
#include <algorithm>

#include "bonuses.h"
#include "windowgeometry.h"

namespace {

const auto kStartId = QStringLiteral("start");
const auto kPlayingId = QStringLiteral("playing");
const auto kGameOverId = QStringLiteral("gameover");
const auto kFinishedId = QStringLiteral("finished");

}  // namespace

OmatrisGame::OmatrisGame(QObject *parent)
    : QObject(parent), m_scores(Modes::scoreTable()),
      m_pacer(OmaGames::Pacer::Repeating, [this]() { step(); }, this),
      m_preferences(Preferences::load()) {
    m_pacer.setTimerType(Qt::PreciseTimer);
    m_pacer.setInterval(kDefaultStepIntervalMs);
    applyHandling();
    m_scores.load();
}

// Handling is a preference like the ghost: it outlives the run and the window.
void OmatrisGame::setHandling(const Handling &handling) {
    if (!m_preferences.setHandling(handling))
        return;
    applyHandling();
    emit handlingChanged();
}

void OmatrisGame::applyHandling() {
    m_shift.setTiming(handling().dasTicks, handling().arrTicks);
    // A replay drops as fast as it was recorded dropping, not as the player
    // likes to.
    if (m_game && !m_replay)
        m_game->setSoftDropFactor(handling().softDropFactor);
}

bool OmatrisGame::adjustHandling(const QString &setting, int delta) {
    Handling::Setting which;
    if (!Handling::fromId(setting, &which))
        return false;
    Handling changed = handling();
    if (!changed.step(which, delta))
        return false;
    setHandling(changed);
    return true;
}

void OmatrisGame::resetHandling() {
    setHandling(Handling::defaults());
}

QString OmatrisGame::phase() const {
    if (!m_game)
        return kStartId;
    switch (m_game->phase()) {
    case Phase::GameOver:
        return kGameOverId;
    case Phase::Finished:
        return kFinishedId;
    case Phase::Playing:
        break;
    }
    return kPlayingId;
}

QVariantList OmatrisGame::nextQueue() const {
    QVariantList list;
    if (!m_game)
        return list;
    for (PieceType piece : m_game->nextQueue())
        list.append(int(piece));
    return list;
}

void OmatrisGame::setStepInterval(int interval) {
    if (!m_pacer.setInterval(interval))
        return;
    syncTimer();
    emit stepIntervalChanged();
}

void OmatrisGame::startGame(Mode mode, quint32 seed) {
    endReplay();
    m_preferences.setMode(mode);
    show(std::make_unique<Game>(mode, seed));
}

void OmatrisGame::show(std::unique_ptr<Game> game) {
    m_game = std::move(game);
    m_newHighScoreRank = -1;
    m_shift.clear();
    applyHandling();
    emit modeChanged();
    emit scoreChanged();
    emit levelChanged();
    emit linesChanged();
    emit elapsedChanged();
    emit comboChanged();
    emit holdChanged();
    emit queueChanged();
    emit difficultyChanged();
    emit phaseChanged();
    emit pausedChanged();
    emit frameChanged();
    syncTimer();
}

void OmatrisGame::newGame(const QString &mode) {
    Mode chosen = m_preferences.mode();
    Modes::fromId(mode, &chosen);
    startGame(chosen, QRandomGenerator::global()->generate());
}

void OmatrisGame::restart() {
    if (m_replay)
        restartReplay();
    else if (m_game)
        startGame(m_preferences.mode(), QRandomGenerator::global()->generate());
}

void OmatrisGame::backToStart() {
    if (!m_game)
        return;
    m_game.reset();
    m_pacer.stop();
    endReplay();
    emit phaseChanged();
    emit pausedChanged();
    emit frameChanged();
}

void OmatrisGame::press(int direction) {
    if (!playerInControl())
        return;
    m_shift.press(direction);
    const Snapshot before = snapshot();
    if (direction < 0)
        m_game->moveLeft();
    else
        m_game->moveRight();
    publish(before);
    emit frameChanged();
}

void OmatrisGame::release(int direction) {
    m_shift.release(direction);
}

void OmatrisGame::turn(int quarters) {
    if (!playerInControl())
        return;
    m_game->rotate(quarters);
    emit frameChanged();
}

void OmatrisGame::setSoftDrop(bool on) {
    if (m_game && !m_replay)
        m_game->setSoftDrop(on);
}

void OmatrisGame::hardDrop() {
    if (!playerInControl())
        return;
    const Snapshot before = snapshot();
    apply(m_game->hardDrop());
    publish(before);
    emit frameChanged();
}

void OmatrisGame::swapHold() {
    if (!playerInControl())
        return;
    const Snapshot before = snapshot();
    apply(m_game->hold());
    publish(before);
    emit frameChanged();
}

void OmatrisGame::pause() {
    if (!playing())
        return;
    m_game->setPaused(true);
    m_shift.clear();
    if (!m_replay)
        m_game->setSoftDrop(false);
    syncTimer();
    emit pausedChanged();
}

void OmatrisGame::resume() {
    if (!m_game || !m_game->paused())
        return;
    m_game->setPaused(false);
    syncTimer();
    emit pausedChanged();
}

void OmatrisGame::togglePause() {
    if (paused())
        resume();
    else
        pause();
}

// The ghost is a preference, not a rule: it outlives the run and the window.
void OmatrisGame::toggleGhost() {
    m_preferences.setGhost(!m_preferences.ghost());
    emit ghostEnabledChanged();
}

void OmatrisGame::step() {
    if (!playing())
        return;
    if (m_replay) {
        replayBeats();
        return;
    }
    const Snapshot before = snapshot();
    if (const int shift = m_shift.tick(); shift != 0) {
        if (m_shift.instant())
            m_game->slide(shift);
        else if (shift < 0)
            m_game->moveLeft();
        else
            m_game->moveRight();
    }
    apply(m_game->tick());
    publish(before);
    emit frameChanged();
}

void OmatrisGame::apply(const std::vector<Event> &events) {
    for (const Event &event : events)
        handle(event);
}

void OmatrisGame::handle(const Event &event) {
    switch (event.type) {
    case Event::Locked:
        emit pieceLocked(QVector<int>(event.cells.begin(), event.cells.end()));
        announce(event.clear, event.at);
        break;
    case Event::LinesCleared:
        emit linesCleared(QVector<int>(event.rows.begin(), event.rows.end()));
        break;
    case Event::LevelUp:
        emit levelReached(event.level);
        break;
    case Event::TopOut:
    case Event::Finished:
        finishGame();
        break;
    case Event::Held:
        break;
    }
}

// Popups are placed in visible board cells, which is all QML knows about.
void OmatrisGame::announce(const ClearInfo &clear, QPoint where) {
    if (replayTooFastToRead())
        return;
    const QPoint at(where.x(), std::max(0, where.y() - Board::kHiddenRows));
    for (const QString &text : Bonuses::texts(clear))
        emit bonusEarned(text, at.x(), at.y());
}

OmatrisGame::Snapshot OmatrisGame::snapshot() const {
    if (!m_game)
        return {};
    return {m_game->score(), m_game->level(),        m_game->lines(),       m_game->elapsedMs(),
            m_game->combo(), int(m_game->heldPiece()), m_game->holdAvailable(), m_game->backToBack(),
            m_game->difficulty(), nextQueue()};
}

void OmatrisGame::publish(const Snapshot &before) {
    const Snapshot now = snapshot();
    if (now.score != before.score)
        emit scoreChanged();
    if (now.level != before.level)
        emit levelChanged();
    if (now.lines != before.lines)
        emit linesChanged();
    if (now.elapsed != before.elapsed)
        emit elapsedChanged();
    if (now.combo != before.combo || now.backToBack != before.backToBack)
        emit comboChanged();
    if (now.hold != before.hold || now.holdAvailable != before.holdAvailable)
        emit holdChanged();
    if (now.queue != before.queue)
        emit queueChanged();
    if (now.difficulty != before.difficulty)
        emit difficultyChanged();
}

void OmatrisGame::finishGame() {
    // Someone else's game is never the player's high score.
    m_newHighScoreRank = m_replay ? -1 : Modes::record(m_scores, *m_game);
    if (m_newHighScoreRank >= 0) {
        m_scores.save();
        emit highScoresChanged();
    }
    m_shift.clear();
    emit phaseChanged();
    syncTimer();
}

QVariantMap OmatrisGame::windowGeometry() const {
    return OmaGames::WindowGeometry::toVariantMap();
}

void OmatrisGame::saveWindowGeometry(int x, int y, int width, int height, bool maximized) {
    OmaGames::WindowGeometry::save(QRect(x, y, width, height), maximized);
}
