#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QUrl>

#include "appsetup.h"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    OmarchyTheme *theme = OmaGames::setupApplication(app, {QStringLiteral("oma2048"), QStringLiteral("Oma2048")});

    QQmlApplicationEngine engine;
    OmaGames::setupEngine(engine, theme);
    engine.load(QUrl(QStringLiteral("qrc:/Main.qml")));
    return engine.rootObjects().isEmpty() ? -1 : app.exec();
}
