#include "sudokugame.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QRect>
#include <QSettings>

#include "sudoku.h"
#include "sudokugrader.h"

namespace {

const auto kStateKey = QStringLiteral("state/v1");
// Still the key it was first stored under, so an existing preference survives
// the control's rename.
const auto kValidateKey = QStringLiteral("play/checkAsYouGo");
const auto kPadModeKey = QStringLiteral("play/padMode");
const auto kGeometryKey = QStringLiteral("window/geometry");
const auto kMaximizedKey = QStringLiteral("window/maximized");
const auto kElapsedKey = QStringLiteral("elapsed");

// Ids shared with QML. They are stable identifiers, never shown to the player:
// the labels beside them are.
const auto kStartId = QStringLiteral("start");
const auto kPlayingId = QStringLiteral("playing");
const auto kWonId = QStringLiteral("won");
const auto kHighlightId = QStringLiteral("highlight");
const auto kNoteId = QStringLiteral("note");
const auto kFillId = QStringLiteral("fill");
const auto kEasyId = QStringLiteral("easy");
const auto kMediumId = QStringLiteral("medium");
const auto kHardId = QStringLiteral("hard");
const auto kExtraHardId = QStringLiteral("extrahard");

QString idFor(Difficulty difficulty) {
    switch (difficulty) {
    case Difficulty::Easy:
        return kEasyId;
    case Difficulty::Medium:
        return kMediumId;
    case Difficulty::Hard:
        return kHardId;
    case Difficulty::ExtraHard:
        return kExtraHardId;
    }
    return kEasyId;
}

QString techniqueName(SudokuGrader::Technique technique) {
    using SudokuGrader::Technique;
    switch (technique) {
    case Technique::NakedSingle:
        return SudokuGame::tr("Naked single");
    case Technique::HiddenSingle:
        return SudokuGame::tr("Hidden single");
    case Technique::NakedPair:
        return SudokuGame::tr("Naked pair");
    case Technique::HiddenPair:
        return SudokuGame::tr("Hidden pair");
    case Technique::PointingPair:
        return SudokuGame::tr("Pointing pair");
    case Technique::Claiming:
        return SudokuGame::tr("Claiming");
    case Technique::NakedTriple:
        return SudokuGame::tr("Naked triple");
    case Technique::XWing:
        return SudokuGame::tr("X-wing");
    case Technique::YWing:
        return SudokuGame::tr("Y-wing");
    case Technique::Swordfish:
        break;
    }
    return SudokuGame::tr("Swordfish");
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
    return idFor(m_board.puzzle().difficulty);
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
        {Difficulty::Easy, tr("Easy"), tr("Singles only: every step is a digit with one place left.")},
        {Difficulty::Medium, tr("Medium"), tr("Adds pairs and box-line eliminations.")},
        {Difficulty::Hard, tr("Hard"), tr("Adds naked triples and X-wings.")},
        {Difficulty::ExtraHard, tr("Extra hard"), tr("Needs a Y-wing or a swordfish somewhere.")},
    };
    QVariantList list;
    for (const Level &level : levels) {
        list.append(QVariantMap {
            {QStringLiteral("id"), idFor(level.difficulty)},
            {QStringLiteral("label"), level.label},
            {QStringLiteral("techniques"), techniquesIntroducedBy(level.difficulty)},
            {QStringLiteral("description"), level.description},
        });
    }
    return list;
}

// Unknown ids only reach here from a stale setting or an empty override, so
// they mean "no opinion" rather than an error.
SudokuGame::PadMode SudokuGame::modeFromId(const QString &id, PadMode fallback) {
    if (id == kHighlightId)
        return PadMode::Highlight;
    if (id == kNoteId)
        return PadMode::Note;
    if (id == kFillId)
        return PadMode::Fill;
    return fallback;
}

QString SudokuGame::padMode() const {
    switch (m_padMode) {
    case PadMode::Highlight:
        return kHighlightId;
    case PadMode::Note:
        return kNoteId;
    case PadMode::Fill:
        break;
    }
    return kFillId;
}

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
    return m_screen == Screen::Playing && m_board.entryCount() > 0;
}

