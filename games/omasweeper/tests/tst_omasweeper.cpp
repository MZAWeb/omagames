#include <QCoreApplication>
#include <QtTest>

#include "boardtests.h"
#include "bridgetests.h"
#include "generatortests.h"
#include "solvertests.h"

// One binary runs every suite so each area keeps its own small file.
int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("Omacom"));
    QCoreApplication::setApplicationName(QStringLiteral("omasweeper"));

    int status = 0;
    BoardTests board;
    status |= QTest::qExec(&board, argc, argv);
    SolverTests solver;
    status |= QTest::qExec(&solver, argc, argv);
    GeneratorTests generator;
    status |= QTest::qExec(&generator, argc, argv);
    BridgeTests bridge;
    status |= QTest::qExec(&bridge, argc, argv);
    return status;
}
