#include <QCoreApplication>
#include <QtTest>

#include "autoshifttests.h"
#include "boardtests.h"
#include "bridgetests.h"
#include "piecetests.h"
#include "scoringtests.h"

// One binary runs every suite so each area keeps its own small file.
int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("Omacom"));
    QCoreApplication::setApplicationName(QStringLiteral("omatris"));

    int status = 0;
    PieceTests pieces;
    status |= QTest::qExec(&pieces, argc, argv);
    BoardTests board;
    status |= QTest::qExec(&board, argc, argv);
    ScoringTests scoring;
    status |= QTest::qExec(&scoring, argc, argv);
    AutoShiftTests autoShift;
    status |= QTest::qExec(&autoShift, argc, argv);
    BridgeTests bridge;
    status |= QTest::qExec(&bridge, argc, argv);
    return status;
}
