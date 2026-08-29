#include "sudokugame.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QRect>
#include <QSettings>

#include "sudoku.h"

namespace {

const auto kStateKey = QStringLiteral("state/v1");
const auto kCheckKey = QStringLiteral("play/checkAsYouGo");
const auto kPadModeKey = QStringLiteral("play/padMode");
const auto kGeometryKey = QStringLiteral("window/geometry");
const auto kMaximizedKey = QStringLiteral("window/maximized");
const auto kElapsedKey = QStringLiteral("elapsed");

// Writing on every keystroke would hit the disk far too often; a short delay
// still survives a crash or a kill in practice.
constexpr int kSaveDelayMs = 500;

}  // namespace

SudokuGame::SudokuGame(QObject *parent) : QObject(parent) {
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
    return m_state == Playing && m_board.entryCount() > 0;
}

QVariantList SudokuGame::digitCounts() const {
    QVariantList counts;
    for (int digit = 1; digit <= 9; ++digit)
        counts.append(m_board.digitCount(digit));
    return counts;
}

void SudokuGame::setPadMode(PadMode padMode) {
    if (m_padMode == padMode)
        return;
    m_padMode = padMode;
    QSettings().setValue(kPadModeKey, int(padMode));
    emit padModeChanged();
}

void SudokuGame::cyclePadMode() {
    setPadMode(PadMode((int(m_padMode) + 1) % 3));
}

void SudokuGame::setCheckAsYouGo(bool checkAsYouGo) {
    if (m_board.checkAsYouGo() == checkAsYouGo)
        return;
    m_board.setCheckAsYouGo(checkAsYouGo);
    QSettings().setValue(kCheckKey, checkAsYouGo);
    m_cells.refreshAll();
    emit checkAsYouGoChanged();
    emit boardChanged();
}

void SudokuGame::newGame(int level) {
    const Difficulty difficulty = level >= int(Difficulty::Easy) && level <= int(Difficulty::Hard)
        ? Difficulty(level)
        : Difficulty::Easy;
    m_board.setPuzzle(SudokuGenerator::generate(difficulty));
    m_cells.refreshAll();

    m_elapsedSeconds = 0;
    emit elapsedSecondsChanged();
    clearHighlight();
    selectFirstEmptyCell();
    clearSavedGame();
    setState(Playing);
    emit boardChanged();
}

void SudokuGame::resumeSavedGame() {
    if (!m_hasSavedGame)
        return;
    selectFirstEmptyCell();
    setState(Playing);
    emit boardChanged();
}

void SudokuGame::select(int index) {
    if (index < -1 || index >= Sudoku::kCells || index == m_selectedIndex)
        return;
    m_selectedIndex = index;
    emit selectedIndexChanged();
    emit selectedValueChanged();
}

void SudokuGame::moveSelection(int deltaRow, int deltaColumn) {
    if (m_state != Playing)
        return;
    if (m_selectedIndex < 0) {
        select(0);
        return;
    }
    // Movement stops at the edges: wrapping around makes arrow keys feel lost.
    const int row = qBound(0, Sudoku::rowOf(m_selectedIndex) + deltaRow, Sudoku::kSize - 1);
    const int column = qBound(0, Sudoku::colOf(m_selectedIndex) + deltaColumn, Sudoku::kSize - 1);
    select(row * Sudoku::kSize + column);
}

void SudokuGame::enterValue(int digit) {
    if (m_state != Playing || m_selectedIndex < 0)
        return;
    applyChange(m_board.setValue(m_selectedIndex, digit));
}

void SudokuGame::toggleNote(int digit) {
    if (m_state != Playing || m_selectedIndex < 0)
        return;
    applyChange(m_board.toggleNote(m_selectedIndex, digit));
}

void SudokuGame::toggleHighlight(int digit) {
    setHighlightDigit(digit >= 1 && digit <= 9 && digit != m_highlightDigit ? digit : -1);
}

void SudokuGame::clearHighlight() {
    setHighlightDigit(-1);
}

