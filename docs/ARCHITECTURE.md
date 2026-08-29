# Architecture

## Goals

1. A growing set of small, polished games for Omarchy, each an independent Arch
   package, developed in one monorepo with shared infrastructure.
2. Every game follows the Omarchy theme (`colors.toml`), system dark/light mode
   and text scale, like `omawrite` / `omacalc`.
3. **Game logic first.** Engines are plain, tested C++; UI is a thin QML layer
   that can be redesigned without touching rules.

## Repository layout

```
omagames.pro            subdirs project: builds every game
bin/build|run|test|install
common/
  common.pri            included by each game's .pro (compiled in, no static lib)
  common-tests.pri      headless subset for test binaries
  common.qrc            fonts + OmaGames QML module
  src/systemtheme.*     portal / Qt dark-mode and text-scale watcher
  src/omarchytheme.*    colors.toml reader → `theme` QML context property
  src/appsetup.*        OmaGames::setupApplication / setupEngine
  qml/OmaGames/         qmldir + generic controls (OmaButton, OmaPanel, PlayingCard, …)
  fonts/                iA Writer Mono S (OFL)
  tests/tests.pro, tst_common.cpp   `bin/test common`, like a game's suite
games/<game>/
  <game>.pro            include(../../common/common.pri) + game sources
  src/                  engine (QtCore only) + <Game>Game QObject bridge + QML
  tests/tests.pro, tst_<game>.cpp
  pkgbuild/PKGBUILD, <game>.desktop, <game>.install, <game>.svg
  README.md             what it is, rules, keys — the spec for that game
docs/
```

### Why `common/` is compiled into each game instead of a library
qmake static libs across subdirs are fiddly (link order, resource init,
tests). A `.pri` keeps every game a single self-contained binary and packaging
identical to omacalc. Revisit if `common/` grows large.

### Shipping individual games
Each `games/<game>/pkgbuild/PKGBUILD` builds only that game (`bin/build <game>`)
and installs its binary, desktop file and icon. CI can loop over `games/*`.

## Shape of a game (mandatory)

```
QML (Main.qml + small components)   renders state, calls Q_INVOKABLEs; no rules; theme colors only
        ↓ context property "game"
<Game>Game : QObject                Q_PROPERTYs, models, Q_INVOKABLE actions, persistence (QSettings), pacing timers
        ↓
Engine classes (QtCore only)        rules, generation, AI; pure and unit-tested; no timers, no QObject
```

- The engine is step-wise and synchronous: anything that needs pacing (an AI
  opponent "thinking", a dealer drawing cards) exposes an `advance()`-style call
  and the bridge drives it with a `QTimer` whose interval is a property, so
  tests run it at 0 ms.
- Randomness is seeded explicitly (`QRandomGenerator(seed)`), never global, so
  tests are deterministic.
- Everything a player would want to keep (bankroll, in-progress puzzle,
  settings) is persisted by the bridge as JSON under a versioned QSettings key.

## Theming contract

`theme` (`OmarchyTheme`) exposes: `background, darkBackground,
darkerBackground, lighterBackground, foreground, darkForeground,
lightForeground, brightForeground, accent, selection, muted, red, yellow,
orange, green, cyan, blue, magenta`, `darkMode`, `textScale`, `color(key)`,
`mix(a, b, t)`, `alpha(c, a)`. Defaults kick in when a key is missing so games
render fine outside Omarchy. `--theme-file <path>` overrides the file for previews.

Sizes are expressed relative to `theme.textScale`; windows should scale their
playfield with the window like omacalc's `uiScale`.

## Keyboard first

Every action has a key, and the control that triggers it shows that key. See
`CLAUDE.md` for the required conventions.

## CI

`.github/workflows/ci.yml` runs on every push to `main` and every pull
request. Every job runs in an `archlinux:latest` container with `base-devel`,
`git`, `qt6-base` and `qt6-declarative`: Omarchy is Arch, so CI compiles
against the packages a player actually has.

| Job | What it does |
|---|---|
| **Discover games** | Lists `games/*` into a JSON matrix, and every other top-level directory with its own `tests/tests.pro` (`common/`) into a second one. Adding a game or a shared suite needs no workflow edit. |
| **build & test (`<game>`)** | One job per game: `bin/build <game>` then `bin/test <game>` (headless, `QT_QPA_PLATFORM=offscreen`). |
| **test (`<suite>`)** | One job per shared suite: `bin/test <suite>`. `common/` has no binary of its own, so there is nothing to build first. |
| **Code quality** | Four named steps, below. |
| **package (`<game>`)** | One job per game: `makepkg` on `games/<game>/pkgbuild/PKGBUILD`, uploading `*.pkg.tar.zst` as an artifact. `makepkg` refuses to run as root, so the job creates a `builder` user and gives it the workspace. |

The quality steps, each failing on its own so the log names the problem:

1. **Shell scripts parse** — `sh -n` over every `bin/*` script.
2. **No hardcoded colors in QML** — greps `games/` and `common/` for
   `#rrggbb`; colors come from the `theme` context property (`CLAUDE.md`).
3. **QML lint** — `qmllint` over every `.qml` with `-I common/qml` so
   `import OmaGames` resolves. `qmllint` exits 0 even when it warns, so the
   step fails on any `Warning:`/`Error:` line. `.qmllint.ini` disables exactly
   one category, `UnqualifiedAccess`: `theme` and `game` are C++ context
   properties injected at runtime and therefore invisible to static analysis,
   and qmllint cannot whitelist individual identifiers.
4. **C++ builds clean with `-Wall -Wextra -Werror`** — rebuilds the games and
   every test suite with `qmake6 … QMAKE_CXXFLAGS+=`. The flags also carry
   `-isystem /usr/include/qt6` so Qt's own headers are exempt: the gate is on
   our code, not on what the current GCC has learned to say about Qt.

Run all four locally before pushing; they need nothing the repo does not
already require.
