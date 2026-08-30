# Omatris

Tetris for Omarchy, built to the modern guideline so it feels the way anyone
who has played Tetris expects: ten columns, twenty visible rows, the seven
tetrominoes under the **Super Rotation System** with the standard wall kicks,
a 7-bag randomiser, a next queue, hold, a ghost piece, lock delay, DAS, and
guideline scoring with back-to-back, combos and T-spins.

## Rules

- The well is **10 × 20**, with four rows hidden above it where pieces enter
  and where a wall kick has room to lift one.
- Pieces are dealt from a **7-bag**: each bag is the seven tetrominoes
  shuffled, so nothing repeats inside a bag and no piece is ever more than
  twelve away. The next **three** are shown.
- Rotation is **SRS**: guideline spawn orientations, four rotation states, and
  the published wall-kick tables — including the I piece's own table — tried
  in order until one fits. The O piece never moves when it turns.
- **Hold** parks the falling piece and brings back whatever was parked, in its
  spawn orientation. Once per piece: it comes back when the next piece locks.
- The **ghost** outlines where the piece would land. `G` turns it off and on;
  the choice is remembered.
- Gravity follows the guideline curve: one row per second at level 1, and one
  row per `(0.8 − 0.007 × (level − 1)) ^ (level − 1)` seconds after that. It
  stops getting faster at level 20.
- **Soft drop** (`↓`) falls twenty times as fast and pays a point a row.
  **Hard drop** (`Space`) drops the piece to the floor and locks it at once,
  for two points a row.
- **Lock delay**: a piece that lands has half a second before it locks. That
  half second only runs while the piece is resting on something — a rotation
  that lifts it off the stack pauses the timer instead of rewinding it — and
  every move or rotation made after it has landed restarts it, **twice** at
  most. (The guideline allows fifteen; two is enough to place a piece and few
  enough that hammering the keys cannot keep one alive forever.) Falling to a
  row it has not reached before hands it a fresh pair; a kick that bounces it
  down and back up does not.
- Full rows flash for 150 ms and then everything above them falls.
- **Level** goes up every **10 lines**.
- The game ends on **block out** (a new piece has nowhere to appear) or
  **lock out** (a piece comes to rest entirely above the visible well).

### T-spins

A T counts as spun when its last move was a rotation and at least **three of
the four corners** of its three-by-three box are filled (the walls and the
floor count). It is a **full** T-spin when both corners on the side the T
points at are filled, or when the rotation only fitted on the last kick of the
table; otherwise it is a **mini**.

## Scoring

Every line below is multiplied by the level.

| What | Points |
|---|---|
| Single / Double / Triple / Tetris | 100 / 300 / 500 / 800 |
| T-spin mini, no lines / single / double | 100 / 200 / 400 |
| T-spin, no lines / single / double / triple | 400 / 800 / 1200 / 1600 |
| Back-to-back (a Tetris or T-spin clear straight after another) | ×1.5 |
| Combo (each placement in a row that clears, after the first) | +50 × combo |
| Soft drop / hard drop | 1 / 2 per row |

A back-to-back chain survives a placement that clears nothing; it is broken by
a line clear that is neither a Tetris nor a T-spin. A combo is broken by any
placement that clears nothing. Every constant is named in `src/rules.h`.

## Modes

| | Goal | Gravity | Ranked on |
|---|---|---|---|
| **Marathon** | endless | ramps with the level | score |
| **Sprint** | 40 lines | stays at level 1 | the clock |
| **Zen** | endless | stays at level 1 | score |

Sprint and Zen still gain a level every ten lines — the level pays out in the
score — but neither speeds up: Sprint is a race the stack should not win, and
Zen is somewhere to stack for as long as you like. A Sprint that tops out
never crossed the line, so it leaves no time behind.

Each mode has its own key and none of them is a default, so no button on the
start screen is drawn as the primary one; the mode played last is merely
marked. The top ten per mode (with lines, level and date) are kept in
`~/.config/Omacom/omatris.conf`, and the header shows the best for the mode in
play.

## Keyboard

Everything is reachable without a mouse; each button shows its key as a badge.
There is nothing to click during play.

| Key | Action |
|---|---|
| `←` `→` | Move. Held down, the piece waits ~167 ms and then steps every ~33 ms (delayed auto shift) |
| `↓` | Soft drop |
| `Space` | Hard drop |
| `↑` or `X` | Rotate clockwise |
| `Z` | Rotate counter-clockwise |
| `C` | Hold |
| `G` | Ghost piece on / off (remembered between runs) |
| `P` | Pause / resume |
| `R` | Restart the run at once (no prompt — it is the retry key) |
| `1` `2` `3` | Start Marathon / Sprint / Zen (start screen) |
| `H` | High scores (start screen) |
| `Enter` / `Space` | Play again after a run ends |
| `Esc` | Leave the game (confirmed mid-game), close the high scores |
| `Y` / `Enter`, `N` / `Esc` | Confirm / cancel a dialog |
| `Ctrl+Q` | Quit |

## Layout

The well is painted by one `FieldView` item at a whole number of pixels per
cell, with the hold box on its left and the next queue on its right, both four
cells wide. The header carries the mode, the best result, the back-to-back and
combo badges, and the numbers that matter for the mode: score, level and lines
in Marathon and Zen, lines left and the clock in Sprint. The window scales with
the desktop text size; the minimum is 660 × 560 logical pixels at 100%.

## Build, test, run

```sh
bin/build omatris
bin/test omatris
bin/run omatris
```

Settings live in `~/.config/Omacom/omatris.conf`.

## Install (Arch)

```sh
curl -fsSL https://raw.githubusercontent.com/MZAWeb/omagames/main/install.sh | bash -s omatris
```

or from a checkout, `bin/install omatris`.
