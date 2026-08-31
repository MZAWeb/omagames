#include <QGuiApplication>
#include <QtTest>

#include "bridgetests.h"
#include "fieldtests.h"
#include "movertests.h"
#include "painttests.h"
#include "playtests.h"
#include "scoringtests.h"

// One binary runs every suite so each area keeps its own small file.
int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    QGuiApplication::setOrganizationName(QStringLiteral("Omacom"));
    QGuiApplication::setApplicationName(QStringLiteral("omanix"));

    int status = 0;
    FieldTests field;
    status |= QTest::qExec(&field, argc, argv);
    MoverTests movers;
    status |= QTest::qExec(&movers, argc, argv);
    PaintTests paint;
    status |= QTest::qExec(&paint, argc, argv);
    PlayTests play;
    status |= QTest::qExec(&play, argc, argv);
    ScoringTests scoring;
    status |= QTest::qExec(&scoring, argc, argv);
    BridgeTests bridge;
    status |= QTest::qExec(&bridge, argc, argv);
    return status;
}
