#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QUrl>

#include "appsetup.h"
#include "fieldview.h"
#include "omasweepergame.h"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    OmarchyTheme *theme =
        OmaGames::setupApplication(app, {QStringLiteral("omasweeper"), QStringLiteral("Omasweeper")});
    qmlRegisterType<FieldView>("Omasweeper", 1, 0, "FieldView");

    // Declared before the engine so it outlives the QML bindings on shutdown.
    OmasweeperGame game(&app);

    QQmlApplicationEngine engine;
    OmaGames::setupEngine(engine, theme);
    engine.rootContext()->setContextProperty(QStringLiteral("game"), &game);
    engine.load(QUrl(QStringLiteral("qrc:/Main.qml")));
    return engine.rootObjects().isEmpty() ? -1 : app.exec();
}
