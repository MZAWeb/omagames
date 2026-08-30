#include "sudokukeys.h"

#include <QString>

namespace {

// What a US-style layout sends for Shift+1 … Shift+9: the symbol printed above
// the digit, rather than the digit itself.
constexpr int kShiftedRowKeys[] = {Qt::Key_Exclam, Qt::Key_At, Qt::Key_NumberSign,
    Qt::Key_Dollar, Qt::Key_Percent, Qt::Key_AsciiCircum, Qt::Key_Ampersand,
    Qt::Key_Asterisk, Qt::Key_ParenLeft};

}  // namespace

int SudokuKeys::digitFor(int key, Qt::KeyboardModifiers modifiers, const QString &text) {
    // Nine named keys and nine named characters, and nothing else: whatever a
    // press carries besides those, the answer is no. That is what keeps a
    // modifier, an arrow or a key Qt could not name from ever writing a digit
    // into the puzzle.
    if (key >= Qt::Key_1 && key <= Qt::Key_9)
        return key - Qt::Key_0;
    if (!(modifiers & Qt::ShiftModifier))
        return 0;

    // Ask the character first: a single character is the one clue no layout can
    // misreport. An empty text is not a match — that is what a bare modifier
    // produces, and matching it once made Shift on its own type a 1.
    if (text.size() == 1) {
        const int symbol = QStringLiteral("!@#$%^&*(").indexOf(text);
        if (symbol >= 0)
            return symbol + 1;
    }
    for (int i = 0; i < 9; ++i) {
        if (key == kShiftedRowKeys[i])
            return i + 1;
    }
    return 0;
}
