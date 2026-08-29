# Task A — Implement Omadoku (Sudoku)

You are implementing the game in `games/omadoku/`. Read `CLAUDE.md` and
`docs/PLAN.md` first. The skeleton (`omadoku.pro`, placeholder `main.cpp`,
`Main.qml`, `tests/`) already builds; replace the placeholders. Do not modify
`games/blackomack/`. You may add *generic* QML controls to `common/qml/OmaGames/`
(append to `qmldir` and `common.qrc`).

Done means: `bin/build omadoku` and `bin/test omadoku` pass, `bin/run omadoku`
shows a fully playable game, and every rule below is covered by a test.

## 1. Engine (QtCore only, no QObject required) — do this first

Files: `src/sudoku.h/.cpp` (grid types + solver), `src/sudokugenerator.h/.cpp`.

```cpp
using Grid = std::array<int, 81>;   // 0 = empty, row-major, index = row*9+col

namespace Sudoku {
    bool isValidPlacement(const Grid&, int index, int value); // no conflict in row/col/box
    bool isComplete(const Grid&);                             // all filled and valid
    // Backtracking solver. Counts solutions up to `limit` (2 is enough for
    // uniqueness checks). Optionally writes the first solution.
    int  countSolutions(const Grid&, int limit, Grid* firstSolution = nullptr);
    bool solve(Grid& inOut);
    // Cells whose current value conflicts with the given solution (for "check as I go").
    std::vector<int> wrongCells(const Grid& current, const Grid& solution);
}

enum class Difficulty { Easy, Medium, Hard };

struct Puzzle { Grid givens; Grid solution; Difficulty difficulty; quint32 seed; };

class SudokuGenerator {
public:
    // Deterministic for a given seed (tests rely on this); pass 0 for random.
    static Puzzle generate(Difficulty, quint32 seed = 0);
};
```

Generator algorithm (well known, no external library needed):
1. Fill an empty grid with randomized backtracking (shuffle candidates 1–9 per cell) → solution.
2. Remove cells one at a time in random order; after each removal run
   `countSolutions(grid, 2)`; if it becomes > 1 put the cell back.
3. Stop when the target clue count is reached: Easy 38–42 clues, Medium 30–34,
   Hard 24–28. If you cannot reach the target for a seed, accept what you have
   (still unique) — never loop forever. Use `QRandomGenerator` seeded explicitly.
   Generation must take < 1 s on a laptop for Hard (use bitmask candidates in the solver).

Nice-to-have (only if time allows, keep it separate): a simple technique
grader (naked singles / hidden singles) so "Hard" requires more than singles.

## 2. Game bridge — `src/sudokugame.h/.cpp` (QObject, exposed to QML as `game`)

State per cell: `given` (bool), `value` (0–9), `notes` (9-bit mask), `wrong` (bool).
Expose the board as a `QAbstractListModel` of 81 rows with those roles
(`CellModel`), or as invokable getters — the model is preferred for QML delegates.

Properties: `difficulty`, `state` (enum: Start, Playing, Won), `notesMode` (bool),
`checkAsYouGo` (bool, persisted setting, default true), `selectedIndex` (int, -1 = none),
`canUndo`, `elapsedSeconds` (optional), `filledCount`.

Invokables: `newGame(difficulty)`, `select(index)`, `moveSelection(dRow, dCol)`,
`enterDigit(d)` (writes value in entry mode, toggles note in notes mode;
ignored on givens), `erase()`, `toggleNotesMode()`, `undo()`, `restart()`
(clears all non-givens), `backToStart()`.

Rules to implement and test:
- Givens are immutable.
- Entering a value in a cell clears that cell's notes; entering a value also
  removes that digit from the notes of peers (row/col/box) — standard convenience.
- **Validation**: with `checkAsYouGo`, after each entry mark `wrong` on cells that
  differ from the solution (only entered values; notes are never validated).
  With it off, `wrong` stays false until all 81 cells are filled; then validate
  all entries at once. In both modes, when the grid matches the solution →
  `state = Won`.
- Undo stack of cell snapshots (value+notes) — at least 100 levels.
- Persistence (`QSettings`): `checkAsYouGo`, and the in-progress game
  (givens, solution, values, notes, difficulty) as JSON under `state/v1` so the
  puzzle resumes on next launch; `newGame` / win clears it. Window geometry too.

## 3. UI — QML, simple but polished, theme-only colors

Screens (one `StackView` or a state switch in `Main.qml`):
1. **Start**: title, three big `OmaButton`s (Easy / Medium / Hard), a
   "Check as I go" toggle (Switch), "Resume" if a saved game exists.
2. **Board**: 9×9 grid, thick 3×3 box borders using `theme.foreground` at
   different alphas; selected cell tinted with `theme.accent`, peers (same
   row/col/box) lightly tinted, same-digit cells highlighted; givens in bold
   `theme.foreground`, entries in `theme.accent`, wrong entries in `theme.red`;
   notes as a 3×3 mini-grid of small digits in `theme.muted`-ish.
   Below/right: digit pad 1–9 (each shows a subtle "done" state when all 9 placed),
   Erase, Notes toggle (clearly showing current mode), Undo, New game.
   Window should be resizable with the board scaling like omacalc's `uiScale`.
3. **Won** overlay: congratulations + difficulty + New game / Back.

Keyboard: arrows/hjkl move, 1–9 enter, `N` toggle notes, Backspace/Delete/0 erase,
`Ctrl+Z` undo, `Escape` back to start (confirm if in progress via a simple dialog),
`Ctrl+Q` quit. Mouse: click cell, click pad.

Keep visuals in small QML components (`SudokuCell.qml`, `DigitPad.qml`,
`StartScreen.qml`, …) so the design can be redone later without touching
`SudokuGame`.

## 4. Tests (`tests/tst_omadoku.cpp`, add sources to `tests/tests.pro`)
- Solver solves a known puzzle; `countSolutions` returns 1 / 2 on crafted grids.
- Generator: for each difficulty and a few fixed seeds, puzzle is unique-solution,
  clue count in range, deterministic per seed, all givens agree with solution.
- Game: givens immutable; notes toggle; entry clears notes and peer notes;
  check-as-you-go marks wrong cells; deferred mode marks nothing until full;
  win detection; undo restores value and notes; save/restore round-trips.

## 5. Finish
- Add `pkgbuild/omadoku.svg` (simple flat icon: rounded square, 3×3 grid glyph,
  same style as omawrite's icon).
- Update `games/omadoku/README.md` (short: what it is, keys, install).
- Report: what's done, what's not, how you verified (paste `bin/test omadoku` totals).
