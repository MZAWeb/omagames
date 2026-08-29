# TODO

## Omadoku
- [ ] Implement a design direction (proposals on the local `design` branch: Focus Grid recommended, Candidate Studio's optional assistance later)
- [ ] Hints (next single), timer/stats per difficulty, best times
- [ ] Extra hard almost always lands on Y-wing (swordfish 1/80); add XY-chain / uniqueness to widen the top band

## Black Omack
- [ ] Implement a design direction (proposals on the local `design` branch: House Table with the optional decision coach and the 0–5 mates scaling study)
- [ ] Insurance, surrender, re-splits as optional rules
- [ ] Speed setting in the UI (`stepInterval` already exists)
- [ ] Session stats screen (hands, net, streaks); bot chatter
- [ ] House-edge regression test from the simulator (assert measured EV stays near −0.5% over seeded rounds)

## Shared
- [ ] Promote `HandView.qml` to `common/` when a second card game needs it
- [ ] Refresh `screenshots/` (both are behind: Omadoku's start screen/validate toggle, Black Omack's stats and badges)

## Release
- [ ] Cut v0.1.0 with `bin/release` and submit `packaging/opr/*` to omarchy-pkgs (see docs/RELEASING.md) — when the games are ready

## Future games
- [ ] **Omanix** — Xonix clone
- [ ] **Tetris** clone
- [ ] **Solitaire** (Klondike) — reuses `PlayingCard`
- [ ] **Minesweeper** clone
- [ ] **Mahjong** solitaire (tile matching) clone
