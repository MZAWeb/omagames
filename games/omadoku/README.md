# Omadoku

Sudoku for Omarchy: pick Easy, Medium or Hard and a puzzle with exactly one
solution is generated on the spot. Colors, dark mode and text scale follow the
current Omarchy theme.

## Playing

- **Check as I go** (start screen, remembered): wrong entries turn red as soon
  as you make them. Switch it off to be told only once all 81 cells are filled.
- **Notes** are pencil marks: up to nine small digits per empty cell. Placing a
  digit clears that cell's notes and retracts the digit from the notes of every
  cell in the same row, column and box. Notes are never checked.
- An unfinished puzzle is saved automatically and offered as *Resume* the next
  time you start the game.

### Keys

| Key | Action |
| --- | --- |
| Arrows or `h` `j` `k` `l` | move the selection |
| `1`–`9` | enter a digit (or toggle a note in notes mode) |
| `N` | notes mode on/off |
| `Backspace`, `Delete`, `0` | erase the cell |
| `Ctrl+Z` | undo |
| `Escape` | back to the start screen (confirms mid-puzzle) |
| `Ctrl+Q` | quit |

On the start screen `1`, `2`, `3` pick a difficulty and `R` resumes.

## Difficulty

| | Clues | Requires |
| --- | --- | --- |
| Easy | 38–42 | naked/hidden singles only |
| Medium | 30–34 | — |
| Hard | 24–28 | more than singles |

## Build and install

```sh
bin/build omadoku     # build
bin/test omadoku      # run the test suite
bin/run omadoku       # play
cd games/omadoku/pkgbuild && makepkg -si
```
