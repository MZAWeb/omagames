# Omanix

Xonix for Omarchy. Drive a marker along the claimed frame of a 64×40 field,
cut into the open sea and close the cut back on solid ground to claim
everything you fenced off. Balls bounce around the sea and pop your trail;
chasers crawl the edge of the ground you own. Claim three quarters of the
field to clear the level.

## Rules

- The field starts as a one-cell frame of claimed ground around open sea —
  the same thickness as the marker and the trail it cuts. The marker lives
  on claimed ground.
- Leaving the ground cuts a **trail** through the sea. The marker never stops
  on the sea: it keeps going in the last direction until it reaches ground
  again. Then the trail becomes ground, and so does every open region that
  holds no ball.
- **Balls** drift diagonally through the sea and bounce off ground, corner
  rules included (the axis that hit is the one that flips). The trail is not
  a wall to them: a ball touching your trail costs a life and wipes the
  trail. So does running into your own trail.
- **Chasers** crawl along the edge of the claimed ground, following every
  edge a claim adds and turning back only at a dead end. Where the boundary
  branches they take one of the ways on at random, so the same claim never
  sends them the same way twice; and a chaser that catches itself going round
  in circles — around a pocket of sea you have fenced off, say — gives that
  boundary up, walks across the ground to another region's edge and carries
  on from there. They kill on contact wherever you are, and more join every
  few levels.
- Every level opens with a short banner ("Level 3 · 5 balls · 2 chasers")
  while everything holds still for 1.2 s; a key pressed during it takes
  effect the moment play starts.
- While a ball is within 3 cells of your trail the trail pulses brighter:
  that is the close-call zone, so finish the cut or expect to lose it. The
  marker is a solid square on the ground and an accent-filled outline while
  cutting, so you always know when you are exposed.
- Level goal: **75%** of the interior claimed. Each level adds a ball, the
  balls get a little faster, and chasers pile up every few levels.
- Three lives, an extra one every 10,000 points. After a lost life everything
  holds still for a second and the marker returns to the bottom edge, onto
  the cell the nearest chaser has the longest crawl to reach.

## Scoring

Big claims are the point:

| What | Points |
|---|---|
| Each claimed cell | 1 × level |
| A single claim of ≥ 10% of the field | ×2 (**Big cut**) |
| A single claim of ≥ 25% of the field | ×4 |
| Closing while a ball is within 3 cells of the trail | +500 (**Close call**) |
| Finishing a level | 500 × level per life left |

Every constant is named in `src/game.h`.

## Difficulties

Speeds are ticks per move at 60 ticks a second, so a smaller number is a
faster mover; the marker itself moves every 4 ticks.

| | Balls at start | Ball speed | Chasers at start | Chaser speed | Ramp |
|---|---|---|---|---|---|
| **Easy** | 2 | 9 | 1, one more every 5 levels | 12 | a tick faster every 4 levels, down to 6 |
| **Normal** | 3 | 8 | 2, one more every 4 levels | 11 | a tick faster every 3 levels, down to 5 |
| **Hard** | 4 | 4 | 3, one more every 2 levels | 7 | a tick faster every 2 levels, down to 3 |

Normal is a short step off Easy — one more ball and one more chaser, a single
tick quicker — not a halfway house to Hard, where the balls move at better
than twice Easy's pace from the first level.

Each difficulty has its own key and none of them is a default, so no button
on the start screen is drawn as the primary one; the difficulty played last
is merely marked. The top ten scores per difficulty (with level and date) are
kept in `~/.config/Omacom/omanix.conf`; the header shows the best for the
difficulty in play and tints the score once you pass it, and the game-over
overlay names your rank when you made the table.

## Keyboard

Everything is reachable without a mouse; each button shows its key as a badge.

| Key | Action |
|---|---|
| `←` `↑` `↓` `→` or `H` `J` `K` `L` | Move. A tap moves one cell on ground, holding keeps moving; on the sea the marker keeps going until it reaches ground. Turns are grid-aligned; turning straight back onto the trail is ignored. |
| `Space` / `P` | Pause / resume |
| `R` | Restart the level from scratch (keeps score and lives) |
| `1` `2` `3` | Start an Easy / Normal / Hard game (start screen) |
| `H` | High scores (start screen) |
| `Enter` / `Space` | Continue after a cleared level; play again after a game over |
| `Esc` | Leave the game (confirmed mid-game), close the high scores |
| `Y` / `Enter`, `N` / `Esc` | Confirm / cancel a dialog |
| `Ctrl+Q` | Quit |

## Layout

The field is painted by one `FieldView` item at a whole number of pixels
per cell, centred in the window; the header carries level, difficulty, the
claimed percentage with a bar toward the goal, lives and the score. The
window scales with the desktop text size; the minimum is 720×540 logical
pixels at 100% (8 pixels per cell).

## Build, test, run

```sh
bin/build omanix
bin/test omanix
bin/run omanix
```

Settings live in `~/.config/Omacom/omanix.conf`.

## Install (Arch)

```sh
curl -fsSL https://raw.githubusercontent.com/MZAWeb/omagames/main/install.sh | bash -s omanix
```

or from a checkout, `bin/install omanix`.
