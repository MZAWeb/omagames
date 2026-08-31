# Omadoku

Sudoku for Omarchy: pick Easy, Medium, Hard or Extra hard and a puzzle with
exactly one solution is generated on the spot, graded by the solving
techniques it actually needs. Colors, dark mode and text scale follow the
current Omarchy theme.

## Playing

- **Validate as I go** (the `Validate` toggle in the rail, remembered between
  sessions): wrong entries turn red, with a small marker in the corner of the
  cell, as soon as you make them. It starts **off** — a first puzzle should not
  correct you before you have finished thinking — so without it you are told
  only once all 81 cells are filled; flip it mid-game and the marks appear or
  vanish immediately.
- **Notes** are pencil marks: up to nine small digits per empty cell. Placing a
  digit clears that cell's own notes and nothing else: the marks you left in the
  rest of the row, column and box are your bookkeeping to keep or tidy. Notes
  are never checked.
- **Auto-notes** (the `Auto-notes` toggle beside `Validate`, `A`, remembered
  between sessions and off by default): while it is on the board keeps the
  pencil. Every empty cell shows exactly the digits its row, column and box
  still leave open, and every entry, erase and undo is reflected at once —
  there is nothing to tidy and nothing to forget. It is deliberately not a
  hint: it repeats what is on the grid and never looks at the solution.
  Your own marks are set aside, not thrown away, and come back the moment you
  switch it off. While it is on nothing can pencil: `Shift`+a digit does
  nothing, the keypad's *Note* mode is greyed out (switching Auto-notes on
  hands a keypad set to it back to *Fill*), and a plain digit fills the cursor
  cell even with several cells selected.
- **The keyboard mapping is fixed** and never depends on anything on screen:
  `1`-`9` fill, `Shift+1`-`9` note, `Ctrl+1`-`9` (or `Alt+1`-`9`) highlight. The
  line under the keypad says so at all times, and says it differently in the
  two cases that bend it: a multi-cell selection, where a plain digit notes
  too, and Auto-notes, where nothing pencils at all.
- **Mouse clicks** (the segmented control under that line) is the one thing the
  *Highlight* / *Note* / *Fill* selector decides: what clicking a key on the
  keypad does. It changes nothing about the keyboard. `N` cycles it and it is
  remembered between sessions; Fill is the default.
- **Selecting several cells** is for pencilling: `Ctrl+click` picks cells out
  one at a time, `Shift`+arrows (or `Shift+h` `j` `k` `l`) sweeps a run, taking
  every cell the cursor crosses. A digit then pencils it into all of them at
  once — with or without `Shift`, since a value belongs to one cell and cannot
  be what you meant across a sweep — cells that already hold a value are
  skipped, and one `Ctrl+Z` takes the whole lot back. The selection answers as
  one: as long as an empty cell in it is missing that note the digit goes into
  all of them, and only once every one carries it does the next press take it
  out everywhere — so a half-pencilled selection converges instead of flipping
  cell by cell. Over a single cell it is the plain toggle it always was, and a
  plain digit fills the cursor cell and folds the selection back onto it; so
  does a plain arrow or a plain click. The cursor is outlined more strongly
  than the rest of the selection.
- **Highlighting** lights up every cell holding that digit, givens included,
  and the key on the keypad with it. The same digit again (or `Escape`) clears
  it, another digit switches it. The cursor and the selection do not put it
  out: a lit cell they land on keeps the yellow, a shade toned down, and wears
  the selection outline on top. It is the one thing in the game that does not
  follow the Omarchy theme: a highlighter pen is highlighter yellow, with dark
  ink on top, whatever the desktop looks like.
- **Best times**: solving a puzzle files the clock, with the day, in that
  level's top five, and the won screen says where it landed (*New best!*, or
  *#3 for Hard*). The start screen shows the fastest under each level and
  keeps the full tables behind *Best times* (`B`). Restarting or leaving a
  puzzle files nothing.
- An unfinished puzzle is saved automatically and offered as *Resume* the next
  time you start the game.

### Keys

| Key | Action |
| --- | --- |
| Arrows or `h` `j` `k` `l` | move the cursor, collapsing any multi-cell selection |
| `Shift`+arrows or `Shift+h` `j` `k` `l` | extend the selection along the way |
| `Ctrl`+click | add a cell to the selection, or take it out |
| `1`–`9` | fill the digit into the cursor cell — over a multi-cell selection it notes instead, unless Auto-notes is on |
| `Shift+1`–`Shift+9` | note the digit in every selected empty cell — clearing it from all only when all of them have it |
| `Ctrl+1`–`Ctrl+9` (or `Alt+1`–`Alt+9`) | highlight every cell holding that digit |
| `N` | cycle what a keypad click does: Highlight → Note → Fill |
| `Backspace`, `Delete`, `0` | erase the cell |
| `Ctrl+Z` | undo |
| `R` | restart the puzzle: clears every entry and note (confirms once you have any) |
| `V` | toggle *Validate as I go* |
| `A` | toggle *Auto-notes*: the board pencils every empty cell with what still fits |
| `Escape` | collapse the selection, else clear the highlight, else back to the start screen (confirms mid-puzzle) |
| `Ctrl+Q` | quit |

