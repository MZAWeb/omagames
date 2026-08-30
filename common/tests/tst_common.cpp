#include <QCoreApplication>
#include <QtTest>

#include "pacertests.h"
#include "scoretabletests.h"
#include "themetests.h"
#include "windowgeometrytests.h"

// One binary runs every suite so each area keeps its own small file.
int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("Omacom"));
    QCoreApplication::setApplicationName(QStringLiteral("common"));

    int status = 0;
    OmarchyThemeTest theme;
    status |= QTest::qExec(&theme, argc, argv);
    ScoreTableTests scores;
    status |= QTest::qExec(&scores, argc, argv);
    WindowGeometryTests geometry;
    status |= QTest::qExec(&geometry, argc, argv);
    PacerTests pacer;
    status |= QTest::qExec(&pacer, argc, argv);
    return status;
}
