#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QUrl>

#include "appsetup.h"
#include "sudokugame.h"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    OmarchyTheme *theme = OmaGames::setupApplication(app, {QStringLiteral("omadoku"), QStringLiteral("Omadoku")});

    // Declared before the engine so it outlives it: QML bindings still touch
    // `game` while the engine tears its objects down.
    SudokuGame game;

    QQmlApplicationEngine engine;
    OmaGames::setupEngine(engine, theme);
    engine.rootContext()->setContextProperty(QStringLiteral("game"), &game);
    engine.load(QUrl(QStringLiteral("qrc:/Main.qml")));
    return engine.rootObjects().isEmpty() ? -1 : app.exec();
}
