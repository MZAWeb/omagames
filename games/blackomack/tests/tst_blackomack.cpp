#include <QCoreApplication>
#include <QtTest>

#include "bridgetests.h"
#include "cardtests.h"
#include "houseedgetests.h"
#include "insurancetests.h"
#include "persistencetests.h"
#include "ruletests.h"
#include "seatlayouttests.h"
#include "tabletests.h"

// One binary runs every suite so each area keeps its own small file. The
// settings the bridge writes go to a temporary directory for all of them.
int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("Omacom"));
    QCoreApplication::setApplicationName(QStringLiteral("blackomack-test"));
    QTemporaryDir settingsDir;
    if (!settingsDir.isValid())
        return 1;
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settingsDir.path());
    QSettings::setDefaultFormat(QSettings::IniFormat);

    int status = 0;
    CardTests cardTests;
    status |= QTest::qExec(&cardTests, argc, argv);
    RuleTests ruleTests;
    status |= QTest::qExec(&ruleTests, argc, argv);
    TableTests tableTests;
    status |= QTest::qExec(&tableTests, argc, argv);
    SeatLayoutTests seatLayoutTests;
    status |= QTest::qExec(&seatLayoutTests, argc, argv);
    InsuranceTests insuranceTests;
    status |= QTest::qExec(&insuranceTests, argc, argv);
    BridgeTests bridgeTests;
    status |= QTest::qExec(&bridgeTests, argc, argv);
    PersistenceTests persistenceTests;
    status |= QTest::qExec(&persistenceTests, argc, argv);
    HouseEdgeTests houseEdgeTests;
    status |= QTest::qExec(&houseEdgeTests, argc, argv);
    return status;
}
