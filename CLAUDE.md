# Omagames — rules for anyone (human or agent) working in this repo

Read `docs/ARCHITECTURE.md` first. Adding a game? Follow `docs/NEW-GAME.md`.
Working on an existing game? Its `games/<game>/README.md` is the spec; stay
inside `games/<game>/` unless you are deliberately changing shared code.

## Stack
- Qt 6, C++17, QML (Qt Quick Controls 2, Material style), qmake. No CMake, no extra deps.
- Build with `bin/build <game>`, test with `bin/test <game>`, run with `bin/run <game>`.
  Every change must leave `bin/build` and `bin/test` green.

## Architecture rule: logic first, UI is replaceable
- All game logic lives in plain C++ classes with **no Qt GUI/QML dependency**
  (QtCore only) under `games/<game>/src/`. They must be fully testable with
  QtTest headlessly (`QTEST_GUILESS_MAIN`).
- One `QObject` "game" class (e.g. `SudokuGame`, `BlackjackGame`) is the *only*
  bridge to QML: it exposes state via `Q_PROPERTY`/models and actions via
  `Q_INVOKABLE`. QML never contains rules; it only renders state and calls actions.
- The UI must be swappable without touching the engine. Keep QML free of
  hardcoded colors, fonts and pixel sizes: use the `theme` context property
  (`OmarchyTheme`) and `theme.textScale`. Shared controls come from `import OmaGames`.
  The one sanctioned exception so far is omadoku's digit highlight
  (`SudokuGame::highlightColor` / `highlightInk`): a highlighter pen has to be
  highlighter yellow whatever the desktop looks like. Such a color is named as
  a constant in C++ and read from QML as a property — never written into a
  `.qml` file, which the CI hex-color grep covers.
- Rule of thumb: delete every `.qml` file and `bin/test` should still pass and
  still cover all the rules.

## Shared code (`common/`)
- `OmarchyTheme` (`theme` in QML): every color from Omarchy's `colors.toml`,
  `darkMode`, `textScale`, plus `theme.mix(a, b, t)` and `theme.alpha(c, a)`.
- `OmaGames::setupApplication` / `setupEngine`: see `common/src/appsetup.h`.
- QML module `OmaGames` (`common/qml/OmaGames/`): `OmaButton`, `OmaPanel`,
  `OmaKeyHint` (keycap badge), `OmaHintButton` (button + badge), `PlayingCard`.
  Add new *generic* controls there
  (register in `qmldir` **and** `common/common.qrc`, append-only); game-specific
  controls stay in the game.
- Changing an existing `common/` API affects every game: build and test all of
  them (`bin/build && bin/test`) and keep the change backwards compatible when possible.

## Keyboard first
- Everything must be playable without a mouse. Every action has a single-key
  shortcut and the UI shows it (`OmaHintButton`, or an `OmaKeyHint` badge on a
  custom control; write chords in full, `Shift+1-9` not `Shift`). `Ctrl+Q` quits,
  `Escape` backs out / closes dialogs, dialogs also accept `Enter`/`Y`/`N`.

## Persistence
- `QSettings` (org "Omacom", app = game name → `~/.config/Omacom/<game>.conf`).
  Store complex state as one JSON string under a versioned key (`state/v1`).
- Tests must redirect it: `QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, tmpDir)`
  and `QSettings::setDefaultFormat(QSettings::IniFormat)` in `initTestCase`.
- Remember window geometry via QSettings (`window/geometry`).

## Style
- Match omacalc/omawrite: `m_` members, `QStringLiteral`, anonymous namespaces
  for helpers, const-correctness, no raw `new` without a parent. One class per
  file, files under ~300 lines. Comments explain *why*, not what. No dead code,
  no leftover `qDebug()`, no TODOs.
- Tests are not optional: every rule gets a test, with deterministic seeds.

## Git
- Atomic commits, one logical change each, message `"<game>: <imperative summary>"`
  (`"common: ..."` for shared code). Every commit builds and passes tests.
- Never commit `build/`, `build-tests/` or generated files.
- Never push to `main` directly; work on a branch and open a PR. Create branches
  with `git checkout -b <branch> --no-track origin/main` (or `git worktree add
  -b`), and leave the repo's `push.default=current` alone — a tracking branch
  plus `push -u` once fast-forwarded `main` by accident.
