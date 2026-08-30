#include "sudokugame.h"

#include <QRect>

#include "sudokukeys.h"
#include "sudokulevels.h"
#include "windowgeometry.h"

namespace {

// Ids shared with QML. They are stable identifiers, never shown to the player:
// the labels beside them are.
const auto kStartId = QStringLiteral("start");
const auto kPlayingId = QStringLiteral("playing");
const auto kWonId = QStringLiteral("won");

// Writing on every keystroke would hit the disk far too often; a short delay
// still survives a crash or a kill in practice.
constexpr int kSaveDelayMs = 500;

}  // namespace

QString SudokuGame::state() const {
    switch (m_screen) {
    case Screen::Playing:
        return kPlayingId;
    case Screen::Won:
        return kWonId;
    case Screen::Start:
        break;
    }
    return kStartId;
}

QString SudokuGame::difficulty() const {
    return SudokuLevels::id(m_board.puzzle().difficulty);
}

QString SudokuGame::difficultyLabel() const {
    return SudokuLevels::label(difficulty());
}

QString SudokuGame::techniqueLabel() const {
    return SudokuLevels::techniqueName(m_board.puzzle().hardest);
}

QVariantList SudokuGame::difficulties() {
    return SudokuLevels::all();
}

QString SudokuGame::clickMode() const {
    return m_input.clickMode();
}

SudokuGame::SudokuGame(QObject *parent) : QObject(parent), m_times(SudokuLevels::timesTable()) {
    m_clock.setInterval(1000);
    connect(&m_clock, &QTimer::timeout, this, [this]() {
        ++m_elapsedSeconds;
        emit elapsedSecondsChanged();
    });

    m_saveTimer.setSingleShot(true);
    m_saveTimer.setInterval(kSaveDelayMs);
    connect(&m_saveTimer, &QTimer::timeout, this, &SudokuGame::saveGame);

    loadSettings();
}

SudokuGame::~SudokuGame() {
    if (inProgress())
        saveGame();
}

bool SudokuGame::inProgress() const {
    // Only the player's own work counts: leaving an untouched puzzle needs no
    // confirmation and is not worth saving.
    return m_screen == Screen::Playing && m_board.entryCount() > 0;
}

QVariantList SudokuGame::digitCounts() const {
    QVariantList counts;
    for (int digit = 1; digit <= 9; ++digit)
        counts.append(m_board.digitCount(digit));
    return counts;
}

void SudokuGame::setClickMode(const QString &clickMode) {
    if (!m_input.setClickMode(clickMode))
        return;
    m_store.setClickMode(m_input.clickMode());
    emit clickModeChanged();
}

void SudokuGame::cycleClickMode() {
    setClickMode(m_input.nextClickMode());
}

void SudokuGame::setValidateAsYouGo(bool validateAsYouGo) {
    if (m_board.validateAsYouGo() == validateAsYouGo)
        return;
    m_board.setValidateAsYouGo(validateAsYouGo);
    m_store.setValidateAsYouGo(validateAsYouGo);
    m_cells.refreshAll();
    emit validateAsYouGoChanged();
    emit boardChanged();
}

int SudokuGame::digitForKey(int key, int modifiers, const QString &text) const {
    return SudokuKeys::digitFor(key, Qt::KeyboardModifiers(modifiers), text);
}

void SudokuGame::newGame(const QString &difficulty) {
    // An unknown id can only come from a caller with a stale idea of the
    // levels; Easy is the safe landing.
    Difficulty level = Difficulty::Easy;
    SudokuLevels::fromId(difficulty, &level);
    m_board.setPuzzle(SudokuGenerator::generate(level));
    m_cells.refreshAll();

    m_elapsedSeconds = 0;
    emit elapsedSecondsChanged();
    m_newBestRank = -1;
    clearHighlight();
    selectFirstEmptyCell();
    clearSavedGame();
    setScreen(Screen::Playing);
    emit boardChanged();
}

void SudokuGame::resumeSavedGame() {
    if (!m_hasSavedGame)
        return;
    m_newBestRank = -1;
    selectFirstEmptyCell();
    setScreen(Screen::Playing);
    emit boardChanged();
}

QVariantList SudokuGame::selectedIndices() const {
    return m_selection.toVariantList();
}

