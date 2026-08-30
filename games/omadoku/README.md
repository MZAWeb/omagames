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
  digit clears that cell's notes and retracts the digit from the notes of every
  cell in the same row, column and box. Notes are never checked.
- **Mode** (the segmented control under the keypad in the rail) decides what a digit does,
  typed or clicked: *Highlight*, *Note* or *Fill*. `N` cycles it and it is
  remembered between sessions; Highlight is the default. Holding a modifier
  overrides it for one press without switching mode, which is what each
  segment's badge spells out: `Alt+1-9` highlights, `Shift+1-9` notes,
  `Ctrl+1-9` fills — and with no cell selected any digit just highlights.
- **Highlighting** lights up every cell holding that digit, givens included.
  The same digit again (or `Escape`) clears it, another digit switches it.
- An unfinished puzzle is saved automatically and offered as *Resume* the next
  time you start the game.

### Keys

| Key | Action |
| --- | --- |
| Arrows or `h` `j` `k` `l` | move the selection |
| `1`–`9` | do the selected mode's action with that digit |
| `Alt+1`–`Alt+9` | highlight every cell holding that digit |
| `Shift+1`–`Shift+9` | toggle the digit as a note in the selected cell |
| `Ctrl+1`–`Ctrl+9` | fill the digit into the selected cell |
| `N` | cycle the mode: Highlight → Note → Fill |
| `Backspace`, `Delete`, `0` | erase the cell |
| `Ctrl+Z` | undo |
| `R` | restart the puzzle |
| `V` | toggle *Validate as I go* |
| `Escape` | clear the highlight, else back to the start screen (confirms mid-puzzle) |
| `Ctrl+Q` | quit |

On the start screen `1`, `2`, `3`, `4` pick a difficulty and `R` resumes.
Every button shows its key, so nothing above needs memorising.

## Layout

The board is the hero: square, centred, and given every pixel the controls do
not need. Everything else lives in one rail, always in the same order — the
3×3 keypad, the mode selector, *Validate as I go*, then Erase, Undo, Restart
and New game, with a line of keyboard reminders at the foot.

The rail sits beside the board on the right, and folds into a bottom sheet
under it when that leaves the board bigger — which is what a tall, narrow
window or a large desktop text scale amounts to. The controls keep their order
across the switch; only their number of columns changes, so nothing a hand has
learnt moves. Feedback stays local to the cell that changed: a placement
settles, retracted pencil marks fade where they stood, and a wrong entry marks
its own corner instead of the board reacting as a whole.

The window is at least 700×560 logical pixels, both multiplied by the desktop
text scale (so 1400×1120 at 200%). It opens at 880×640 the first time and
remembers its geometry afterwards.

## Difficulty

Every level is defined by the human solving techniques its puzzles need, and
the generator guarantees the claim both ways: a puzzle at a level is fully
solvable with that level's techniques and **not** solvable with the level
below's, so each level really asks for something new. The bands follow the
grading most apps and graders agree on: two singles-only levels that differ
in how hard the scanning is, then a level where pencil marks become
necessary, then one for the pattern techniques. Only techniques a casual
player recognises are used, named as in the sudoku.coach lessons
(<https://sudoku.coach/en/learn>); locked candidates (pointing pairs,
claiming) are deliberately not part of the ladder, so no puzzle ever needs
them. The start screen lists the techniques under each level; the board
header shows the hardest one the current puzzle needs.

| Level | Techniques it adds | Feel | Typical clues |
| --- | --- | --- | --- |
| Easy | naked single | some cell always has one digit left; no pencil marks | 38–42 |
| Medium | hidden single | singles only, but you must scan each unit for where a digit has to go | 25–36 |
| Hard | naked pair, hidden pair, naked triple | pencil marks and eliminations | 21–30 |
| Extra hard | X-wing, XY-wing, swordfish | at least one pattern technique somewhere | 23–28 |

Clue counts are a target the generator aims for, not a promise: the
technique requirement always wins. A Medium is never solvable with naked
singles alone, a Hard never with singles alone, an Extra hard never without
a fish or an XY-wing.

### How grading works

The grader is a human-technique solver that never guesses. It climbs a fixed
ladder — naked single, hidden single, naked pair, hidden pair, naked
triple, X-wing, XY-wing, swordfish — always trying the easiest
rung first and starting over from the bottom after every step, exactly as a
player would. The hardest rung it has to climb is the puzzle's grade. A grid
it cannot finish is beyond the ladder (that is where chains, uniqueness
arguments and the like would begin) and is never handed out.

The generator carves clues out of a random complete grid, removing one only
while the puzzle stays unique and the ladder can still finish it within the
level's ceiling. It carves past the clue target until the level below can no
longer solve the puzzle, and when a removal would jump straight over a
narrow band it puts a different clue back to land inside it. Each level's
promise is checked in the test suite over fixed seeds, and every technique
has a test on a crafted position where nothing easier makes progress.
Generation is instant for Easy and Medium and takes a few milliseconds for
Hard and Extra hard (worst case around 50 ms) on a laptop.

## Build and install

```sh
curl -fsSL https://raw.githubusercontent.com/MZAWeb/omagames/main/install.sh | bash -s omadoku   # install
bin/build omadoku     # build
bin/test omadoku      # run the test suite
bin/run omadoku       # play
bin/install omadoku   # build + install the Arch package from a checkout
```

Settings and the saved puzzle live in `~/.config/Omacom/omadoku.conf`.
