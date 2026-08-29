#pragma once

#include <QString>

class QGuiApplication;
class QQmlApplicationEngine;
class OmarchyTheme;

// One-call bootstrap shared by every game so main.cpp stays ~20 lines:
//
//   QGuiApplication app(argc, argv);
//   OmarchyTheme *theme = OmaGames::setupApplication(app, {"omadoku", "Omadoku"});
//   QQmlApplicationEngine engine;
//   OmaGames::setupEngine(engine, theme);
//   engine.rootContext()->setContextProperty("game", &game);
//   engine.load(QUrl("qrc:/Main.qml"));
namespace OmaGames {

struct AppInfo {
    QString name;         // binary / desktop file / QSettings name, e.g. "omadoku"
    QString displayName;  // window title, e.g. "Omadoku"
};

// Sets application metadata (organization "Omacom" like the other Omarchy apps,
// so QSettings lands in ~/.config/Omacom/<name>.conf), loads the bundled
// iA Writer Mono font, picks the Material style, and wires SystemTheme
// (portal dark mode + text scale) into a new OmarchyTheme owned by `app`.
// Honors `--theme-file <path>` on the command line for testing themes.
OmarchyTheme *setupApplication(QGuiApplication &app, const AppInfo &info);

// Adds the shared QML import path (`import OmaGames`), exposes `theme` as a
// context property and logs QML warnings to stderr.
void setupEngine(QQmlApplicationEngine &engine, OmarchyTheme *theme);

}
