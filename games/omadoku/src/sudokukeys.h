#pragma once

#include <QtGlobal>

#include <Qt>

class QString;

// Reading a key press. A key event carries three different clues about which
// digit was meant and they disagree by keyboard layout, so the resolution lives
// here — plain, headless and unit-tested — rather than in QML, where getting it
// wrong once made a bare Shift type a 1.
namespace SudokuKeys {

// The digit 1-9 the press means, or 0 for anything else: modifier keys, the
// letters that move the cursor, and every key that simply is not a digit.
int digitFor(int key, Qt::KeyboardModifiers modifiers, const QString &text, quint32 nativeScanCode);

}  // namespace SudokuKeys
