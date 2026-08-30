#include "sudokukeys.h"

#include <QString>

namespace {

// The physical top row as X11 sends it: the last resort for layouts whose
// unshifted digits are letters or symbols we cannot name.
constexpr quint32 kFirstRowScanCode = 10;
constexpr quint32 kLastRowScanCode = 18;

// A modifier held down is a key press in its own right, carrying neither text
// nor a digit. It has to be turned away before any fallback looks at it.
bool isModifierKey(int key) {
    switch (key) {
    case Qt::Key_Shift:
    case Qt::Key_Control:
    case Qt::Key_Alt:
    case Qt::Key_AltGr:
    case Qt::Key_Meta:
    case Qt::Key_Super_L:
    case Qt::Key_Super_R:
    case Qt::Key_Hyper_L:
    case Qt::Key_Hyper_R:
    case Qt::Key_CapsLock:
    case Qt::Key_NumLock:
    case Qt::Key_ScrollLock:
    case Qt::Key_Mode_switch:
        return true;
    default:
        return false;
    }
}

// What a US layout sends for Shift+1 … Shift+9 when the event carries no text.
constexpr int kShiftedRowKeys[] = {Qt::Key_Exclam, Qt::Key_At, Qt::Key_NumberSign,
    Qt::Key_Dollar, Qt::Key_Percent, Qt::Key_AsciiCircum, Qt::Key_Ampersand,
    Qt::Key_Asterisk, Qt::Key_ParenLeft};

}  // namespace

int SudokuKeys::digitFor(int key, Qt::KeyboardModifiers modifiers, const QString &text,
                         quint32 nativeScanCode) {
    if (isModifierKey(key))
        return 0;
    if (key >= Qt::Key_1 && key <= Qt::Key_9)
        return key - Qt::Key_0;

    if (modifiers & Qt::ShiftModifier) {
        // US sends the symbol row with Shift; ask the character first, since a
        // single character is the one clue no layout can misreport. An empty
        // text is not a match — that is exactly what a bare Shift produces.
        if (text.size() == 1) {
            const int symbol = QStringLiteral("!@#$%^&*(").indexOf(text);
            if (symbol >= 0)
                return symbol + 1;
        }
        for (int i = 0; i < 9; ++i) {
            if (key == kShiftedRowKeys[i])
                return i + 1;
        }
    }

    if (nativeScanCode >= kFirstRowScanCode && nativeScanCode <= kLastRowScanCode)
        return int(nativeScanCode - kFirstRowScanCode) + 1;
    return 0;
}
