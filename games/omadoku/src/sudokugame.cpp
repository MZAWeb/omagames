#include "sudokugame.h"

#include <QRect>

#include <algorithm>

#include "sudoku.h"
#include "sudokugrader.h"
#include "sudokukeys.h"
#include "windowgeometry.h"

namespace {

// Levels cross to QML, and name a best-times table, as these ids. They are
// stored under the same ids QML sees, so a table survives their order
// changing.
QString difficultyId(Difficulty difficulty) {
    switch (difficulty) {
    case Difficulty::Easy:
        return QStringLiteral("easy");
    case Difficulty::Medium:
        return QStringLiteral("medium");
    case Difficulty::Hard:
        return QStringLiteral("hard");
    case Difficulty::ExtraHard:
        break;
    }
    return QStringLiteral("extrahard");
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

// The five fastest solves per level. Faster is better, and a solve of no
// time at all is a hand-edited file, not a record.
OmaGames::ScoreTable timesTable() {
    QStringList ids;
    for (int i = 0; i < kDifficultyCount; ++i)
        ids << difficultyId(Difficulty(i));
    OmaGames::ScoreTable table({QStringLiteral("seconds")}, 5,
                               OmaGames::ScoreTable::sameOrder(ids, OmaGames::ScoreTable::LowerIsBetter));
    table.setMinimumValue(1);
    return table;
}

// Ids shared with QML. They are stable identifiers, never shown to the player:
// the labels beside them are.
const auto kStartId = QStringLiteral("start");
const auto kPlayingId = QStringLiteral("playing");
const auto kWonId = QStringLiteral("won");
const auto kHighlightId = QStringLiteral("highlight");
const auto kNoteId = QStringLiteral("note");
const auto kFillId = QStringLiteral("fill");

QString techniqueName(SudokuGrader::Technique technique) {
    using SudokuGrader::Technique;
    switch (technique) {
    case Technique::LastDigit:
        return SudokuGame::tr("Last digit");
    case Technique::HiddenSingleBox:
        return SudokuGame::tr("Hidden single (box)");
    case Technique::NakedSingle:
        return SudokuGame::tr("Naked single");
    case Technique::HiddenSingleLine:
        return SudokuGame::tr("Hidden single (line)");
    case Technique::NakedPair:
        return SudokuGame::tr("Naked pair");
    case Technique::HiddenPair:
        return SudokuGame::tr("Hidden pair");
    case Technique::NakedTriple:
        return SudokuGame::tr("Naked triple");
    case Technique::HiddenTriple:
        return SudokuGame::tr("Hidden triple");
    case Technique::XWing:
        return SudokuGame::tr("X-wing");
    case Technique::Swordfish:
        return SudokuGame::tr("Swordfish");
    case Technique::XYWing:
        break;
    }
    return SudokuGame::tr("XY-wing");
}

// The rungs a level adds on top of the level below: everything between the
// two ceilings, which is exactly what its puzzles can demand and the easier
// level's never do.
QStringList techniquesIntroducedBy(Difficulty difficulty) {
    const int from = difficulty == Difficulty::Easy
        ? 0 : int(SudokuGenerator::ceiling(Difficulty(int(difficulty) - 1))) + 1;
    QStringList names;
    for (int t = from; t <= int(SudokuGenerator::ceiling(difficulty)); ++t)
        names << techniqueName(SudokuGrader::Technique(t));
    return names;
}

// Writing on every keystroke would hit the disk far too often; a short delay
// still survives a crash or a kill in practice.
constexpr int kSaveDelayMs = 500;

// Highlighter yellow, and an ink dark enough to stay readable on it. The one
// place in the game that is deliberately deaf to the Omarchy theme.
constexpr QColor kHighlightColor(0xff, 0xf0, 0x2b);
constexpr QColor kHighlightInk(0x1a, 0x1a, 0x14);

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
    return difficultyId(m_board.puzzle().difficulty);
}

QString SudokuGame::difficultyLabel() const {
    for (const QVariant &entry : difficulties()) {
        const QVariantMap map = entry.toMap();
        if (map.value(QStringLiteral("id")).toString() == difficulty())
            return map.value(QStringLiteral("label")).toString();
    }
    return {};
}

QString SudokuGame::techniqueLabel() const {
    return techniqueName(m_board.puzzle().hardest);
}

QVariantList SudokuGame::difficulties() {
    struct Level {
        Difficulty difficulty;
        QString label;
        QString description;
    };
    const QVector<Level> levels {
        {Difficulty::Easy, tr("Easy"), tr("Plenty of clues; a box scan or a lone digit always moves you on.")},
        {Difficulty::Medium, tr("Medium"), tr("Fewer clues; scan rows and columns too, or spot a naked pair.")},
        {Difficulty::Hard, tr("Hard"), tr("Needs pencil marks: a hidden pair, naked triple or hidden triple.")},
        {Difficulty::ExtraHard, tr("Extra hard"), tr("Needs an X-wing, swordfish or XY-wing somewhere.")},
    };
    QVariantList list;
    for (const Level &level : levels) {
        list.append(QVariantMap {
            {QStringLiteral("id"), difficultyId(level.difficulty)},
            {QStringLiteral("label"), level.label},
            {QStringLiteral("techniques"), techniquesIntroducedBy(level.difficulty)},
            {QStringLiteral("description"), level.description},
        });
    }
    return list;
}

// An unknown id only reaches here from a stale setting, so it means "no
// opinion" rather than an error.
SudokuGame::ClickMode SudokuGame::modeFromId(const QString &id, ClickMode fallback) {
    if (id == kHighlightId)
        return ClickMode::Highlight;
    if (id == kNoteId)
        return ClickMode::Note;
    if (id == kFillId)
        return ClickMode::Fill;
    return fallback;
}

QString SudokuGame::clickMode() const {
    switch (m_clickMode) {
    case ClickMode::Highlight:
        return kHighlightId;
    case ClickMode::Note:
        return kNoteId;
    case ClickMode::Fill:
        break;
    }
    return kFillId;
}

SudokuGame::SudokuGame(QObject *parent) : QObject(parent), m_times(timesTable()) {
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
    const ClickMode mode = modeFromId(clickMode, ClickMode::Fill);
    if (m_clickMode == mode)
        return;
    m_clickMode = mode;
    m_store.setClickMode(this->clickMode());
    emit clickModeChanged();
}

void SudokuGame::cycleClickMode() {
    switch (m_clickMode) {
    case ClickMode::Highlight:
        setClickMode(kNoteId);
        break;
    case ClickMode::Note:
        setClickMode(kFillId);
        break;
    case ClickMode::Fill:
        setClickMode(kHighlightId);
        break;
    }
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
    difficultyFromId(difficulty, &level);
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
    QVariantList indices;
    indices.reserve(int(m_selection.size()));
    for (int index : m_selection)
        indices.append(index);
    return indices;
}

int SudokuGame::stepped(int index, int deltaRow, int deltaColumn) {
    const int row = qBound(0, Sudoku::rowOf(index) + deltaRow, Sudoku::kSize - 1);
    const int column = qBound(0, Sudoku::colOf(index) + deltaColumn, Sudoku::kSize - 1);
    return row * Sudoku::kSize + column;
}

void SudokuGame::select(int index) {
    if (index < -1 || index >= Sudoku::kCells)
        return;
    if (index == m_cursorIndex && m_selection.size() <= 1)
        return;
    m_selection.clear();
    if (index >= 0)
        m_selection.push_back(index);
    m_cursorIndex = index;
    emit selectionChanged();
    emit cursorValueChanged();
}

void SudokuGame::toggleSelection(int index) {
    if (m_screen != Screen::Playing || index < 0 || index >= Sudoku::kCells)
        return;
    const auto at = std::find(m_selection.begin(), m_selection.end(), index);
    if (at == m_selection.end()) {
        // The cell you just picked is where you are, so the cursor follows.
        m_selection.push_back(index);
        m_cursorIndex = index;
    } else {
        m_selection.erase(at);
        if (m_cursorIndex == index)
            m_cursorIndex = m_selection.empty() ? -1 : m_selection.back();
    }
    emit selectionChanged();
    emit cursorValueChanged();
}

void SudokuGame::moveCursor(int deltaRow, int deltaColumn) {
    if (m_screen != Screen::Playing)
        return;
    if (m_cursorIndex < 0) {
        select(0);
        return;
    }
    select(stepped(m_cursorIndex, deltaRow, deltaColumn));
}

void SudokuGame::extendSelection(int deltaRow, int deltaColumn) {
    if (m_screen != Screen::Playing)
        return;
    if (m_cursorIndex < 0) {
        select(0);
        return;
    }
    // One cell at a time, so everything the cursor passes over joins in and
    // not just where it lands.
    const int steps = std::max(std::abs(deltaRow), std::abs(deltaColumn));
    const int rowStep = deltaRow > 0 ? 1 : deltaRow < 0 ? -1 : 0;
    const int columnStep = deltaColumn > 0 ? 1 : deltaColumn < 0 ? -1 : 0;
    bool moved = false;
    for (int step = 0; step < steps; ++step) {
        const int next = stepped(m_cursorIndex, rowStep, columnStep);
        if (next == m_cursorIndex)
            break;  // the edge of the grid
        m_cursorIndex = next;
        if (std::find(m_selection.begin(), m_selection.end(), next) == m_selection.end())
            m_selection.push_back(next);
        moved = true;
    }
    if (!moved)
        return;
    emit selectionChanged();
    emit cursorValueChanged();
}

void SudokuGame::collapseSelection() {
    if (m_selection.size() <= 1)
        return;
    m_selection.assign(1, m_cursorIndex);  // the cursor stays where it is
    emit selectionChanged();
}

bool SudokuGame::backOut() {
    if (m_selection.size() > 1) {
        collapseSelection();
        return true;
    }
    if (m_highlightDigit >= 0) {
        clearHighlight();
        return true;
    }
    return false;
}

void SudokuGame::enterValue(int digit) {
    if (m_screen != Screen::Playing || m_cursorIndex < 0)
        return;
    // A value lands in one cell — the cursor — and folds the selection back
    // onto it, so the digit after it cannot surprise anyone.
    const std::vector<int> changed = m_board.setValue(m_cursorIndex, digit);
    collapseSelection();
    applyChange(changed);
}

void SudokuGame::toggleNote(int digit) {
    if (m_screen != Screen::Playing)
        return;
    // Every selected cell at once, as one undo step, and all of them end the
    // same way. This is what a multi-selection is for.
    applyChange(m_board.toggleNotes(m_selection, digit));
}

QColor SudokuGame::highlightColor() {
    return kHighlightColor;
}

QColor SudokuGame::highlightInk() {
    return kHighlightInk;
}

void SudokuGame::toggleHighlight(int digit) {
    setHighlightDigit(digit >= 1 && digit <= 9 && digit != m_highlightDigit ? digit : -1);
}

void SudokuGame::clearHighlight() {
    setHighlightDigit(-1);
}

void SudokuGame::pressDigitKey(int digit, int modifiers) {
    // The one place the keyboard contract is written down. It is deliberately
    // deaf to the click mode: a key does the same thing in every mode, so no
    // player has to look at a selector before typing a digit.
    const Qt::KeyboardModifiers mods(modifiers);
    if (mods & (Qt::ControlModifier | Qt::AltModifier))
        toggleHighlight(digit);
    else if (mods & Qt::ShiftModifier)
        toggleNote(digit);
    else
        enterValue(digit);
}

void SudokuGame::clickDigit(int digit) {
    switch (m_clickMode) {
    case ClickMode::Highlight:
        toggleHighlight(digit);
        return;
    case ClickMode::Note:
        toggleNote(digit);
        return;
    case ClickMode::Fill:
        enterValue(digit);
        return;
    }
}

void SudokuGame::erase() {
    if (m_screen != Screen::Playing || m_cursorIndex < 0)
        return;
    applyChange(m_board.erase(m_cursorIndex));
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
        if (!difficultyFromId(level.value(QStringLiteral("id")).toString(), &difficulty))
            continue;
        const QString id = difficultyId(difficulty);
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
        map.insert(difficultyId(Difficulty(i)), m_times.best(difficultyId(Difficulty(i))));
    return map;
}

// The clock has already stopped by the time this runs, so what goes in the
// table is exactly the time the player is about to be shown.
void SudokuGame::recordWin() {
    m_newBestRank = m_times.insert(difficultyId(m_board.puzzle().difficulty),
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

void SudokuGame::setHighlightDigit(int digit) {
    if (m_highlightDigit == digit)
        return;
    m_highlightDigit = digit;
    emit highlightDigitChanged();
}

void SudokuGame::setHasSavedGame(bool hasSavedGame) {
    if (m_hasSavedGame == hasSavedGame)
        return;
    m_hasSavedGame = hasSavedGame;
    emit hasSavedGameChanged();
}

void SudokuGame::selectFirstEmptyCell() {
    for (int i = 0; i < Sudoku::kCells; ++i) {
        if (m_board.value(i) == 0) {
            select(i);
            return;
        }
    }
    select(-1);
}

void SudokuGame::loadSettings() {
    m_times.load();
    // Validation is off by default: a first puzzle should not correct you
    // before you have finished thinking. A choice already stored still wins,
    // and anything unrecognised falls back to the default.
    m_board.setValidateAsYouGo(m_store.validateAsYouGo(false));
    m_clickMode = modeFromId(m_store.clickMode(), ClickMode::Fill);

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
