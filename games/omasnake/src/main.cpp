#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QUrl>

#include <cstdio>

#include "appsetup.h"
#include "fieldview.h"
#include "omasnakegame.h"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    OmarchyTheme *theme = OmaGames::setupApplication(app, {QStringLiteral("omasnake"), QStringLiteral("Omasnake")});
    qmlRegisterType<FieldView>("Omasnake", 1, 0, "FieldView");

    // Declared before the engine so it outlives the QML bindings on shutdown.
    OmasnakeGame game(&app);

    // `--replay <file>` opens straight into watching an agent's game
    // (docs/AGENT-ENV.md). One that cannot be played is said so and nothing
    // opens: a start screen would look as if the file had been ignored.
    const QStringList args = app.arguments();
    if (const int replayArg = args.indexOf(QStringLiteral("--replay")); replayArg >= 0) {
        QString error = QStringLiteral("--replay needs a file");
        if (replayArg + 1 >= args.size() || !game.loadReplay(args.at(replayArg + 1), &error)) {
            std::fprintf(stderr, "omasnake: %s\n", qPrintable(error));
            return 2;
        }
    }

    QQmlApplicationEngine engine;
    OmaGames::setupEngine(engine, theme);
    engine.rootContext()->setContextProperty(QStringLiteral("game"), &game);
    engine.load(QUrl(QStringLiteral("qrc:/Main.qml")));
    return engine.rootObjects().isEmpty() ? -1 : app.exec();
}
