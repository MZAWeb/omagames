# TODO

## Omadoku
- [ ] **Sudoku coach** — like Black Omack's coach: opt-in, off by default, one panel that names the next logical step for the selected cell/board ("Naked single: only 5 fits in r5c5", up to X-wing/Y-wing/swordfish with the cells involved highlighted on request). Must be backed by the real technique ladder in `sudokugrader` so every explanation is a genuine deduction, never a lookup of the solution. Larger; needs the grader to report the cells/candidates behind each step.
- [ ] Hints (next single), timer/stats per difficulty, best times
- [ ] Extra hard almost always lands on Y-wing (swordfish 1/80); add XY-chain / uniqueness to widen the top band

## Black Omack
- [ ] **Speed setting.** The bridge already has `stepInterval` (500 ms between bot/dealer actions, 180 ms per dealt card); expose it as Slow / Normal / Fast / Instant in the header with a key, persisted. Small.
- [ ] **Session stats screen.** A panel with hands played, net, best/worst streak, blackjacks, busts, biggest win — the bridge already tracks hands and net; the rest needs counting in `finishRound()` and persisting. Moderate.
- [ ] **Highlight the deciding seat during insurance.** `Table::currentSeat()` is −1 outside player turns, so the bot deciding on insurance isn't marked; expose the insurance cursor and give `TableSeat` the active treatment. Tiny.

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
