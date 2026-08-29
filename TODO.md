# TODO

## Omadoku
- [ ] Mode badges show the full chord (`Shift+1-9`, …) — PR in progress
- [ ] Design pass: animations, start screen, won overlay
- [ ] Hints (next single), timer/stats per difficulty, best times
- [ ] Difficulty grading with more techniques (pairs, pointing, X-wing) so Hard is truly hard

## Black Omack
- [ ] Design pass: table felt/layout (a lot of empty space at rest), card art, chip animations
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
