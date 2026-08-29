#include <QCoreApplication>
#include <QtTest>

#include "solvertests.h"

// One binary runs every suite so each area keeps its own small file.
int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("Omacom"));
    QCoreApplication::setApplicationName(QStringLiteral("omadoku"));

    int status = 0;
    SolverTests solver;
    status |= QTest::qExec(&solver, argc, argv);
    return status;
}
