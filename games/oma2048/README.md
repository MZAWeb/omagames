# Oma2048

2048 for Omarchy. Slide numbered tiles around a 4×4 grid; when two tiles with
the same number collide they merge into one worth their sum. Every merge
scores, every move spawns a new tile, and the board only ever fills up — reach
**2048** before you run out of moves, then keep going for the high score.

## Rules

- The board is **4×4** and a new game starts with **two** tiles.
- A move (`←` `↑` `↓` `→`) slides **every** tile as far as it can go in that
  direction. Two equal tiles that collide **merge** into one tile of double
  the value. A tile merges **at most once per move**, and pairs closest to the
  edge you are moving toward merge first: `2 2 2 →` gives `_ 2 4`, and
  `2 2 2 2 →` gives `_ _ 4 4`.
- Each merge **scores** the value of the tile it created: merging two 8s is
  worth 16.
- After every move that changed the board, one new tile appears in a uniformly
  random empty cell: a **2** nine times out of ten, a **4** the tenth. A move
  that would change nothing is ignored — nothing slides, nothing spawns.
- Making a **2048** tile wins: the win overlay shows once, and **Keep going**
  continues the same run with the same score. The run truly ends only when it
  is out of moves.
- **Game over** when no cell is empty and no two adjacent tiles are equal.
  The score is recorded then, and only then — an abandoned run keeps nothing.
- **Undo** (`U`) takes back the last move — board, score and the spawned tile.
  One level only: it is not available before the first move, twice in a row,
  after the game is over, or after a relaunch.

## Scoring

The top ten runs — score, the highest tile reached, and the date — are kept in
`~/.config/Omacom/oma2048.conf`. The header shows the current score and the
best, and tints the score once you pass the best. The game-over overlay names
your rank when the run made the table.

An in-progress run (board, score, whether the win was already shown) is saved
on every move and restored on launch, so closing the window loses nothing.

## Keyboard

Everything is reachable without a mouse; each button shows its key as a badge.
The board is not clickable.

| Key | Action |
|---|---|
| `←` `↑` `↓` `→` or `H` `J` `K` `L` | Slide the tiles |
| `U` | Undo the last move |
| `N` | New game (confirmed while a run is in progress) |
| `S` | High scores |
| `C` / `Enter` | Keep going (win overlay) |
| `Enter` / `Space` | New game (game-over overlay) |
| `Y` / `Enter`, `N` / `Esc` | Confirm / cancel a dialog |
| `Esc` | Close an overlay or dialog |
| `Ctrl+Q` | Quit |

## Layout

The board is a centred square that scales with the window, the header above it
carries the score and the best. Tiles slide, merged tiles pop, spawned tiles
fade in. Tile colors are a ramp built with `theme.mix` from the Omarchy theme
— low tiles near the background, high tiles toward the accent — so the board
follows the desktop like every other omagame; no color is hardcoded.

## Agent environment

`bin/build-env oma2048` builds `build-env/oma2048/liboma2048_env.so`, the
game as a program plays it, for training agents (`docs/AGENT-ENV.md`): the
same engine as the app, without undo, high scores or settings.

**Config:** `goal` (default 0): the tile that ends the run when it first
appears; 0 plays on until no slide is left. Plus `max_steps`, as every env.

**Actions:** `left`, `right`, `up`, `down`; a slide that would change
nothing is masked. One step is one slide, then the new tile spawns.

**Observation:**

| Tensor | Type | Shape | What |
|---|---|---|---|
| `board` | uint8 | 4 × 4 | each tile's power of two (1 is a 2, 11 is 2048), 0 for empty |
| `afterstates` | uint8 | 4 × 4 × 4 | per action, the board the slide leaves before the spawn; zeros where it is masked |
| `gains` | int32 | 4 | per action, the points that slide would score |
| `stats` | int32 | 4 | score, highest tile, empty cells, moves |

Where the next tile spawns is never shown: a clone with `reseed_hidden`
redraws it.

**Reward and signals:** the reward is the score gained. Each step also
reports `score`, `merges`, `highest`, `empty` and `won` (1 on the move that
first makes a 2048). An episode ends when no slide is left, or at the goal.

**Rules version:** `Rules::kVersion`, 1. It goes up with any change that
makes the same moves from the same seed play out differently.

**Replays and frames:** a replay is the seed and the slides, `L R U D`.
`og_replay_frames` plays one back for omagym's web viewer: a frame per move
with the 16 tile values, the score, the highest tile and the move made.

## Build, test, run

```sh
bin/build oma2048
bin/test oma2048
bin/run oma2048
```

Settings live in `~/.config/Omacom/oma2048.conf`.

## Install (Arch)

```sh
curl -fsSL https://raw.githubusercontent.com/MZAWeb/omagames/main/install.sh | bash -s oma2048
```

or from a checkout, `bin/install oma2048`.
