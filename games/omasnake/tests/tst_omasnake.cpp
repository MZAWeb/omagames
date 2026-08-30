#include <QCoreApplication>
#include <QtTest>

#include "bridgetests.h"
#include "enginetests.h"

// One binary runs every suite so each area keeps its own small file.
int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("Omacom"));
    QCoreApplication::setApplicationName(QStringLiteral("omasnake"));

    int status = 0;
    EngineTests engine;
    status |= QTest::qExec(&engine, argc, argv);
    BridgeTests bridge;
    status |= QTest::qExec(&bridge, argc, argv);
    return status;
}
