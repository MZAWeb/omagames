# TODO

## Omadoku
- [ ] Layout-aware digit keys (AZERTY: unshifted row is symbols) — digits resolve only from Key_1-9 and the US shifted symbols since the keycode fallback was removed
- [ ] `sudokugame.cpp` is ~575 lines; split the selection/notes/highlight handling out of the bridge
- [ ] **Sudoku coach** — like Black Omack's coach: opt-in, off by default, one panel that names the next logical step for the selected cell/board ("Naked single: only 5 fits in r5c5", up to X-wing/Y-wing/swordfish with the cells involved highlighted on request). Must be backed by the real technique ladder in `sudokugrader` so every explanation is a genuine deduction, never a lookup of the solution. Larger; needs the grader to report the cells/candidates behind each step.
- [ ] Hints (next single), timer/stats per difficulty, best times
- [ ] Extra hard almost always lands on Y-wing (swordfish 1/80); add XY-chain / uniqueness to widen the top band

## Omanix
- [ ] Play-test the difficulty ramp (ball speed per level, chaser count) and tune the constants in `level.cpp`
- [ ] Optional: reduced-motion setting for the sweep-fill/pulse animations

## Omasweeper
- [ ] Custom board sizes (width/height/mines) with the same no-guess guarantee
- [ ] Optional "?" marks and a per-preset stats panel (games, wins, streak)

## Black Omack
- [ ] **Speed setting.** The bridge already has `stepInterval` (500 ms between bot/dealer actions, 180 ms per dealt card); expose it as Slow / Normal / Fast / Instant in the header with a key, persisted. Small.
- [ ] **Session stats screen.** A panel with hands played, net, best/worst streak, blackjacks, busts, biggest win — the bridge already tracks hands and net; the rest needs counting in `finishRound()` and persisting. Moderate.
- [ ] **Highlight the deciding seat during insurance.** `Table::currentSeat()` is −1 outside player turns, so the bot deciding on insurance isn't marked; expose the insurance cursor and give `TableSeat` the active treatment. Tiny.

## Shared
- [ ] Promote `HandView.qml` to `common/` when a second card game needs it

## Release
- [ ] Cut v0.1.0 with `bin/release` and submit `packaging/opr/*` to omarchy-pkgs (see docs/RELEASING.md) — when the games are ready

## Future games
- [ ] **Tetris** clone
- [ ] **Snake** clone (Omasnake?) — reuse Omanix's tick engine, painted `FieldView`, overlays and high-score table
- [ ] **Solitaire** (Klondike) — reuses `PlayingCard`
- [ ] **Mahjong** solitaire (tile matching) clone