void SudokuGame::emitSelectionChanged() {
    emit selectionChanged();
    emit cursorValueChanged();
}

void SudokuGame::select(int index) {
    if (m_selection.select(index))
        emitSelectionChanged();
}

void SudokuGame::toggleSelection(int index) {
    if (m_screen == Screen::Playing && m_selection.toggle(index))
        emitSelectionChanged();
}

void SudokuGame::moveCursor(int deltaRow, int deltaColumn) {
    if (m_screen == Screen::Playing && m_selection.moveCursor(deltaRow, deltaColumn))
        emitSelectionChanged();
}

void SudokuGame::extendSelection(int deltaRow, int deltaColumn) {
    if (m_screen == Screen::Playing && m_selection.extend(deltaRow, deltaColumn))
        emitSelectionChanged();
}

// Collapsing leaves the cursor where it was, so only the selection changed.
void SudokuGame::collapseSelection() {
    if (m_selection.collapse())
        emit selectionChanged();
}

bool SudokuGame::backOut() {
    if (m_selection.indices().size() > 1) {
        collapseSelection();
        return true;
    }
    if (m_input.highlightDigit() >= 0) {
        clearHighlight();
        return true;
    }
    return false;
}

void SudokuGame::enterValue(int digit) {
    if (m_screen != Screen::Playing || m_selection.cursor() < 0)
        return;
    // A value lands in one cell — the cursor — and folds the selection back
    // onto it, so the digit after it cannot surprise anyone.
    const std::vector<int> changed = m_board.setValue(m_selection.cursor(), digit);
    collapseSelection();
    applyChange(changed);
}

void SudokuGame::toggleNote(int digit) {
    if (m_screen != Screen::Playing)
        return;
    // Every selected cell at once, as one undo step, and all of them end the
    // same way. This is what a multi-selection is for.
    applyChange(m_board.toggleNotes(m_selection.indices(), digit));
}

QColor SudokuGame::highlightColor() {
    return SudokuInput::highlightColor();
}

QColor SudokuGame::highlightInk() {
    return SudokuInput::highlightInk();
}

void SudokuGame::toggleHighlight(int digit) {
    if (m_input.toggleHighlight(digit))
        emit highlightDigitChanged();
}

void SudokuGame::clearHighlight() {
    if (m_input.clearHighlight())
        emit highlightDigitChanged();
}

void SudokuGame::pressDigitKey(int digit, int modifiers) {
    applyDigit(SudokuInput::keyAction(Qt::KeyboardModifiers(modifiers)), digit);
}

void SudokuGame::clickDigit(int digit) {
    applyDigit(m_input.clickAction(), digit);
}

void SudokuGame::applyDigit(SudokuInput::Action action, int digit) {
    switch (action) {
    case SudokuInput::Action::Highlight:
        toggleHighlight(digit);
        return;
    case SudokuInput::Action::Note:
        toggleNote(digit);
        return;
    case SudokuInput::Action::Fill:
        enterValue(digit);
        return;
    }
}

void SudokuGame::erase() {
    if (m_screen != Screen::Playing || m_selection.cursor() < 0)
        return;
    applyChange(m_board.erase(m_selection.cursor()));
}

void SudokuGame::undo() {
    if (m_screen != Screen::Playing)
        return;
    applyChange(m_board.undo());
}

void SudokuGame::requestRestart() {
    if (m_screen != Screen::Playing)
        return;
    // An untouched puzzle has nothing to lose, so it is not worth a question.
    if (!inProgress()) {
        restart();
        return;
    }
    setRestartPending(true);
}

void SudokuGame::confirmRestart() {
    // Only ever the answer to the question requestRestart() asked: without a
    // question outstanding this does nothing, so the board cannot be wiped by
    // a stray call from QML.
    if (!m_restartPending)
        return;
    setRestartPending(false);
    restart();
}

void SudokuGame::cancelRestart() {
    setRestartPending(false);
}

void SudokuGame::restart() {
    if (m_screen != Screen::Playing)
        return;
    applyChange(m_board.restart());
}

void SudokuGame::backToStart() {
    clearHighlight();
    if (inProgress())
        saveGame();
    m_clock.stop();
    setScreen(Screen::Start);
}

