# TODO

## Omadoku
- [ ] Layout-aware digit keys (AZERTY: unshifted row is symbols) — digits resolve only from Key_1-9 and the US shifted symbols since the keycode fallback was removed
- [ ] `sudokugame.cpp` is ~575 lines; split the selection/notes/highlight handling out of the bridge
- [ ] **Sudoku coach** — like Black Omack's coach: opt-in, off by default, one panel that names the next logical step for the selected cell/board ("Naked single: only 5 fits in r5c5", up to X-wing/Y-wing/swordfish with the cells involved highlighted on request). Must be backed by the real technique ladder in `sudokugrader` so every explanation is a genuine deduction, never a lookup of the solution. Larger; needs the grader to report the cells/candidates behind each step.
- [ ] Hints (next single), timer/stats per difficulty, best times
- [ ] Extra hard almost always lands on Y-wing (swordfish 1/80); add XY-chain / uniqueness to widen the top band

## Omanix
- [ ] Redo the slowdown fix (reverted in #32): the close-call scan and repaint caching from #29 broke rendering/movement on the real desktop while headless tests stayed green — redo incrementally on a clean base, verifying each commit visually on screen, and add a rendering test that catches misplaced geometry (e.g. compare a FieldView grab against the engine state)
- [ ] Optional: reduced-motion setting for the sweep-fill/pulse animations

## Omasweeper
- [ ] Custom board sizes (width/height/mines) with the same no-guess guarantee
- [ ] Optional "?" marks and a per-preset stats panel (games, wins, streak)

## Omasnake
- [ ] Play-test the three speeds and the bonus timing

## Omatris
- [ ] Play-test DAS/ARR and lock-delay feel; consider a settings panel for them
- [ ] Sprint leaderboard shows time only — add lines/min

## Black Omack
- [ ] **Speed setting.** The bridge already has `stepInterval` (500 ms between bot/dealer actions, 180 ms per dealt card); expose it as Slow / Normal / Fast / Instant in the header with a key, persisted. Small.
- [ ] **Session stats screen.** A panel with hands played, net, best/worst streak, blackjacks, busts, biggest win — the bridge already tracks hands and net; the rest needs counting in `finishRound()` and persisting. Moderate.
- [ ] **Highlight the deciding seat during insurance.** `Table::currentSeat()` is −1 outside player turns, so the bot deciding on insurance isn't marked; expose the insurance cursor and give `TableSeat` the active treatment. Tiny.

## Shared
- [ ] Promote `HandView.qml` to `common/` when a second card game needs it

## Release
- [ ] Cut v0.2.0 with `bin/release` and submit `packaging/opr/*` to omarchy-pkgs (see docs/RELEASING.md) — when the games are ready

## Future games
- [ ] **Texas hold 'em** — same table, bots with persistent personalities, chips and pacing as Black Omack; blinds, betting rounds, hand ranking, showdown
- [ ] **Five-card draw poker** — same family as hold 'em (ante, draw, showdown), sharing the poker hand evaluator
- [ ] **Shared Omabucks wallet** — one bankroll persisted in `common/` and used by every betting game (Black Omack, hold 'em, five-card draw); best-bankroll high score becomes shared too
- [ ] **Breakout / Arkanoid** — on the Omanix tick engine and painted field; paddle, ball, bricks, a few power-ups
- [ ] **2048** — tiny engine, slide/merge animations, keyboard-native, high scores
- [ ] **Asteroids** — Omanix loop with painted vector shapes; thrust/rotate, wrap edges, high scores
- [ ] **FreeCell** — reuses `PlayingCard`; keyboard cursor over piles; every deal solvable (Microsoft-compatible deal numbers)
- [ ] **Omaflow** — rotate-the-pipes puzzle (Simon Tatham's "Net"). Generate a random spanning tree over the grid (randomised DFS/Kruskal from a seed); each cell's tile is its tree degree (end, straight/corner, T, cross); pick a source cell; scramble by rotating every tile 0–3 quarter turns — the tree is the guaranteed solution. Run a solver (edge parity: each edge is connected on both sides or neither, propagate, bounded search fallback) and regenerate until the solution is unique, like Omasweeper's no-guess guarantee; rate difficulty by how much search vs. propagation it needed. Knobs: grid size 5×5–15×15, wrapping edges (much harder), crosses allowed or not, how many tiles start already correct, optional wall barriers. Painted FieldView with pipes as strokes and water flowing from the source along connected tiles as the reveal; keyboard cursor + rotate both ways, mouse left/right click; best times per preset. Later: Netslide variant (slide rows/columns).
- [ ] **Solitaire** (Klondike) — reuses `PlayingCard`
- [ ] **Mahjong** solitaire (tile matching) clone
