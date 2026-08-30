#pragma once

#include <Qt>

class QString;

// Reading a key press. Layouts disagree about what the digit row sends, so the
// resolution lives here — plain, headless and unit-tested — rather than in QML.
//
// It is deliberately blind to the hardware keycode. A keycode says how a key
// travelled, not what it is, and a synthetic keymap (wtype, ydotool, a
// remapper) is free to put an arrow on the codes a US keyboard uses for its
// digit row. Only what Qt says the key *is* may resolve to a digit.
namespace SudokuKeys {

// The digit 1-9 the press means, or 0 for anything else: modifier keys, the
// letters and arrows that move the cursor, and every key that is not a digit.
int digitFor(int key, Qt::KeyboardModifiers modifiers, const QString &text);

}  // namespace SudokuKeys
