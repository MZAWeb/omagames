#include "omasweepergame.h"

#include <QRandomGenerator>
#include <QRect>
#include <QSettings>
#include <algorithm>

namespace {

const auto kPresetKey = QStringLiteral("play/preset");
const auto kGeometryKey = QStringLiteral("window/geometry");
const auto kMaximizedKey = QStringLiteral("window/maximized");

const auto kStartId = QStringLiteral("start");
const auto kReadyId = QStringLiteral("ready");
const auto kPlayingId = QStringLiteral("playing");
const auto kWonId = QStringLiteral("won");
const auto kLostId = QStringLiteral("lost");

quint32 freshSeed() {
    return QRandomGenerator::global()->generate();
}

}  // namespace

OmasweeperGame::OmasweeperGame(QObject *parent) : QObject(parent) {
    connect(&m_timer, &QTimer::timeout, this, &OmasweeperGame::step);
    loadSettings();
}

void OmasweeperGame::loadSettings() {
    QSettings settings;
    BestTimes::presetFromId(settings.value(kPresetKey).toString(), &m_preset);
    m_times.load();
}

QString OmasweeperGame::phase() const {
    if (!m_board)
        return kStartId;
    switch (m_board->status()) {
    case Status::Won:
        return kWonId;
    case Status::Lost:
        return kLostId;
    case Status::Ready:
    case Status::Playing:
        break;
    }
    return kPlayingId;
}

QString OmasweeperGame::status() const {
    if (!m_board)
        return kReadyId;
    switch (m_board->status()) {
    case Status::Ready:
        return kReadyId;
    case Status::Won:
        return kWonId;
    case Status::Lost:
        return kLostId;
    case Status::Playing:
        break;
    }
    return kPlayingId;
}

QString OmasweeperGame::presetLabel() const {
    return QString::fromLatin1(spec().label);
}

QVariantList OmasweeperGame::presets() {
    QVariantList list;
    for (const PresetSpec &s : Presets::kAll) {
        list.append(QVariantMap {
            {QStringLiteral("id"), QString::fromLatin1(s.key)},
            {QStringLiteral("label"), QString::fromLatin1(s.label)},
            {QStringLiteral("width"), s.width},
            {QStringLiteral("height"), s.height},
            {QStringLiteral("mines"), s.mines},
        });
    }
    return list;
}

QVariantList OmasweeperGame::bestTimes() const {
    QVariantList list;
    for (const PresetSpec &s : Presets::kAll) {
        for (const TimeEntry &e : m_times.entries(s.id)) {
            list.append(QVariantMap {
                {QStringLiteral("preset"), QString::fromLatin1(s.key)},
                {QStringLiteral("label"), QString::fromLatin1(s.label)},
                {QStringLiteral("seconds"), e.seconds},
                {QStringLiteral("date"), e.date.toString(Qt::ISODate)},
            });
        }
    }
    return list;
}

QVariantMap OmasweeperGame::bests() const {
    QVariantMap map;
    for (const PresetSpec &s : Presets::kAll)
        map.insert(QString::fromLatin1(s.key), m_times.best(s.id));
    return map;
}

void OmasweeperGame::setStepInterval(int interval) {
    if (m_stepInterval == interval)
        return;
    m_stepInterval = interval;
    syncTimer();
    emit stepIntervalChanged();
}

void OmasweeperGame::syncTimer() {
    if (m_board && m_board->status() == Status::Playing && m_stepInterval > 0)
        m_timer.start(m_stepInterval);
    else
        m_timer.stop();
}

void OmasweeperGame::startGame(Preset preset, quint32 seed) {
    m_preset = preset;
    QSettings().setValue(kPresetKey, BestTimes::idFor(preset));
    m_seed = seed;
    const PresetSpec &s = Presets::spec(preset);
    m_board.emplace(s.width, s.height, s.mines, seed);
    m_status = Status::Ready;
    m_elapsedSeconds = 0;
    m_newBestRank = -1;
    m_exploded = -1;
    m_noGuess = true;
    m_ripple = {};
    m_cursor = QPoint(s.width / 2, s.height / 2);
    emit presetChanged();
    emit countersChanged();
    emit elapsedSecondsChanged();
    emit cursorChanged();
    emit noGuessChanged();
    emit phaseChanged();
    emit fieldChanged();
    syncTimer();
}

void OmasweeperGame::newGame(const QString &preset) {
    Preset chosen = m_preset;
    BestTimes::presetFromId(preset, &chosen);
    startGame(chosen, freshSeed());
}

void OmasweeperGame::restart() {
    startGame(m_preset, freshSeed());
}

void OmasweeperGame::backToStart() {
    if (!m_board)
        return;
    m_board.reset();
    m_status = Status::Ready;
    m_ripple = {};
    m_timer.stop();
    emit phaseChanged();
    emit countersChanged();
    emit fieldChanged();
}

