#pragma once

#include <QColor>
#include <QString>
#include <Qt>

// What a digit means when it arrives, and the digit the player asked to see
// everywhere on the board.
//
// The two ways in are deliberately different: a click on the keypad does
// whatever the click mode says, while the number row is deaf to it, so no
// player has to look at a selector before typing. Neither path touches the
// board — this decides, the bridge acts — so the whole contract is testable
// headlessly.
class SudokuInput {
public:
    // What a digit does. Fill by default: writing digits is what a player does
    // most, and every other action has its own chord.
    enum class Action { Highlight, Note, Fill };

    // The click mode as QML knows it: "highlight" | "note" | "fill".
    QString clickMode() const;
    // An unknown id only reaches here from a stale setting or a stale QML
    // caller, so it means "no opinion" and lands on Fill. False when the mode
    // was already that one.
    bool setClickMode(const QString &id);
    // The mode after this one, so a single key can walk the three.
    QString nextClickMode() const;
    Action clickAction() const { return m_clickMode; }

    // The number row's fixed contract: Ctrl (or Alt) highlights, Shift notes,
    // plain fills.
    static Action keyAction(Qt::KeyboardModifiers modifiers);

    // The digit shown everywhere on the board, -1 for none. The same digit
    // again clears it, and anything that is not a digit clears it too.
    int highlightDigit() const { return m_highlightDigit; }
    bool toggleHighlight(int digit);
    bool clearHighlight() { return setHighlightDigit(-1); }

    // The single sanctioned exception to the theming rule (see CLAUDE.md): a
    // digit highlight is a highlighter pen, and a pen that changed color with
    // the desktop would stop reading as one. Fixed marker yellow, with the
    // dark ink that keeps a digit on top of it legible. Both live here rather
    // than in QML so no literal color is written into a .qml file.
    static QColor highlightColor();
    static QColor highlightInk();

private:
    bool setHighlightDigit(int digit);

    Action m_clickMode = Action::Fill;
    int m_highlightDigit = -1;
};
