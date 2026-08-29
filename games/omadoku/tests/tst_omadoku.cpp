#include <QCoreApplication>
#include <QtTest>

#include "boardtests.h"
#include "gametests.h"
#include "generatortests.h"
#include "persistencetests.h"
#include "gradertests.h"
#include "solvertests.h"

// One binary runs every suite so each area keeps its own small file.
int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("Omacom"));
    QCoreApplication::setApplicationName(QStringLiteral("omadoku"));

    int status = 0;
    SolverTests solver;
    status |= QTest::qExec(&solver, argc, argv);
    GeneratorTests generator;
    status |= QTest::qExec(&generator, argc, argv);
    GraderTests grader;
    status |= QTest::qExec(&grader, argc, argv);
    BoardTests board;
    status |= QTest::qExec(&board, argc, argv);
    GameTests gameTests;
    status |= QTest::qExec(&gameTests, argc, argv);
    PersistenceTests persistence;
    status |= QTest::qExec(&persistence, argc, argv);
    return status;
}
