#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QUrl>

#include "appsetup.h"
#include "oma2048game.h"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    OmarchyTheme *theme = OmaGames::setupApplication(app, {QStringLiteral("oma2048"), QStringLiteral("Oma2048")});

    // Declared before the engine so it outlives the QML bindings on shutdown.
    Oma2048Game game(&app);

    QQmlApplicationEngine engine;
    OmaGames::setupEngine(engine, theme);
    engine.rootContext()->setContextProperty(QStringLiteral("game"), &game);
    engine.load(QUrl(QStringLiteral("qrc:/Main.qml")));
    return engine.rootObjects().isEmpty() ? -1 : app.exec();
}