void SudokuGame::applyChange(const std::vector<int> &changed) {
    if (changed.empty())
        return;
    // Deferred validation can light up cells far from the edited one, so repaint
    // everything unless we know only these cells changed.
    if (m_board.validateAsYouGo())
        m_cells.refresh(changed);
    else
        m_cells.refreshAll();
    emit boardChanged();
    emit cursorValueChanged();

    if (m_board.isSolved()) {
        m_clock.stop();
        clearSavedGame();
        recordWin();
        setScreen(Screen::Won);
        return;
    }
    m_saveTimer.start();
}

QVariantList SudokuGame::bestTimes() const {
    QVariantList list;
    for (const QVariant &entry : difficulties()) {
        const QVariantMap level = entry.toMap();
        Difficulty difficulty = Difficulty::Easy;
        if (!SudokuLevels::fromId(level.value(QStringLiteral("id")).toString(), &difficulty))
            continue;
        const QString id = SudokuLevels::id(difficulty);
        for (const QVariant &time : m_times.toVariantList(id)) {
            QVariantMap row = time.toMap();
            row.insert(QStringLiteral("difficulty"), id);
            row.insert(QStringLiteral("label"), level.value(QStringLiteral("label")));
            list.append(row);
        }
    }
    return list;
}

QVariantMap SudokuGame::bests() const {
    QVariantMap map;
    for (int i = 0; i < kDifficultyCount; ++i)
        map.insert(SudokuLevels::id(Difficulty(i)), m_times.best(SudokuLevels::id(Difficulty(i))));
    return map;
}

// The clock has already stopped by the time this runs, so what goes in the
// table is exactly the time the player is about to be shown.
void SudokuGame::recordWin() {
    m_newBestRank = m_times.insert(SudokuLevels::id(m_board.puzzle().difficulty),
                                   {m_elapsedSeconds, QDate::currentDate(), {}});
    if (m_newBestRank < 0)
        return;
    m_times.save();
    emit bestTimesChanged();
}

void SudokuGame::setScreen(Screen screen) {
    if (m_screen == screen)
        return;
    m_screen = screen;
    setRestartPending(false);  // no question outlives the puzzle it was about
    // The clock only runs while a puzzle is actually on screen.
    if (m_screen == Screen::Playing)
        m_clock.start();
    else
        m_clock.stop();
    emit stateChanged();
}

void SudokuGame::setRestartPending(bool pending) {
    if (m_restartPending == pending)
        return;
    m_restartPending = pending;
    emit restartPendingChanged();
}

void SudokuGame::setHasSavedGame(bool hasSavedGame) {
    if (m_hasSavedGame == hasSavedGame)
        return;
    m_hasSavedGame = hasSavedGame;
    emit hasSavedGameChanged();
}

void SudokuGame::selectFirstEmptyCell() {
    select(m_board.firstEmptyIndex());
}

void SudokuGame::loadSettings() {
    m_times.load();
    // Validation is off by default: a first puzzle should not correct you
    // before you have finished thinking. A choice already stored still wins,
    // and anything unrecognised falls back to the default.
    m_board.setValidateAsYouGo(m_store.validateAsYouGo(false));
    m_input.setClickMode(m_store.clickMode());

    const SudokuStore::SavedGame saved = m_store.savedGame();
    if (saved.board.isEmpty() || !SudokuBoard::fromJson(saved.board, &m_board))
        return;
    m_elapsedSeconds = saved.elapsedSeconds;
    // A finished puzzle is not worth resuming.
    if (m_board.isSolved()) {
        clearSavedGame();
        return;
    }
    m_cells.refreshAll();
    setHasSavedGame(true);
}

void SudokuGame::saveGame() {
    m_store.saveGame(m_board.toJson(), m_elapsedSeconds);
    setHasSavedGame(true);
}

void SudokuGame::clearSavedGame() {
    m_store.clearSavedGame();
    setHasSavedGame(false);
}

QVariantMap SudokuGame::windowGeometry() const {
    return OmaGames::WindowGeometry::toVariantMap();
}

void SudokuGame::saveWindowGeometry(int x, int y, int width, int height, bool maximized) {
    OmaGames::WindowGeometry::save(QRect(x, y, width, height), maximized);
}
