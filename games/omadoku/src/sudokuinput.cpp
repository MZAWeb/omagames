#include "sudokuinput.h"

namespace {

// Ids shared with QML. They are stable identifiers, never shown to the player.
const auto kHighlightId = QStringLiteral("highlight");
const auto kNoteId = QStringLiteral("note");
const auto kFillId = QStringLiteral("fill");

// Highlighter yellow, and an ink dark enough to stay readable on it. The one
// place in the game that is deliberately deaf to the Omarchy theme.
constexpr QColor kHighlightColor(0xff, 0xf0, 0x2b);
constexpr QColor kHighlightInk(0x1a, 0x1a, 0x14);

}  // namespace

QString SudokuInput::clickMode() const {
    switch (m_clickMode) {
    case Action::Highlight:
        return kHighlightId;
    case Action::Note:
        return kNoteId;
    case Action::Fill:
        break;
    }
    return kFillId;
}

bool SudokuInput::setClickMode(const QString &id) {
    const Action mode = id == kHighlightId ? Action::Highlight
        : id == kNoteId                    ? Action::Note
                                           : Action::Fill;
    if (m_clickMode == mode)
        return false;
    m_clickMode = mode;
    return true;
}

QString SudokuInput::nextClickMode() const {
    switch (m_clickMode) {
    case Action::Highlight:
        return kNoteId;
    case Action::Note:
        return kFillId;
    case Action::Fill:
        break;
    }
    return kHighlightId;
}

SudokuInput::Action SudokuInput::keyAction(Qt::KeyboardModifiers modifiers) {
    if (modifiers & (Qt::ControlModifier | Qt::AltModifier))
        return Action::Highlight;
    if (modifiers & Qt::ShiftModifier)
        return Action::Note;
    return Action::Fill;
}

SudokuInput::Action SudokuInput::actionFor(Action action, int selectedCells, bool ownNotes) {
    return ownNotes && action == Action::Fill && selectedCells > 1 ? Action::Note : action;
}

bool SudokuInput::toggleHighlight(int digit) {
    return setHighlightDigit(digit >= 1 && digit <= 9 && digit != m_highlightDigit ? digit : -1);
}

bool SudokuInput::setHighlightDigit(int digit) {
    if (m_highlightDigit == digit)
        return false;
    m_highlightDigit = digit;
    return true;
}

QColor SudokuInput::highlightColor() {
    return kHighlightColor;
}

QColor SudokuInput::highlightInk() {
    return kHighlightInk;
}
