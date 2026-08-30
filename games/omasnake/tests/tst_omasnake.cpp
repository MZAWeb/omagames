#include <QCoreApplication>
#include <QtTest>

#include "bridgetests.h"
#include "foodtests.h"
#include "snaketests.h"
#include "speedtests.h"

// One binary runs every suite so each area keeps its own small file.
int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("Omacom"));
    QCoreApplication::setApplicationName(QStringLiteral("omasnake"));

    int status = 0;
    SnakeTests snake;
    status |= QTest::qExec(&snake, argc, argv);
    FoodTests food;
    status |= QTest::qExec(&food, argc, argv);
    SpeedTests speed;
    status |= QTest::qExec(&speed, argc, argv);
    BridgeTests bridge;
    status |= QTest::qExec(&bridge, argc, argv);
    return status;
}
