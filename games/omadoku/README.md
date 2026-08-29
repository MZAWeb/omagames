# Omadoku

Sudoku for Omarchy: pick Easy, Medium, Hard or Extra hard and a puzzle with
exactly one solution is generated on the spot, graded by the solving
techniques it actually needs. Colors, dark mode and text scale follow the
current Omarchy theme.

## Playing

- **Validate as I go** (the `Validate` toggle beside the board's actions,
  remembered between sessions): wrong entries turn red as soon as you make
  them. Switch it off to be told only once all 81 cells are filled; flip it
  mid-game and the marks appear or vanish immediately.
- **Notes** are pencil marks: up to nine small digits per empty cell. Placing a
  digit clears that cell's notes and retracts the digit from the notes of every
  cell in the same row, column and box. Notes are never checked.
- **Mode** (the segmented control under the keypad) decides what a digit does,
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

## Difficulty

Every level is defined by the human solving techniques its puzzles need, and
the generator guarantees the claim both ways: a puzzle at a level is fully
solvable with that level's techniques and **not** solvable with the level
below's, so each level really asks for something new. The start screen lists
the techniques under each level; the board header shows the hardest one the
current puzzle needs.

| Level | Techniques it adds | Typical clues |
| --- | --- | --- |
| Easy | naked single, hidden single | 38–42 |
| Medium | naked pair, hidden pair, pointing pair, claiming | 22–34 |
| Hard | naked triple, X-wing | 21–30 |
| Extra hard | Y-wing, swordfish | 23–28 |

Clue counts are a target the generator aims for, not a promise: the
technique requirement always wins.

### How grading works

The grader is a human-technique solver that never guesses. It climbs a fixed
ladder — naked single, hidden single, naked pair, hidden pair, pointing pair,
claiming, naked triple, X-wing, Y-wing, swordfish — always trying the easiest
rung first and starting over from the bottom after every step, exactly as a
player would. The hardest rung it has to climb is the puzzle's grade. A grid
it cannot finish is beyond the ladder (that is where chains, uniqueness
arguments and the like would begin) and is never handed out.

The generator carves clues out of a random complete grid, removing one only
while the puzzle stays unique and the ladder can still finish it within the
level's ceiling. It carves past the clue target until the level below can no
longer solve the puzzle, and when a removal would jump straight over a
narrow band (Hard's, mostly) it puts a different clue back to land inside
it. Each level's promise is checked in the test suite over fixed seeds, and
every technique has a test on a crafted position where nothing easier makes
progress. Generation takes well under a second on a laptop: Easy and Medium
are instant, Extra hard about 20 ms, Hard about 100–150 ms because its band
is the narrowest.

## Build and install

```sh
bin/build omadoku     # build
bin/test omadoku      # run the test suite
bin/run omadoku       # play
cd games/omadoku/pkgbuild && makepkg -si
```
