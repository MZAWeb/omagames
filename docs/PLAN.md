# Omagames — plan

*Status as of 2026-08-29: repo skeleton and shared library are in place and
build; both games are placeholders awaiting implementation (see `docs/tasks/`).*

## Goals

1. Ship a growing set of small, polished games for Omarchy, each an independent
   Arch package, developed in one monorepo with shared infrastructure.
2. Every game follows the Omarchy theme (colors.toml), system dark/light mode and
   text scale, like `omawrite`/`omacalc`.
3. **Game logic first.** Engines are plain, tested C++; UI is a thin QML layer we
   can redesign later without touching rules.

First two games: **Omadoku** (Sudoku) and **Black Omack** (Blackjack).

## Repository layout

```
omagames.pro            subdirs project: builds every game
bin/build|run|test|install
common/
  common.pri            included by each game's .pro (compiled in, no static lib)
  common-tests.pri      headless subset for test binaries
  common.qrc            fonts + OmaGames QML module
  src/systemtheme.*     portal/Qt dark-mode + text-scale watcher (from omacalc)
  src/omarchytheme.*    colors.toml reader → `theme` QML context property
  src/appsetup.*        OmaGames::setupApplication / setupEngine
  qml/OmaGames/         qmldir, OmaButton.qml, OmaPanel.qml
  fonts/                iA Writer Mono S (OFL)
games/<game>/
  <game>.pro            include(../../common/common.pri) + game sources
  src/                  engine (QtCore only) + <Game>Game QObject bridge + QML
  tests/tests.pro, tst_<game>.cpp
  pkgbuild/PKGBUILD, <game>.desktop, <game>.install, <game>.svg (icon, TODO)
docs/PLAN.md, docs/tasks/*.md
```

### Why compile `common/` into each game instead of a library
qmake static libs across subdirs are fiddly (link order, resource init, tests).
A `.pri` keeps every game a single self-contained binary, and packaging stays
identical to omacalc. If `common/` grows large we can revisit.

### Shipping individual games
Each `games/<game>/pkgbuild/PKGBUILD` builds only that game (`bin/build <game>`)
and installs its binary, desktop file and icon. Later a CI job can loop over
`games/*` and publish each package to the Omarchy repo.

## Architecture per game (mandatory shape)

```
QML (Main.qml + views)   ← renders state, calls Q_INVOKABLEs; no rules; theme colors only
        ↓ context property "game"
<Game>Game : QObject     ← Q_PROPERTYs, models, Q_INVOKABLE actions, persistence (QSettings)
        ↓
Engine classes (QtCore only, no QObject needed)   ← rules, generation, AI; unit-tested
```

Rule of thumb: if you deleted every `.qml` file, `bin/test` should still pass
and cover all the rules.

## Theming contract

`theme` (OmarchyTheme) exposes: `background, darkBackground, darkerBackground,
lighterBackground, foreground, darkForeground, lightForeground,
brightForeground, accent, selection, muted, red, yellow, orange, green, cyan,
blue, magenta`, `darkMode`, `textScale`, `color(key)`, `mix(a,b,t)`,
`alpha(c,a)`. Defaults kick in when a key is missing so games render fine
outside Omarchy. `--theme-file <path>` overrides the file for previews.

Shared QML controls (`import OmaGames`): `OmaButton { primary: bool }`,
`OmaPanel`. Agents may add generic controls (card face, number pad, dialog
frame…) — register in `qmldir` and `common.qrc`.

## Persistence

`QSettings` with org `Omacom`, app `<game>` → `~/.config/Omacom/<game>.conf`.
Complex state (a blackjack table, an in-progress sudoku) is stored as one JSON
string value under a versioned key (`state/v1`) so we can migrate later.

## Game specs (summary — full detail in `docs/tasks/`)

### Omadoku
- Start screen: pick Easy / Medium / Hard → puzzle generated on the fly with a
  unique solution.
- Entry mode vs Notes mode (pencil marks, up to 9 small digits per cell).
- Setting: *Check as I go* (wrong entries flagged immediately) vs *Check when
  full* (validate only once all 81 cells are filled). Notes are never validated.
- Undo, erase, restart, new game, resume in-progress puzzle across launches.
- Keyboard: arrows move, 1–9 enter, `N` toggles notes, Backspace/Delete erase, `Ctrl+Z` undo.

### Black Omack
- Player starts with 1,000 Omabucks; bankroll persists across launches until
  broke (then forced new game) or the user starts a new game.
- Bets: 10 … all-in, in increments of 10.
- 0–5 AI table mates chosen by the user (changeable between hands). Each bot has
  a persistent personality (skill, aggressiveness) rolled at creation and stored.
- Rules (v1): 6-deck shoe, dealer stands on soft 17, blackjack pays 3:2, double
  on any first two cards, split once (no re-split), no insurance/surrender.
- Bots play automatically with visible, slightly paced actions; the user acts
  in their own seat with Hit / Stand / Double / Split.

## Work breakdown for parallel agents

| Agent | Brief | Touches |
|---|---|---|
| A | `docs/tasks/omadoku.md` | `games/omadoku/**` (+ may add generic QML to `common/`) |
| B | `docs/tasks/blackomack.md` | `games/blackomack/**` (+ may add generic QML to `common/`) |
| C (optional, alongside) | `docs/tasks/repo-chores.md` | icons, README screenshots, packaging check, CI, common/ tests |

A and B are independent. The only shared-file contention is `common/qml/OmaGames/qmldir`
and `common/common.qrc` if both add controls — keep additions append-only.

## Later ideas (not now)
- Design pass on both games (animations, card art, better start screens).
- Sudoku: difficulty grading by solving technique, hints, timer/stats.
- Blackjack: insurance, surrender, re-splits, bot chatter, table stats.
- CI: build + test all games, produce packages per game.
- More games sharing `common/` (cards model/UI from Black Omack is the first candidate).
