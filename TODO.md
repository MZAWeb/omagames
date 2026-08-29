# TODO

## Omadoku
- [ ] Implement a design direction (proposals on the local `design` branch: Focus Grid recommended, Candidate Studio's optional assistance later)
- [ ] Hints (next single), timer/stats per difficulty, best times
- [ ] Extra hard almost always lands on Y-wing (swordfish 1/80); add XY-chain / uniqueness to widen the top band

## Black Omack
- [ ] **Seat overlap at 4 mates** (in progress) — space the arc slots per mate count; no two seats may intersect.
- [ ] **Small windows: cap at 2 mates** (in progress) — below the size where the arc fits, the table only allows two mates instead of scrolling a roster.
- [ ] **Bet presets** (in progress) — `1`/`2`/`3` before a deal set preset bets.
- [ ] **Card travel animation** (in progress) — cards fly from the shoe to the seat.
- [ ] **Insurance and re-splits** (in progress) — real casino options, off by default; rerun the house-edge simulation after.
- [ ] **House-edge regression test** (in progress) — the simulator as a CI test.
- [ ] **Speed setting.** The bridge already has `stepInterval` (500 ms between bot/dealer actions, 180 ms per dealt card); expose it as Slow / Normal / Fast / Instant in the header with a key, persisted. Small.
- [ ] **Session stats screen.** A panel with hands played, net, best/worst streak, blackjacks, busts, biggest win — the bridge already tracks hands and net; the rest needs counting in `finishRound()` and persisting. Moderate.

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
