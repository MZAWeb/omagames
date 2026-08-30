# Omasweeper

Minesweeper for Omarchy, with one promise the original never made: **every
board can be finished by logic alone**. No 50/50s, no guessing, ever. Your
first click always opens a zero region, and from there each mine can be
proved.

## Rules

- Three presets: **Beginner** 9×9 with 10 mines, **Intermediate** 16×16 with
  40, **Expert** 30×16 with 99.
- Reveal a cell. A mine ends the game; a number tells how many of the eight
  cells around it are mines; a zero opens every cell around it, rippling
  outwards until numbers fence it in.
- Flag a hidden cell you have proved to be a mine; flag it again to clear
  the flag. Flags are for you: the game does not need them to be won.
- **Chord** on a revealed number that already has as many flags around it as
  its value: every other hidden neighbour opens at once. A wrong flag means
  one of them is a mine, and that is a loss.
- The game is won when every cell without a mine is revealed.
- The mine counter shows mines minus flags, and goes negative if you
  over-flag. The clock runs from your first reveal to the win or the loss.

## The no-guess promise

Mines are placed after the first click, keeping the 3×3 around it clear so
the opening is always a zero. The board is then played by a built-in logical
solver that only sees what you see: it uses satisfied and saturated numbers,
subset reasoning between overlapping numbers (the 1-2-1 family), and an
exact enumeration of every consistent mine layout along the frontier,
cross-checked against the mines still unaccounted for. If the solver gets
stuck, the layout is thrown away and another is drawn from the next seed,
until it finds one it can finish. Beginner and Intermediate boards need one
or two tries; Expert about six, still well under a hundredth of a second.

The solver has a bounded search budget and the generator a bounded number
of tries. Neither bound has been hit on any seed tested, but if either ever
is, the game says so rather than pretending.

That is what the badge in the header is for. **no guessing** means this
board was proved solvable before you saw it: every mine on it can be
deduced, and if you are stuck, you have missed something. **may need a
guess** means the generator ran out of tries and handed over the last layout
it drew, unproven — the board is perfectly playable, it just comes with no
promise, and somewhere on it there may be a position only luck gets past.

## Keyboard

Everything is reachable without a mouse; each button shows its key as a badge.

| Key | Action |
|---|---|
| `←` `↑` `↓` `→` or `H` `J` `K` `L` | Move the cursor |
| `Space` / `Enter` | Open the cell — or chord it, when the cursor is on a number whose flags already satisfy it |
| `F` | Flag / unflag |
| `R` | Restart: same preset, a new board |
| `1` `2` `3` | Start a Beginner / Intermediate / Expert board (start screen) |
| `H` | Best times (start screen) |
| `Enter` / `Space` | Play again after a win or a loss |
| `Esc` | Leave the board (confirmed while the clock is running), close the best times |
| `Y` / `Enter`, `N` / `Esc` | Confirm / cancel a dialog |
| `Ctrl+Q` | Quit |

## Mouse

The cursor belongs to the keyboard — hovering never moves it — but every
click plants it where you clicked, so the two never disagree.

| Button | Action |
|---|---|
| Left | Open the cell, or chord it when it is a satisfied number |
| Right | Flag / unflag |
| Middle, or left and right together | Chord |

Left and right are acted on when the button comes up, so pressing both
chords once instead of also opening or flagging.

## Best times

The five fastest wins per preset, with the day they happened, are kept in
`~/.config/Omacom/omasweeper.conf`. The start screen shows the best time
beside each preset, `H` opens all three tables, and the win overlay names
your rank when the run made the table. The last preset played is
preselected.

## Layout

The board is painted by one `FieldView` item at a whole number of pixels per
cell, centred in the window: hidden cells are raised lids, revealed ones lie
flat, numbers 1–8 each take their own theme color, and a freshly opened
cascade flips in from the cell you clicked over about a quarter of a second.
The header carries the preset, the no-guess badge, the mine counter and the
clock; the legend under the board names every key.

The window scales with the desktop text size. The minimum is 640×480 logical
pixels at 100%, which still gives an Expert board 20 pixels per cell; the
board never shrinks below 8 pixels per cell.

## Build, test, run

```sh
bin/build omasweeper
bin/test omasweeper
bin/run omasweeper
```

Settings live in `~/.config/Omacom/omasweeper.conf`.

## Install (Arch)

```sh
curl -fsSL https://raw.githubusercontent.com/MZAWeb/omagames/main/install.sh | bash -s omasweeper
```

or from a checkout, `bin/install omasweeper`.
