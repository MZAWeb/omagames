# TODO

## Omadoku
- [ ] Implement a design direction (proposals on the local `design` branch: Focus Grid recommended, Candidate Studio's optional assistance later)
- [ ] Hints (next single), timer/stats per difficulty, best times
- [ ] Extra hard almost always lands on Y-wing (swordfish 1/80); add XY-chain / uniqueness to widen the top band

## Black Omack
- [ ] Best bankroll high score (PR in progress)
- [ ] House Table follow-ups from the design: bet presets, Tab regions, paged roster at the hard minimum, sounds/table talk, card travel animations
- [ ] Insurance, surrender, re-splits as optional rules
- [ ] Speed setting in the UI (`stepInterval` already exists)
- [ ] Session stats screen (hands, net, streaks); bot chatter
- [ ] House-edge regression test from the simulator (assert measured EV stays near −0.5% over seeded rounds)

## Shared
- [ ] Promote `HandView.qml` to `common/` when a second card game needs it

## Release
- [ ] Cut v0.1.0 with `bin/release` and submit `packaging/opr/*` to omarchy-pkgs (see docs/RELEASING.md) — when the games are ready

## Future games
- [ ] **Omanix** — Xonix clone
- [ ] **Tetris** clone
- [ ] **Solitaire** (Klondike) — reuses `PlayingCard`
- [ ] **Minesweeper** clone
- [ ] **Mahjong** solitaire (tile matching) clone
