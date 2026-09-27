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
- **Soft drop** (`↓`) falls twenty times as fast by default (see Handling)
  and pays a point a row.
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
| **Challenge** | clear the dealt stack | stays at level 1 | not ranked |

Sprint and Zen still gain a level every ten lines — the level pays out in the
score — but neither speeds up: Sprint is a race the stack should not win, and
Zen is somewhere to stack for as long as you like. A Sprint that tops out
never crossed the line, so it leaves no time behind.

**Challenge** is Zen with a mess to clean up first. The run opens on a stack
covering 4 to 10 of the 20 visible rows (20–50%), and it ends, won, the moment
the last of those rows is cleared; topping out ends it lost. Lines built above
the mess count for the score and the level but not for the goal. The stack
is dealt from the run's seed but never random noise: it is whole
tetrominoes, in bag order, each dropped straight down from the top of the
well where a careless player would have put it — the lowest of a few poor
spots — until 55–70% of the covered rows are filled. So nothing floats, the
colours are the pieces' own, there are holes and overhangs, and no row is
full. The rows still to clear are tinted in the well, and the header counts
them (`Rows left 3 / 7`) beside the clock.

Under the hold box a big **Difficulty** number, 1 to 100, says how hard the
board left is to finish. It is rated on the deal and again every time a piece
settles (after its lines clear), with an arrow for how far it has moved since
the deal, and runs green through yellow to red. It estimates the work left,
not how tidy the board looks. The rows that have to go are the dealt rows,
plus any row with a block over an empty cell of one of those, since it must
be cleared before that cell can be filled. The work is the empty cells across
those rows, a piece per four, plus extra for each that is buried under
something, and a steep charge once the stack passes row 12. So building on
top of the mess clear of its gaps leaves the number where it was, filling a
gap lowers it, and sealing one under a misplaced piece makes it jump. It is
linear in that work, so a fresh deal rates about 10–70, a mistake shows as a
step of several points, and the last dealt row cleared rates 1. It is a
heuristic, not a solver; the costs are named in `src/difficulty.cpp`. How a Challenge goes depends on
the stack it dealt, so it keeps no high-score table; `R` deals a new one.

Each mode has its own key and none of them is a default, so no button on the
start screen is drawn as the primary one; the mode played last is merely
marked. The top ten per ranked mode (with lines, level and date) are kept in
`~/.config/Omacom/omatris.conf`, and the header shows the best for the mode in
play.

## Handling

How the keys feel is the player's to tune, from the start screen or the pause
overlay (`S`). `↑` `↓` pick a setting, `←` `→` change it, and changes apply at
once, mid-run included:

| Setting | Default | Range |
|---|---|---|
| **Auto-shift delay** (DAS): how long `←` / `→` is held before the piece starts sliding | 167 ms | 17–333 ms |
| **Auto-repeat rate** (ARR): the gap between cells once it slides | 33 ms | Instant, 17–167 ms |
| **Soft drop speed**: how much faster `↓` falls than gravity | 20× | 5×, 10×, 20×, 40×, Instant |

Times move in steps of one tick (1/60 s), the only unit the game counts in.
An **instant** repeat takes the piece straight to the wall once the delay is
up, and that slide spends one lock-delay reset, not one per cell. An
**instant** soft drop reaches the floor in a single tick without locking, so
the piece can still be slid or turned there; it pays a point a row like any
soft drop.

The choice is kept between launches (under `handling/v1`), and a value moved
off its default is shown in the accent color. **Reset to defaults** (`D`)
puts all three back to the values above and forgets the stored choice, so a
player who resets follows the defaults from then on. The lock delay and its
two resets are rules, not handling, and are not in the panel: they shape how
hard the game is, and every run on the high-score tables is played under the
same ones.

## Keyboard

Everything is reachable without a mouse; each button shows its key as a badge.
There is nothing to click during play.

| Key | Action |
|---|---|
| `←` `→` | Move. Held down, the piece waits ~167 ms and then steps every ~33 ms (delayed auto shift; see Handling) |
| `↓` | Soft drop |
| `Space` | Hard drop |
| `↑` or `X` | Rotate clockwise |
| `Z` | Rotate counter-clockwise |
| `C` | Hold |
| `G` | Ghost piece on / off (remembered between runs) |
| `P` | Pause / resume |
| `R` | Restart the run at once (no prompt — it is the retry key) |
| `1` `2` `3` `4` | Start Marathon / Sprint / Zen / Challenge (start screen) |
| `H` | High scores (start screen) |
| `S` | Handling (start screen, pause overlay); inside it `↑` `↓` choose, `←` `→` change, `D` resets to defaults, `Esc` / `Enter` / `S` close |
| `Enter` / `Space` | Play again after a run ends |
| `Esc` | Leave the game (confirmed mid-game), close the high scores |
| `Y` / `Enter`, `N` / `Esc` | Confirm / cancel a dialog |
| `Ctrl+Q` | Quit |

## Layout

The well is painted by one `FieldView` item at a whole number of pixels per
cell, with the hold box on its left and the next queue on its right, both four
cells wide. The header carries the mode, the best result, the back-to-back and
combo badges, and the numbers that matter for the mode: score, level and lines
in Marathon and Zen, lines left and the clock in Sprint, rows left and the
clock in Challenge. The window scales with the desktop text size; the
minimum is 660 × 560 logical pixels at 100%, where the start screen scrolls
rather than clip.

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
