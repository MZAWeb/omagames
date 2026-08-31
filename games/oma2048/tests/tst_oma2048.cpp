#include <QCoreApplication>
#include <QtTest>

#include "boardtests.h"
#include "gametests.h"

// One binary runs every suite so each area keeps its own small file.
int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("Omacom"));
    QCoreApplication::setApplicationName(QStringLiteral("oma2048"));

    int status = 0;
    BoardTests board;
    status |= QTest::qExec(&board, argc, argv);
    GameTests game;
    status |= QTest::qExec(&game, argc, argv);
    return status;
}
