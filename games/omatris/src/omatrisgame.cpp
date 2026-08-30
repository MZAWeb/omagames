#include "omatrisgame.h"

#include <QRandomGenerator>
#include <QSettings>
#include <algorithm>

#include "windowgeometry.h"

namespace {

const auto kModeKey = QStringLiteral("play/mode");
const auto kGhostKey = QStringLiteral("play/ghost");

const auto kMarathonId = QStringLiteral("marathon");
const auto kSprintId = QStringLiteral("sprint");
const auto kZenId = QStringLiteral("zen");

const auto kStartId = QStringLiteral("start");
const auto kPlayingId = QStringLiteral("playing");
const auto kGameOverId = QStringLiteral("gameover");
const auto kFinishedId = QStringLiteral("finished");

struct ModeInfo {
    Mode mode;
    QString label;
    QString description;
};

// Modes cross to QML, and name a score table, as these ids.
QString modeId(Mode mode) {
    switch (mode) {
    case Mode::Sprint:
        return kSprintId;
    case Mode::Zen:
        return kZenId;
    case Mode::Marathon:
        break;
    }
    return kMarathonId;
}

bool modeFromId(const QString &id, Mode *mode) {
    for (int i = 0; i < kModeCount; ++i) {
        if (modeId(Mode(i)) == id) {
            *mode = Mode(i);
            return true;
        }
    }
    return false;
}

// The top ten per mode. A run keeps its score, lines, level and clock
// whatever the mode; which of them ranks is the mode's business, and Sprint
// is the one raced against the clock.
OmaGames::ScoreTable scoreTable() {
    std::vector<OmaGames::ScoreTable::Category> categories;
    for (int i = 0; i < kModeCount; ++i) {
        const Mode mode = Mode(i);
        const bool byTime = Rules::params(mode).rankByTime;
        categories.push_back({modeId(mode),
                              byTime ? OmaGames::ScoreTable::LowerIsBetter
                                     : OmaGames::ScoreTable::HigherIsBetter,
                              byTime ? QStringLiteral("millis") : QString()});
    }
    return OmaGames::ScoreTable({QStringLiteral("score"), QStringLiteral("lines"),
                                 QStringLiteral("level"), QStringLiteral("millis")},
                                10, std::move(categories));
}

QVector<ModeInfo> modeInfos() {
    return {
        {Mode::Marathon, OmatrisGame::tr("Marathon"), OmatrisGame::tr("Endless. The levels keep coming and so does gravity.")},
        {Mode::Sprint, OmatrisGame::tr("Sprint"), OmatrisGame::tr("Forty lines at the first level's pace. The clock is the score.")},
        {Mode::Zen, OmatrisGame::tr("Zen"), OmatrisGame::tr("Endless and never faster. Stack for as long as you like.")},
    };
}

// What a clear is called on the popup, or nothing when it is not worth saying.
QString clearName(const ClearInfo &clear) {
    static const QString kLineNames[5] = {{}, OmatrisGame::tr("Single"), OmatrisGame::tr("Double"),
                                          OmatrisGame::tr("Triple"), OmatrisGame::tr("Tetris")};
    if (clear.spin == Spin::None)
        return clear.lines == 4 ? kLineNames[4] : QString();
    const QString spin = clear.spin == Spin::Mini ? OmatrisGame::tr("T-Spin Mini") : OmatrisGame::tr("T-Spin");
    return clear.lines == 0 ? spin : spin + QLatin1Char(' ') + kLineNames[clear.lines];
}

}  // namespace

OmatrisGame::OmatrisGame(QObject *parent)
    : QObject(parent), m_scores(scoreTable()),
      m_pacer(OmaGames::Pacer::Repeating, [this]() { step(); }, this) {
    m_pacer.setTimerType(Qt::PreciseTimer);
    m_pacer.setInterval(kDefaultStepIntervalMs);
    loadSettings();
}

QString OmatrisGame::mode() const {
    return modeId(m_mode);
}

void OmatrisGame::loadSettings() {
    QSettings settings;
    modeFromId(settings.value(kModeKey).toString(), &m_mode);
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

QString OmatrisGame::modeLabel() const {
    for (const ModeInfo &info : modeInfos()) {
        if (info.mode == m_mode)
            return info.label;
    }
    return {};
}

QVariantList OmatrisGame::modes() {
    QVariantList list;
    for (const ModeInfo &info : modeInfos()) {
        list.append(QVariantMap {
            {QStringLiteral("id"), modeId(info.mode)},
            {QStringLiteral("label"), info.label},
            {QStringLiteral("description"), info.description},
            {QStringLiteral("goal"), Rules::params(info.mode).lineGoal},
        });
    }
    return list;
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

QVariantList OmatrisGame::highScores() const {
    QVariantList list;
    for (const ModeInfo &info : modeInfos()) {
        const QString id = modeId(info.mode);
        for (const QVariant &entry : m_scores.toVariantList(id)) {
            QVariantMap row = entry.toMap();
            row.insert(QStringLiteral("mode"), id);
            row.insert(QStringLiteral("label"), info.label);
            list.append(row);
        }
    }
    return list;
}

QVariantMap OmatrisGame::bests() const {
    QVariantMap map;
    for (int i = 0; i < kModeCount; ++i)
        map.insert(modeId(Mode(i)), m_scores.best(modeId(Mode(i))));
    return map;
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
    QSettings().setValue(kModeKey, modeId(mode));
    m_game = std::make_unique<Game>(mode, seed);
    m_newHighScoreRank = -1;
    m_shift = 0;
    m_shiftTicks = 0;
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
    modeFromId(mode, &chosen);
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
    m_shift = direction;
    m_shiftTicks = 0;
    const Snapshot before = snapshot();
    if (direction < 0)
        m_game->moveLeft();
    else
        m_game->moveRight();
    publish(before);
    emit frameChanged();
}

void OmatrisGame::release(int direction) {
    if (m_shift == direction)
        m_shift = 0;
}

void OmatrisGame::autoShift() {
    if (m_shift == 0)
        return;
    ++m_shiftTicks;
    if (m_shiftTicks < kDasTicks || (m_shiftTicks - kDasTicks) % kArrTicks != 0)
        return;
    if (m_shift < 0)
        m_game->moveLeft();
    else
        m_game->moveRight();
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
    m_shift = 0;
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
    autoShift();
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
    const QString name = clearName(clear);
    if (!name.isEmpty())
        emit bonusEarned(name, at.x(), at.y());
    if (clear.backToBack)
        emit bonusEarned(tr("Back-to-Back"), at.x(), at.y());
    if (clear.combo >= 1)
        emit bonusEarned(tr("Combo x%1").arg(clear.combo), at.x(), at.y());
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
        ranked ? m_scores.insert(modeId(m_mode),
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
    m_shift = 0;
    emit phaseChanged();
    syncTimer();
}

QVariantMap OmatrisGame::windowGeometry() const {
    return OmaGames::WindowGeometry::toVariantMap();
}

void OmatrisGame::saveWindowGeometry(int x, int y, int width, int height, bool maximized) {
    OmaGames::WindowGeometry::save(QRect(x, y, width, height), maximized);
}
