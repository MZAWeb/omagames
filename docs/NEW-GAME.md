# Adding a new game

Pick a short lowercase name (`<game>`, e.g. `omachess`) and a display name.

## 1. Scaffold

```
games/<game>/
  <game>.pro
  src/main.cpp  src/Main.qml  src/resources.qrc
  tests/tests.pro  tests/tst_<game>.cpp
  pkgbuild/PKGBUILD  <game>.desktop  <game>.install  <game>.svg
  README.md
```

Copy these from an existing game and rename; the interesting parts are:

`<game>.pro`
```qmake
include(../../common/common.pri)
CONFIG += c++17 release
TARGET = <game>
TEMPLATE = app
HEADERS += src/<engine>.h src/<game>game.h
SOURCES += src/main.cpp src/<engine>.cpp src/<game>game.cpp
RESOURCES += src/resources.qrc
```

`src/main.cpp`
```cpp
QGuiApplication app(argc, argv);
OmarchyTheme *theme = OmaGames::setupApplication(app, {QStringLiteral("<game>"), QStringLiteral("<Display Name>")});
<Game>Game game;
QQmlApplicationEngine engine;
OmaGames::setupEngine(engine, theme);
engine.rootContext()->setContextProperty(QStringLiteral("game"), &game);
engine.load(QUrl(QStringLiteral("qrc:/Main.qml")));
```

`tests/tests.pro` includes `../../../common/common-tests.pri`, adds `QT += testlib`,
lists the engine + bridge sources, and the test uses `QTEST_GUILESS_MAIN`.

Then add the game to `omagames.pro`'s `SUBDIRS`. `bin/build <game>`,
`bin/test <game>`, `bin/run <game>` and `bin/install <game>` work immediately.

## 2. Write the spec first

`games/<game>/README.md` is the spec and stays the user-facing documentation:
what the game is, the exact rules/variant implemented, settings, keyboard
shortcuts, install line. Decide the rules before writing code.

## 3. Engine, then bridge, then UI

1. **Engine** (`src/*.h/.cpp`, QtCore only, no QObject): rules, generators,
   AI. Deterministic with explicit seeds. Step-wise API for anything that needs
   pacing. Write the tests alongside — every rule in the README gets a test.
2. **Bridge** (`src/<game>game.h/.cpp`, one QObject): `Q_PROPERTY` state,
   models, `Q_INVOKABLE` actions, and the shared pieces rather than your own:

   - pacing with `OmaGames::Pacer` (`Repeating`, or `SingleShot` when the step
     decides whether to schedule the next one), its `interval` a property so
     tests can set it to 0;
   - kept results with `OmaGames::ScoreTable` — name the integers an entry
     carries, the category ids, the cap and whether higher or lower is better,
     and it does the ordering, the ranks and the JSON under `scores/v1`;
   - the window's place with `OmaGames::WindowGeometry`;
   - anything else as JSON under a versioned QSettings key (`state/v1`).

   ```cpp
   // In the .cpp, where the ids and the shape of a result live:
   OmaGames::ScoreTable scoreTable() {   // the top ten per difficulty,
       return OmaGames::ScoreTable(      // ranked on the score, level alongside
           {QStringLiteral("score"), QStringLiteral("level")}, 10,
           OmaGames::ScoreTable::sameOrder({kEasyId, kNormalId, kHardId},
                                           OmaGames::ScoreTable::HigherIsBetter));
   }

   MyGame::MyGame(QObject *parent)
       : QObject(parent), m_scores(scoreTable()),
         m_pacer(OmaGames::Pacer::Repeating, [this]() { step(); }, this) {
       m_pacer.setInterval(kDefaultStepIntervalMs);
       m_scores.load();
   }

   // -1 when the run did not make the table.
   const int rank = m_scores.insert(
       difficultyId(m_difficulty),
       {score, QDate::currentDate(), {{QStringLiteral("level"), level}}});
   ```
3. **UI** (`src/*.qml`, small components): renders the bridge's state, no rules,
   colors and sizes only from `theme`. Reuse `import OmaGames` controls before
   writing your own: `OmaOverlayPanel` is the shell of every overlay,
   `OmaConfirmDialog` the yes/no prompt, `OmaPauseOverlay` the paused screen,
   `OmaKeyLegend` the row of keycaps, `OmaScoresPanel` the kept tables,
   `OmaBonusPopup` a label that rises and fades. A game's own `KeyLegend.qml`
   should be `OmaKeyLegend { model: [...] }` and nothing else. If you build
   something generic, put it in `common/qml/OmaGames/` and register it in
   `qmldir` and `common/common.qrc`.
   Every action gets a key and shows it: use `OmaHintButton` (an `OmaButton`
   with an `OmaKeyHint` badge) for buttons and `OmaKeyHint` on custom controls.

## 4. Finish

- Icon: flat SVG, rounded square, one glyph, same style as the other games.
- `pkgbuild/PKGBUILD`: only paths/names change; verify with `makepkg -f` in `pkgbuild/`.
- Add the game to the table in the root `README.md` (and a screenshot).
- From a clean state: `rm -rf build build-tests && bin/build && bin/test`.
- Nothing else to register: CI, `bin/test`, `install.sh` and `bin/opr-pkgbuilds`
  discover games from `games/*`.