void SudokuGame::pressPad(int digit) {
    // With no cell to write into, the pad can only light a digit up, whatever
    // the mode says.
    if (m_padMode == Highlight || m_state != Playing || m_selectedIndex < 0)
        toggleHighlight(digit);
    else if (m_padMode == Note)
        toggleNote(digit);
    else
        enterValue(digit);
}

void SudokuGame::erase() {
    if (m_state != Playing || m_selectedIndex < 0)
        return;
    applyChange(m_board.erase(m_selectedIndex));
}

void SudokuGame::undo() {
    if (m_state != Playing)
        return;
    applyChange(m_board.undo());
}

void SudokuGame::restart() {
    if (m_state != Playing)
        return;
    applyChange(m_board.restart());
}

void SudokuGame::backToStart() {
    clearHighlight();
    if (inProgress())
        saveGame();
    m_clock.stop();
    setState(Start);
}

void SudokuGame::applyChange(const std::vector<int> &changed) {
    if (changed.empty())
        return;
    // Deferred checking can light up cells far from the edited one, so repaint
    // everything unless we know only these cells changed.
    if (m_board.checkAsYouGo())
        m_cells.refresh(changed);
    else
        m_cells.refreshAll();
    emit boardChanged();
    emit selectedValueChanged();

    if (m_board.isSolved()) {
        m_clock.stop();
        clearSavedGame();
        setState(Won);
        return;
    }
    m_saveTimer.start();
}

void SudokuGame::setState(State state) {
    if (m_state == state)
        return;
    m_state = state;
    // The clock only runs while a puzzle is actually on screen.
    if (m_state == Playing)
        m_clock.start();
    else
        m_clock.stop();
    emit stateChanged();
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
    const QSettings settings;
    m_board.setCheckAsYouGo(settings.value(kCheckKey, true).toBool());
    const int padMode = settings.value(kPadModeKey, int(Fill)).toInt();
    m_padMode = padMode >= int(Highlight) && padMode <= int(Fill) ? PadMode(padMode) : Fill;

    const QJsonObject json =
        QJsonDocument::fromJson(settings.value(kStateKey).toString().toUtf8()).object();
    if (json.isEmpty() || !SudokuBoard::fromJson(json, &m_board))
        return;
    m_elapsedSeconds = json.value(kElapsedKey).toInt(0);
    // A finished puzzle is not worth resuming.
    if (m_board.isSolved()) {
        clearSavedGame();
        return;
    }
    m_cells.refreshAll();
    setHasSavedGame(true);
}

void SudokuGame::saveGame() {
    QJsonObject json = m_board.toJson();
    json.insert(kElapsedKey, m_elapsedSeconds);
    QSettings().setValue(kStateKey, QString::fromUtf8(QJsonDocument(json).toJson(QJsonDocument::Compact)));
    setHasSavedGame(true);
}

void SudokuGame::clearSavedGame() {
    QSettings().remove(kStateKey);
    setHasSavedGame(false);
}

QVariantMap SudokuGame::windowGeometry() const {
    const QSettings settings;
    const QRect geometry = settings.value(kGeometryKey).toRect();
    QVariantMap map;
    // Positions can legitimately be negative on monitors left of or above the
    // primary, so validity travels separately instead of being encoded as -1.
    map.insert(QStringLiteral("valid"), geometry.isValid());
    map.insert(QStringLiteral("x"), geometry.x());
    map.insert(QStringLiteral("y"), geometry.y());
    map.insert(QStringLiteral("width"), geometry.width());
    map.insert(QStringLiteral("height"), geometry.height());
    map.insert(QStringLiteral("maximized"), settings.value(kMaximizedKey, false).toBool());
    return map;
}

void SudokuGame::saveWindowGeometry(int x, int y, int width, int height, bool maximized) {
    QSettings settings;
    settings.setValue(kGeometryKey, QRect(x, y, width, height));
    settings.setValue(kMaximizedKey, maximized);
}
