#include "omatrisgame.h"

#include <QRandomGenerator>
#include <QRect>
#include <QSettings>
#include <algorithm>

#include "bonuses.h"
#include "windowgeometry.h"

namespace {

const auto kModeKey = QStringLiteral("play/mode");
const auto kGhostKey = QStringLiteral("play/ghost");

const auto kStartId = QStringLiteral("start");
const auto kPlayingId = QStringLiteral("playing");
const auto kGameOverId = QStringLiteral("gameover");
const auto kFinishedId = QStringLiteral("finished");

}  // namespace

OmatrisGame::OmatrisGame(QObject *parent)
    : QObject(parent), m_scores(Modes::scoreTable()),
      m_pacer(OmaGames::Pacer::Repeating, [this]() { step(); }, this) {
    m_pacer.setTimerType(Qt::PreciseTimer);
    m_pacer.setInterval(kDefaultStepIntervalMs);
    loadSettings();
}

void OmatrisGame::loadSettings() {
    QSettings settings;
    Modes::fromId(settings.value(kModeKey).toString(), &m_mode);
    m_ghostEnabled = settings.value(kGhostKey, true).toBool();
    m_scores.load();
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

bool OmatrisGame::playing() const {
    return m_game && m_game->phase() == Phase::Playing && !m_game->paused();
}

int OmatrisGame::holdPiece() const {
    return m_game ? int(m_game->heldPiece()) : int(PieceType::None);
}

QVariantList OmatrisGame::nextQueue() const {
    QVariantList list;
    if (!m_game)
        return list;
    for (PieceType piece : m_game->nextQueue())
        list.append(int(piece));
    return list;
}

QVariantMap OmatrisGame::pieceShape(int piece) const {
    QVariantList cells;
    if (piece < 0 || piece >= kPieceCount)
        return {{QStringLiteral("cells"), cells}, {QStringLiteral("width"), 0}, {QStringLiteral("height"), 0}};
    const PieceCells shape = Piece::cells(PieceType(piece), 0);
    QPoint origin = shape.front();
    QPoint far = shape.front();
    for (QPoint cell : shape) {
        origin = {std::min(origin.x(), cell.x()), std::min(origin.y(), cell.y())};
        far = {std::max(far.x(), cell.x()), std::max(far.y(), cell.y())};
    }
    for (QPoint cell : shape) {
        cells.append(QVariantMap {{QStringLiteral("x"), cell.x() - origin.x()},
                                  {QStringLiteral("y"), cell.y() - origin.y()}});
    }
    return {{QStringLiteral("cells"), cells},
            {QStringLiteral("width"), far.x() - origin.x() + 1},
            {QStringLiteral("height"), far.y() - origin.y() + 1}};
}

void OmatrisGame::setStepInterval(int interval) {
    if (!m_pacer.setInterval(interval))
        return;
    syncTimer();
    emit stepIntervalChanged();
}

void OmatrisGame::syncTimer() {
    m_pacer.setRunning(playing());
}

void OmatrisGame::startGame(Mode mode, quint32 seed) {
    m_mode = mode;
    QSettings().setValue(kModeKey, Modes::id(mode));
    m_game = std::make_unique<Game>(mode, seed);
    m_newHighScoreRank = -1;
    m_shift.clear();
    emit modeChanged();
    emit scoreChanged();
    emit levelChanged();
    emit linesChanged();
    emit elapsedChanged();
    emit comboChanged();
    emit holdChanged();
    emit queueChanged();
    emit phaseChanged();
    emit pausedChanged();
    emit frameChanged();
    syncTimer();
}

void OmatrisGame::newGame(const QString &mode) {
    Mode chosen = m_mode;
    Modes::fromId(mode, &chosen);
    startGame(chosen, QRandomGenerator::global()->generate());
}

void OmatrisGame::restart() {
    if (m_game)
        startGame(m_mode, QRandomGenerator::global()->generate());
}

void OmatrisGame::backToStart() {
    if (!m_game)
        return;
    m_game.reset();
    m_pacer.stop();
    emit phaseChanged();
    emit pausedChanged();
    emit frameChanged();
}

void OmatrisGame::press(int direction) {
    if (!playing())
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
    if (!playing())
        return;
    m_game->rotate(quarters);
    emit frameChanged();
}

void OmatrisGame::setSoftDrop(bool on) {
    if (m_game)
        m_game->setSoftDrop(on);
}

void OmatrisGame::hardDrop() {
    if (!playing())
        return;
    const Snapshot before = snapshot();
    apply(m_game->hardDrop());
    publish(before);
    emit frameChanged();
}

void OmatrisGame::swapHold() {
    if (!playing())
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
    m_ghostEnabled = !m_ghostEnabled;
    QSettings().setValue(kGhostKey, m_ghostEnabled);
    emit ghostEnabledChanged();
}

void OmatrisGame::step() {
    if (!playing())
        return;
    const Snapshot before = snapshot();
    if (const int shift = m_shift.tick(); shift < 0)
        m_game->moveLeft();
    else if (shift > 0)
        m_game->moveRight();
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
    const QPoint at(where.x(), std::max(0, where.y() - Board::kHiddenRows));
    for (const QString &text : Bonuses::texts(clear))
        emit bonusEarned(text, at.x(), at.y());
}

OmatrisGame::Snapshot OmatrisGame::snapshot() const {
    if (!m_game)
        return {};
    return {m_game->score(), m_game->level(),        m_game->lines(),       m_game->elapsedMs(),
            m_game->combo(), int(m_game->heldPiece()), m_game->holdAvailable(), m_game->backToBack(),
            nextQueue()};
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
}

void OmatrisGame::finishGame() {
    // A Sprint that tops out never crossed the line, so it has no time to keep.
    const bool ranked = m_game->phase() == Phase::Finished || !rankByTime();
    m_newHighScoreRank =
        ranked ? m_scores.insert(Modes::id(m_mode),
                                 {rankByTime() ? m_game->elapsedMs() : m_game->score(),
                                  QDate::currentDate(),
                                  {{QStringLiteral("score"), m_game->score()},
                                   {QStringLiteral("lines"), m_game->lines()},
                                   {QStringLiteral("level"), m_game->level()},
                                   {QStringLiteral("millis"), m_game->elapsedMs()}}})
               : -1;
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