// The board is drawn only now, so the first cell opened is always safe and
// the layout behind it is one the solver can finish.
void OmasweeperGame::generate(QPoint firstClick) {
    std::vector<int> flagged;
    for (int i = 0; i < m_board->cellCount(); ++i) {
        if (m_board->cell(i).state == CellState::Flagged)
            flagged.push_back(i);
    }
    const GenerationResult result = Generator::generate(spec(), firstClick, m_seed, m_maxAttempts);
    m_board = result.board;
    // Flags planted before the first click outlive the layout behind them.
    for (int i : flagged)
        m_board->toggleFlag(m_board->point(i));
    if (m_noGuess != result.guaranteed) {
        m_noGuess = result.guaranteed;
        emit noGuessChanged();
    }
}

void OmasweeperGame::setCursor(QPoint cursor) {
    if (!m_board)
        return;
    const QPoint clamped(std::clamp(cursor.x(), 0, m_board->width() - 1),
                         std::clamp(cursor.y(), 0, m_board->height() - 1));
    if (clamped == m_cursor)
        return;
    m_cursor = clamped;
    emit cursorChanged();
    emit fieldChanged();
}

void OmasweeperGame::moveCursor(int dx, int dy) {
    setCursor(m_cursor + QPoint(dx, dy));
}

void OmasweeperGame::reveal(int x, int y) {
    if (!m_board || !m_board->contains({x, y}))
        return;
    const QPoint p(x, y);
    setCursor(p);
    // Space on an open number is the chord: one key for both gestures.
    if (m_board->cell(p).state == CellState::Revealed) {
        apply(m_board->chord(p), p);
        return;
    }
    // A flag is a deliberate "not here": it takes an unflag to open the cell,
    // and it must not be what draws the board.
    if (m_board->cell(p).state == CellState::Flagged)
        return;
    if (!m_board->minesPlaced())
        generate(p);
    apply(m_board->reveal(p), p);
}

void OmasweeperGame::toggleFlag(int x, int y) {
    if (!m_board || !m_board->contains({x, y}))
        return;
    const QPoint p(x, y);
    setCursor(p);
    apply(m_board->toggleFlag(p), p);
}

void OmasweeperGame::chord(int x, int y) {
    if (!m_board || !m_board->contains({x, y}))
        return;
    const QPoint p(x, y);
    setCursor(p);
    apply(m_board->chord(p), p);
}

void OmasweeperGame::revealAtCursor() {
    reveal(m_cursor.x(), m_cursor.y());
}

void OmasweeperGame::flagAtCursor() {
    toggleFlag(m_cursor.x(), m_cursor.y());
}

void OmasweeperGame::chordAtCursor() {
    chord(m_cursor.x(), m_cursor.y());
}

void OmasweeperGame::apply(const MoveResult &result, QPoint origin) {
    if (!result.changed())
        return;
    if (result.revealed.empty())
        m_ripple = {};
    else
        buildRipple(result.revealed, origin);
    if (result.exploded >= 0)
        m_exploded = result.exploded;
    const Status status = m_board->status();
    if (status != m_status) {
        m_status = status;
        if (status == Status::Won || status == Status::Lost)
            finishGame(status);
        syncTimer();
        emit phaseChanged();
    }
    emit countersChanged();
    emit fieldChanged();
    if (!result.revealed.empty())
        emit revealRippled();
}

// Chebyshev distance: a cascade spreads a ring of cells per step, which is
// exactly how the view should light them up.
void OmasweeperGame::buildRipple(const std::vector<int> &cells, QPoint origin) {
    m_ripple.cells.clear();
    m_ripple.distances.clear();
    m_ripple.cells.reserve(int(cells.size()));
    m_ripple.distances.reserve(int(cells.size()));
    m_ripple.maxDistance = 0;
    for (int index : cells) {
        const QPoint p = m_board->point(index);
        const int distance = std::max(std::abs(p.x() - origin.x()), std::abs(p.y() - origin.y()));
        m_ripple.cells.append(index);
        m_ripple.distances.append(distance);
        m_ripple.maxDistance = std::max(m_ripple.maxDistance, distance);
    }
}

void OmasweeperGame::finishGame(Status status) {
    if (status != Status::Won)
        return;
    m_newBestRank = m_times.insert(m_preset, {m_elapsedSeconds, QDate::currentDate()});
    if (m_newBestRank >= 0) {
        m_times.save();
        emit bestTimesChanged();
    }
}

void OmasweeperGame::step() {
    if (!m_board || m_board->status() != Status::Playing)
        return;
    ++m_elapsedSeconds;
    emit elapsedSecondsChanged();
}

QVariantMap OmasweeperGame::windowGeometry() const {
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

void OmasweeperGame::saveWindowGeometry(int x, int y, int width, int height, bool maximized) {
    QSettings settings;
    settings.setValue(kGeometryKey, QRect(x, y, width, height));
    settings.setValue(kMaximizedKey, maximized);
}
