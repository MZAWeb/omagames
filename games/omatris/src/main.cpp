#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QUrl>

#include <cstdio>

#include "appsetup.h"
#include "fieldview.h"
#include "omatrisgame.h"
#include "replayplaylist.h"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    OmarchyTheme *theme = OmaGames::setupApplication(app, {QStringLiteral("omatris"), QStringLiteral("Omatris")});
    qmlRegisterType<FieldView>("Omatris", 1, 0, "FieldView");

    // Declared before the engine so it outlives the QML bindings on shutdown.
    OmatrisGame game(&app);

    // `--replay <file>` opens straight into watching an agent's game
    // (docs/AGENT-ENV.md); several open the first, to step through. One that
    // cannot be played is said so and nothing opens: a start screen would
    // look as if the file had been ignored.
    QString error;
    const auto replays = OmaGames::ReplayPlaylist::fromArguments(app.arguments(), &error);
    if (!replays || (!replays->isEmpty() && !game.loadReplays(*replays, &error))) {
        std::fprintf(stderr, "omatris: %s\n", qPrintable(error));
        return 2;
    }

    QQmlApplicationEngine engine;
    OmaGames::setupEngine(engine, theme);
    engine.rootContext()->setContextProperty(QStringLiteral("game"), &game);
    engine.load(QUrl(QStringLiteral("qrc:/Main.qml")));
    return engine.rootObjects().isEmpty() ? -1 : app.exec();
}
