#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QUrl>

#include "appsetup.h"
#include "fieldview.h"
#include "omadropgame.h"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    OmarchyTheme *theme = OmaGames::setupApplication(
        app, {QStringLiteral("omadrop"), QStringLiteral("Omadrop")});
    qmlRegisterType<DropFieldView>("Omadrop", 1, 0, "DropFieldView");

    OmadropGame game(&app);
    QQmlApplicationEngine engine;
    OmaGames::setupEngine(engine, theme);
    engine.rootContext()->setContextProperty(QStringLiteral("game"), &game);
    engine.load(QUrl(QStringLiteral("qrc:/Main.qml")));
    return engine.rootObjects().isEmpty() ? -1 : app.exec();
}