On the start screen `1`, `2`, `3`, `4` pick a difficulty, `R` resumes and `B`
shows the best times (`Escape` closes them).
Every button shows its key, so nothing above needs memorising.

## Layout

The board is the hero: square, centred, and given every pixel the controls do
not need. Everything else lives in one rail, always in the same order — the
3×3 keypad, the line stating the keyboard mapping, the *Mouse clicks*
selector, *Validate as I go* and *Auto-notes*, then Erase, Undo, Restart and
New game, with a line of keyboard reminders at the foot.

The rail sits beside the board on the right, and folds into a bottom sheet
under it when that leaves the board bigger — which is what a tall, narrow
window or a large desktop text scale amounts to. The controls keep their order
across the switch; only their number of columns changes, so nothing a hand has
learnt moves. Feedback stays local to the cell that changed: a placement
settles and a wrong entry marks its own corner, instead of the board reacting
as a whole.

The window is at least 700×560 logical pixels, both multiplied by the desktop
text scale (so 1400×1120 at 200%). It opens at 880×640 the first time and
remembers its geometry afterwards.

## Difficulty

Every level is defined by the human solving techniques its puzzles need, and
the generator guarantees the claim both ways: a puzzle at a level is fully
solvable with that level's techniques and **not** solvable with the level
below's, so each level really asks for something new. The techniques are
the ones sudoku.coach teaches (<https://sudoku.coach/en/learn>), under the
names it uses, and the bands are cut along the Sudoku Explainer (SE) rating
sudoku.coach grades by (<https://sudoku.coach/en/learn/sudoku-difficulty>):
its categories are Easy up to 1.2, Medium up to 2.0, Hard up to 3.0 and
Advanced above 4.0. Locked candidates (pointing, claiming) are deliberately
not part of the ladder, so no puzzle ever needs them. The start screen
lists the techniques under each level; the board header shows the hardest
one the current puzzle needs.

| Level | Techniques it adds (SE rating) | Feel | Typical clues |
| --- | --- | --- | --- |
| Easy | last digit (1.0), hidden single in a box (1.2), naked single (2.3) | a box scan or a cell with one digit left always moves you on; never a row or column scan | 38–42 |
| Medium | hidden single in a row or column (1.5), naked pair (3.0) | fewer clues; scan lines too, and now and then a pair needs pencil marks | 22–35 |
| Hard | hidden pair (3.4), naked triple (3.6), hidden triple (4.0) | pencil marks and eliminations throughout | 21–30 |
| Extra hard | X-wing (3.2), swordfish (3.8), XY-wing (4.2) | at least one pattern technique somewhere | 23–28 |

Clue counts are a target the generator aims for, not a promise: the
technique requirement always wins. An Easy is never solved by hunting rows
and columns, a Medium never with Easy's techniques, a Hard never without a
hidden pair or a triple, an Extra hard never without a fish or an XY-wing.

### How grading works

The grader is a human-technique solver that never guesses. It climbs a fixed
ladder — last digit, hidden single (box), naked single, hidden single
(line), naked pair, hidden pair, naked triple, hidden triple, X-wing,
swordfish, XY-wing — always trying the easiest rung first and starting over
from the bottom after every step, exactly as a player would. The hardest
rung it has to climb is the puzzle's grade. A grid it cannot finish is
beyond the ladder (that is where chains, uniqueness arguments and the like
would begin) and is never handed out.

The ladder follows the SE ratings above with two deliberate deviations,
both there so that every level is a prefix of the ladder: the naked single
(2.3) sits ahead of the line hidden single (1.5), because Easy includes the
former and must never need the latter; and the X-wing (3.2) sits after the
triples, because the fish belong to Extra hard rather than to Hard. The
last digit is SE's "last value": the one empty cell left in a row, column
or box.

The generator carves clues out of a random complete grid, removing one only
while the puzzle stays unique and the ladder can still finish it within the
level's ceiling. It carves past the clue target until the level below can no
longer solve the puzzle, and when a removal would jump straight over a
narrow band it puts a different clue back to land inside it. Each level's
promise is checked in the test suite over fixed seeds, and every technique
has a test on a crafted position where nothing easier makes progress.
Generation is instant for Easy and Medium and takes some 10–15 ms on average
for Hard and Extra hard (worst case around 75 ms over 80 seeds) on a laptop.

## Build and install

```sh
curl -fsSL https://raw.githubusercontent.com/MZAWeb/omagames/main/install.sh | bash -s omadoku   # install
bin/build omadoku     # build
bin/test omadoku      # run the test suite
bin/run omadoku       # play
bin/install omadoku   # build + install the Arch package from a checkout
```

Settings, the best times and the saved puzzle live in
`~/.config/Omacom/omadoku.conf`.
