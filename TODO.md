# TODO

## Omadoku
- [ ] Implement a design direction (proposals on the local `design` branch: Focus Grid recommended, Candidate Studio's optional assistance later)
- [ ] Hints (next single), timer/stats per difficulty, best times
- [ ] Extra hard almost always lands on Y-wing (swordfish 1/80); add XY-chain / uniqueness to widen the top band

## Black Omack
- [ ] **Seat overlap at 4 mates.** The bot seats sit on fixed slots of the upper arc; with 4 mates the two left slots are too close and seat 2 covers seat 1's total (visible in `screenshots/blackomack.png`: Otto's "soft 20" reads "ft 20"). Fix: space the slots per mate count (or shrink seats when neighbours would overlap) and add a geometry check in tests/QML that no two seats intersect at the default window size.
- [ ] **Bet presets.** From the design: keys `1`/`2`/`3` before a deal set the bet to preset amounts (e.g. 10 / 50 / 100, or fractions of the bankroll) so you don't step −10/+10 every round. Small: bridge invokable + three badged buttons in the bet bar.
- [ ] **Tab regions.** From the design: `Tab` cycles keyboard focus between the table, the event log, the roster and the action dock so screen-reader/keyboard users can read each region; ordinary action keys keep working globally. Accessibility work, moderate.
- [ ] **Paged roster at the hard minimum.** At very small windows (≈640×520) with 5–6 mates the roster scrolls; the design wants `J`/`K` (or PgUp/PgDn) to page through full-size seats instead so nothing is ever clipped. Only matters on tiny windows.
- [ ] **Sounds and table talk.** Optional short cues (deal, win, lose, your turn) and one-line remarks from the bots keyed to their personality ("bold rookie" chases, "timid pro" grumbles). Needs a mute setting and a `common/` sound helper if we do it. Pure flavour; skip unless we want personality to shine.
- [ ] **Card travel animation.** Cards currently fade/slide in place; the design has them travel from the shoe to the seat. Cosmetic, moderate QML work.
- [ ] **Optional rules: insurance, surrender, re-splits.** Real blackjack options, off by default: insurance when the dealer shows an ace (side bet paying 2:1), late surrender (give up half the bet), and splitting again after a split (up to 3–4 hands). Engine + tests + UI buttons/keys for each; changes the house edge slightly, so the simulator should be rerun.
- [ ] **Speed setting.** The bridge already has `stepInterval` (500 ms between bot/dealer actions, 180 ms per dealt card); expose it as Slow / Normal / Fast (or Instant) in the header with a key, persisted. Small.
- [ ] **Session stats screen.** A panel (key `T`?) with hands played, net, best/worst streak, blackjacks, busts, biggest win — the bridge already tracks hands and net; the rest needs counting in `finishRound()` and persisting. Moderate.
- [ ] **House-edge regression test.** Turn the simulator from `scratchpad/sim.cpp` into a test: 50k seeded rounds of basic strategy through the real `Table` must land within −1.5%…+0.5% per initial bet, so any future rule change that accidentally tilts the game (for or against the house) fails CI. Runs in well under a second. Small.

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
