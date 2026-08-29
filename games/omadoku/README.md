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
- **Highlighting**: pressing a bare digit lights up every cell holding it,
  givens included. Press it again (or `Escape`) to clear it, or another digit
  to switch. The digit pad does the same when no cell is selected.
- An unfinished puzzle is saved automatically and offered as *Resume* the next
  time you start the game.

### Keys

| Key | Action |
| --- | --- |
| Arrows or `h` `j` `k` `l` | move the selection |
| `1`–`9` | highlight every cell holding that digit |
| `Ctrl+1`–`Ctrl+9` | enter the digit in the selected cell |
| `Shift+1`–`Shift+9` | toggle the digit as a note in the selected cell |
| `N` | notes mode on/off (governs what the digit pad does) |
| `Backspace`, `Delete`, `0` | erase the cell |
| `Ctrl+Z` | undo |
| `R` | restart the puzzle |
| `Escape` | clear the highlight, else back to the start screen (confirms mid-puzzle) |
| `Ctrl+Q` | quit |

On the start screen `1`, `2`, `3` pick a difficulty, `R` resumes and `C`
toggles *Check as I go*. Every button shows its key, so nothing above needs
memorising.

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
