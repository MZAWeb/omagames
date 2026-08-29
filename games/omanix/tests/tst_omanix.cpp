#include <QCoreApplication>
#include <QtTest>

#include "enginetests.h"

// One binary runs every suite so each area keeps its own small file.
int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("Omacom"));
    QCoreApplication::setApplicationName(QStringLiteral("omanix"));

    int status = 0;
    EngineTests engine;
    status |= QTest::qExec(&engine, argc, argv);
    return status;
}
