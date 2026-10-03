#include <QCoreApplication>
#include <QtTest>

#include "autoshifttests.h"
#include "boardmetricstests.h"
#include "boardtests.h"
#include "challengetests.h"
#include "difficultytests.h"
#include "handlingtests.h"
#include "lockdelaytests.h"
#include "inputtests.h"
#include "persistencetests.h"
#include "piecetests.h"
#include "placementtests.h"
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
    LockDelayTests lockDelay;
    status |= QTest::qExec(&lockDelay, argc, argv);
    ScoringTests scoring;
    status |= QTest::qExec(&scoring, argc, argv);
    ChallengeTests challenge;
    status |= QTest::qExec(&challenge, argc, argv);
    DifficultyTests difficulty;
    status |= QTest::qExec(&difficulty, argc, argv);
    AutoShiftTests autoShift;
    status |= QTest::qExec(&autoShift, argc, argv);
    HandlingTests handling;
    status |= QTest::qExec(&handling, argc, argv);
    InputTests input;
    status |= QTest::qExec(&input, argc, argv);
    PersistenceTests persistence;
    status |= QTest::qExec(&persistence, argc, argv);
    BoardMetricsTests metrics;
    status |= QTest::qExec(&metrics, argc, argv);
    PlacementTests placements;
    status |= QTest::qExec(&placements, argc, argv);
    return status;
}
