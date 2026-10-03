# Omasnake

Snake for Omarchy. Steer a snake around a 32×24 grid, eat every dot that
appears, grow one cell longer each time and keep clear of the walls and of
your own tail. The longer you get the faster you go and the more each dot is
worth, so a long game is worth far more than a careful one.

## Rules

- The snake starts 4 cells long in the middle of the field, heading right,
  after a short "Ready" beat in which nothing moves.
- It moves **one cell per step**, always in its current heading; it never
  stops. The arrows (or `hjkl`) set the *next* heading: one turn is taken per
  step, a turn back onto its own neck is ignored, and up to **two** turns wait
  in a queue so a quick double tap round a corner lands both.
- Eating **food** grows the snake by one cell and scores. A new dot appears
  immediately, never under the snake and never under a bonus.
- Every 5th food also puts a **bonus** dot on the field, worth 50 and shown
  with a ring that shrinks as its 6 seconds run out. It grows the snake too;
  when the ring closes it simply goes away.
- Running into your own body ends the game — including turning into it. The
  cell the tail is about to leave is *not* your body: following your own tail
  is legal.
- The **edges** are a setting of their own, chosen before the run and kept
  until you change it. **Classic**: the walls kill, with deliberately **no
  grace window** — the step that leaves the field is the step that ends the
  game, exactly as the original does. **Wrap**: the edges carry you across
  instead, and only your own body can end the run.
- Filling the board leaves nowhere to put the next dot; that ends the game
  too, as a perfect one.

## Speed

The step rate is counted in ticks per move at 60 ticks a second, so a smaller
number is a faster snake. Every few foods shaves one tick off, down to the
chosen speed's floor:

| | Start | Floor | Faster every | Range |
|---|---|---|---|---|
| **Slow** | 12 | 8 | 6 foods | 5.0 → 7.5 cells/s |
| **Normal** | 9 | 5 | 5 foods | 6.7 → 12.0 cells/s |
| **Fast** | 7 | 4 | 5 foods | 8.6 → 15.0 cells/s |

The header shows the current rate in cells per second and flashes when it
steps up.

## Scoring

| What | Points |
|---|---|
| Each food | 10 × the length multiplier |
| Each bonus | 50 |

The **length multiplier** starts at ×1 and climbs a step every 8 cells the
snake has grown, up to ×5:

| Length | 4–11 | 12–19 | 20–27 | 28–35 | 36+ |
|---|---|---|---|---|---|
| Multiplier | ×1 | ×2 | ×3 | ×4 | ×5 |

Every constant is named in `src/game.h`.

The top ten scores of each edges setting and speed (with the length reached
and the date) are kept in `~/.config/Omacom/omasnake.conf`. The start screen
shows the best for each speed under the edges you are about to play, the header
shows the best for the game in progress and tints the score once you pass it,
and the game-over overlay names your rank when you made the table.

## Start screen

Two settings, and they are not the same kind of thing, so they are not one
list:

- **Edges** is a two-segment switch, Classic | Wrap, with the chosen segment
  filled in the accent color and one line under it saying what that choice
  costs you. `M` flips it. It changes nothing else; it just waits.
- **Speed** is a heading over three plain buttons, Slow / Normal / Fast on
  `1` `2` `3`, each with its range of cells a second, the best score for that
  speed under the current edges, and a "Last played" tag on the one you chose
  last. Pressing one starts a game right away, with whichever edges are set.

Both settings are remembered between sessions. The high scores panel (`H`)
repeats the same grouping: the edges switch on top — `M` still flips it — and
the three tables of that setting under a **Speed** heading.

## Keyboard

Everything is reachable without a mouse; each button shows its key as a badge.
There is no mouse control at all — the field is not clickable.

| Key | Action |
|---|---|
| `←` `↑` `↓` `→` or `H` `J` `K` `L` | Turn. Reversing is ignored; two turns can be queued |
| `Space` / `P` | Pause / resume |
| `R` | Restart with the same edges and speed |
| `Esc` | Leave the game (confirmed mid-game), close the high scores |
| `1` `2` `3` | Start a Slow / Normal / Fast game (start screen) |
| `M` | Switch the edges, Classic ↔ Wrap (start screen, high scores) |
| `H` | High scores (start screen) |
| `Enter` / `Space` | Play again after a game over |
| `Y` / `Enter`, `N` / `Esc` | Confirm / cancel a dialog |
| `Ctrl+Q` | Quit |

## Layout

The field is painted by one `FieldView` item at a whole number of pixels per
cell, centred in the window; the header carries the score, the length, the
current speed and the best score for these edges and speed. The window
scales with the desktop text size; the minimum is 640×520 logical pixels at
100%, which still leaves at least 16 pixels a cell.

## Agent environment

`bin/build-env omasnake` builds `build-env/omasnake/libomasnake_env.so`,
the game as a program plays it, for training agents (`docs/AGENT-ENV.md`).
It plays the same engine as the app but owns nothing else: no high scores,
nothing written to `omasnake.conf`.

**One step is one move of the snake.** The agent's turn is queued and the
game ticks until the head has moved, so the opening "Ready" beat and the
ticks between moves at any speed are folded into the step; nothing done
between moves could change anything.

### Config

| Key | Default | Choices |
|---|---|---|
| `mode` | `classic` | `classic` (walls kill), `wrap` |
| `difficulty` | `normal` | `slow`, `normal`, `fast`: how many ticks a move takes, which is what a bonus's six seconds are counted against |
| `actions` | `relative` | `relative`, `absolute` (below) |
| `max_steps` | 0 | moves before an episode is cut short; 0 never |

### Actions

- **`relative`**: `straight`, `turn_left`, `turn_right`, against the
  current heading. Never masked; the usual setup for a learner, since the
  same action means the same thing whichever way the snake points.
- **`absolute`**: `up`, `down`, `left`, `right`. The way back onto the neck
  is masked, since the snake would ignore it.

### Observation

| Tensor | Type | Shape | What |
|---|---|---|---|
| `grid` | uint8 | 24 × 32 | 0 empty, 1 body, 2 head, 3 tail, 4 food, 5 bonus |
| `state` | int32 | 12 | head_x, head_y, heading (0 up, 1 down, 2 left, 3 right), length, food_x, food_y, bonus_x, bonus_y (−1 when there is none), bonus_ticks left, score, multiplier, move_ticks |

Where the next dot will appear is never shown: a planner's clone with
`reseed_hidden` redraws it.

### Reward and signals

The reward is the score gained: 10 × the length multiplier for a food, 50
for a bonus. Each step also reports `score`, `ate`, `bonus`, `length`,
`food_distance` (moves from the head to the food, across an edge in Wrap),
`died` and `filled` (the perfect game). The episode ends when the run does.

The env reports `rules_version` 1 (`Rules::kVersion`), raised whenever the
same turns would play out differently. Its replays write each turn as
`U`, `D`, `L` or `R` between runs of ticks; the app has no replay viewer
for Snake yet.

## Build, test, run

```sh
bin/build omasnake
bin/test omasnake
bin/run omasnake
bin/build-env omasnake   # the agent environment library
```

Settings live in `~/.config/Omacom/omasnake.conf`.

## Install (Arch)

```sh
curl -fsSL https://raw.githubusercontent.com/MZAWeb/omagames/main/install.sh | bash -s omasnake
```

or from a checkout, `bin/install omasnake`.
