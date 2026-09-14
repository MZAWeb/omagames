# Omadrop

An endless physics game for Omarchy. Aim the launcher, send a ball through the
field and clear numbered pegs before they reach the top.

## Rules

- Move the dotted guide to aim, then launch. The ball falls under gravity and
  rebounds from the walls and the pile.
- A hit removes one point from a peg and adds one point to the score. A peg
  disappears when its number reaches zero.
- Circles and squares obey the same rules and stay put during the shot.
- After the ball leaves the bottom, every remaining peg moves one step upward
  and one, two or three new pegs enter along the bottom. Single-peg turns are
  most common.
- The run ends when that upward step carries a peg through the top edge.

## Keys

| Key | Action |
|---|---|
| `Left` / `Right` or `H` / `L` | Aim |
| `Space` / `Enter` | Launch |
| `P` | Pause / resume |
| `R` | Restart |
| `H` | High scores (start screen) |
| `Esc` | Back / leave game |
| `Ctrl+Q` | Quit |

Drag and release on the field to aim and launch with a mouse or touchscreen.

## Install

```sh
curl -fsSL https://raw.githubusercontent.com/MZAWeb/omagames/main/install.sh | bash -s omadrop
```

## Build and run

```sh
bin/run omadrop
bin/test omadrop
```
