# TODO

## Omadoku
- [ ] Digit pad as a 3×3 keypad beside the board, with a Highlight / Note / Fill mode selector under it (in progress)
- [ ] Design pass: animations, start screen, won overlay
- [ ] Hints (next single), timer/stats per difficulty, best times
- [ ] Difficulty grading with more techniques (pairs, pointing, X-wing) so Hard is truly hard
- [ ] Promote `HintButton.qml` to `common/` (same one exists in Black Omack)

## Black Omack
- [ ] Design pass: table felt/layout (a lot of empty space at rest), card art, chip animations
- [ ] Insurance, surrender, re-splits as optional rules
- [ ] Speed setting in the UI (`stepInterval` already exists)
- [ ] Session stats screen (hands, net, streaks); bot chatter
- [ ] Promote `HintButton.qml` / `HandView.qml` to `common/` when a second card game needs them

## Repo
- [ ] GitHub Actions CI: build + test per game, qmllint, `-Werror`, no hex colors, packaging artifacts (in progress)
- [ ] Publish packages to the Omarchy repository
- [ ] Screenshots per game in the READMEs
- [ ] `common/` unit tests (OmarchyTheme parsing, defaults, mix/alpha)

## Future games
- [ ] **Omanix** — Xonix clone
- [ ] **Tetris** clone
- [ ] **Solitaire** (Klondike) — reuses `PlayingCard`
- [ ] **Minesweeper** clone
- [ ] **Mahjong** solitaire (tile matching) clone