QVariantList SudokuGame::digitCounts() const {
    QVariantList counts;
    for (int digit = 1; digit <= 9; ++digit)
        counts.append(m_board.digitCount(digit));
    return counts;
}

void SudokuGame::setPadMode(const QString &padMode) {
    const PadMode mode = modeFromId(padMode, PadMode::Highlight);
    if (m_padMode == mode)
        return;
    m_padMode = mode;
    QSettings().setValue(kPadModeKey, this->padMode());
    emit padModeChanged();
}

void SudokuGame::cyclePadMode() {
    switch (m_padMode) {
    case PadMode::Highlight:
        setPadMode(kNoteId);
        break;
    case PadMode::Note:
        setPadMode(kFillId);
        break;
    case PadMode::Fill:
        setPadMode(kHighlightId);
        break;
    }
}

void SudokuGame::setValidateAsYouGo(bool validateAsYouGo) {
    if (m_board.validateAsYouGo() == validateAsYouGo)
        return;
    m_board.setValidateAsYouGo(validateAsYouGo);
    QSettings().setValue(kValidateKey, validateAsYouGo);
    m_cells.refreshAll();
    emit validateAsYouGoChanged();
    emit boardChanged();
}

void SudokuGame::newGame(const QString &difficulty) {
    // An unknown id can only come from a caller with a stale idea of the
    // levels; Easy is the safe landing.
    Difficulty level = Difficulty::Easy;
    for (int i = 0; i < kDifficultyCount; ++i) {
        if (idFor(Difficulty(i)) == difficulty)
            level = Difficulty(i);
    }
    m_board.setPuzzle(SudokuGenerator::generate(level));
    m_cells.refreshAll();

    m_elapsedSeconds = 0;
    emit elapsedSecondsChanged();
    clearHighlight();
    selectFirstEmptyCell();
    clearSavedGame();
    setScreen(Screen::Playing);
    emit boardChanged();
}

void SudokuGame::resumeSavedGame() {
    if (!m_hasSavedGame)
        return;
    selectFirstEmptyCell();
    setScreen(Screen::Playing);
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
    if (m_screen != Screen::Playing)
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
    if (m_screen != Screen::Playing || m_selectedIndex < 0)
        return;
    applyChange(m_board.setValue(m_selectedIndex, digit));
}

void SudokuGame::toggleNote(int digit) {
    if (m_screen != Screen::Playing || m_selectedIndex < 0)
        return;
    applyChange(m_board.toggleNote(m_selectedIndex, digit));
}

void SudokuGame::toggleHighlight(int digit) {
    setHighlightDigit(digit >= 1 && digit <= 9 && digit != m_highlightDigit ? digit : -1);
}

void SudokuGame::clearHighlight() {
    setHighlightDigit(-1);
}

void SudokuGame::pressDigit(int digit, const QString &overrideMode) {
    const PadMode mode = modeFromId(overrideMode, m_padMode);
    // With no cell to write into, a digit can only light itself up, whatever
    // the mode or the modifier asked for.
    if (mode == PadMode::Highlight || m_screen != Screen::Playing || m_selectedIndex < 0)
        toggleHighlight(digit);
    else if (mode == PadMode::Note)
        toggleNote(digit);
    else
        enterValue(digit);
}

void SudokuGame::erase() {
    if (m_screen != Screen::Playing || m_selectedIndex < 0)
        return;
    applyChange(m_board.erase(m_selectedIndex));
}

void SudokuGame::undo() {
    if (m_screen != Screen::Playing)
        return;
    applyChange(m_board.undo());
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
    emit selectedValueChanged();

    if (m_board.isSolved()) {
        m_clock.stop();
        clearSavedGame();
        setScreen(Screen::Won);
        return;
    }
    m_saveTimer.start();
}

void SudokuGame::setScreen(Screen screen) {
    if (m_screen == screen)
        return;
    m_screen = screen;
    // The clock only runs while a puzzle is actually on screen.
    if (m_screen == Screen::Playing)
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
    m_board.setValidateAsYouGo(settings.value(kValidateKey, true).toBool());
    // Anything unrecognised (including the int this key held before the modes
    // got names) falls back to the default.
    m_padMode = modeFromId(settings.value(kPadModeKey).toString(), PadMode::Highlight);

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
