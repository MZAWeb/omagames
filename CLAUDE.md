# Omagames — rules for anyone (human or agent) working in this repo

Read `docs/PLAN.md` first. If you are implementing a game, your brief is in
`docs/tasks/<game>.md`; follow it and stay inside your game's directory unless
the brief says otherwise.

## Stack
- Qt 6.11, C++17, QML (Qt Quick Controls 2, Material style), qmake. No CMake, no extra deps.
- Build with `bin/build <game>`, test with `bin/test <game>`, run with `bin/run <game>`.
  Every change must leave `bin/build` and `bin/test` green.

## Architecture rule: logic first, UI is replaceable
- All game logic lives in plain C++ classes with **no Qt GUI/QML dependency**
  (QtCore only) under `games/<game>/src/`. They must be fully testable with
  QtTest headlessly (`QTEST_GUILESS_MAIN`).
- One `QObject` "game" class (e.g. `SudokuGame`, `BlackjackGame`) is the *only*
  bridge to QML: it exposes state via `Q_PROPERTY`/models and actions via
  `Q_INVOKABLE`. QML never contains rules; it only renders state and calls actions.
- The UI must be swappable later without touching the engine. Keep QML free of
  hardcoded colors/fonts: use the `theme` context property (`OmarchyTheme`) and
  `theme.textScale` for sizes. Shared controls come from `import OmaGames`.

## Shared code (`common/`)
- `OmarchyTheme` (`theme` in QML): every color from Omarchy's `colors.toml`,
  `darkMode`, `textScale`, plus `theme.mix(a, b, t)` and `theme.alpha(c, a)`.
- `OmaGames::setupApplication` / `setupEngine`: see `common/src/appsetup.h`.
- QML module `OmaGames`: `OmaButton`, `OmaPanel`. Add new *generic* controls
  there (register in `common/qml/OmaGames/qmldir` **and** `common/common.qrc`);
  game-specific controls stay in the game.
- Coordinate before changing existing common/ APIs — other games use them.

## Persistence
- `QSettings` (org "Omacom", app = game name → `~/.config/Omacom/<game>.conf`).
  Tests must redirect it: `QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, tmpDir)`
  and `QSettings::setDefaultFormat(QSettings::IniFormat)` in `initTestCase` (see omacalc tests).

## Style
- Match omacalc/omawrite: `m_` members, `QStringLiteral`, brief comments that
  explain *why*. Keep files small; one class per file.
- Keyboard first: everything reachable without a mouse; `Ctrl+Q` quits; `Escape` backs out.
- Window: remember geometry via QSettings like omacalc (`window/geometry`).
