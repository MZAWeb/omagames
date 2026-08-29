#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QUrl>

#include "appsetup.h"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    OmarchyTheme *theme = OmaGames::setupApplication(app, {QStringLiteral("blackomack"), QStringLiteral("Black Omack")});

    QQmlApplicationEngine engine;
    OmaGames::setupEngine(engine, theme);
    // TODO(agent): expose the game object, e.g.
    // engine.rootContext()->setContextProperty("game", &game);
    engine.load(QUrl(QStringLiteral("qrc:/Main.qml")));
    return engine.rootObjects().isEmpty() ? -1 : app.exec();
}
