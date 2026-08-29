#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QUrl>
#include <QtQml>

#include "appsetup.h"
#include "sudokugame.h"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    OmarchyTheme *theme = OmaGames::setupApplication(app, {QStringLiteral("omadoku"), QStringLiteral("Omadoku")});

    SudokuGame game;
    // Registered only so QML can name the State and Level enums; the instance
    // itself comes in as a context property.
    qmlRegisterUncreatableType<SudokuGame>("Omadoku", 1, 0, "SudokuGame",
                                           QStringLiteral("SudokuGame is provided as the `game` context property"));

    QQmlApplicationEngine engine;
    OmaGames::setupEngine(engine, theme);
    engine.rootContext()->setContextProperty(QStringLiteral("game"), &game);
    engine.load(QUrl(QStringLiteral("qrc:/Main.qml")));
    return engine.rootObjects().isEmpty() ? -1 : app.exec();
}
