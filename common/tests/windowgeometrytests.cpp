#include "windowgeometrytests.h"

#include <QSettings>
#include <QTemporaryDir>
#include <QtTest>

#include "windowgeometry.h"

namespace WG = OmaGames::WindowGeometry;

void WindowGeometryTests::initTestCase() {
    static QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, dir.path());
}

void WindowGeometryTests::init() {
    QSettings settings;
    settings.clear();
    settings.sync();
}

void WindowGeometryTests::nothingSavedYetIsInvalidRatherThanZeroed() {
    QVERIFY(!WG::rect().isValid());
    QVERIFY(!WG::maximized());
    const QVariantMap map = WG::toVariantMap();
    QCOMPARE(map.value(QStringLiteral("valid")).toBool(), false);
    QCOMPARE(map.value(QStringLiteral("maximized")).toBool(), false);
}

void WindowGeometryTests::aSavedRectComesBack() {
    WG::save(QRect(120, 60, 900, 640), false);
    QCOMPARE(WG::rect(), QRect(120, 60, 900, 640));

    const QVariantMap map = WG::toVariantMap();
    QCOMPARE(map.value(QStringLiteral("valid")).toBool(), true);
    QCOMPARE(map.value(QStringLiteral("x")).toInt(), 120);
    QCOMPARE(map.value(QStringLiteral("y")).toInt(), 60);
    QCOMPARE(map.value(QStringLiteral("width")).toInt(), 900);
    QCOMPARE(map.value(QStringLiteral("height")).toInt(), 640);
    QCOMPARE(map.value(QStringLiteral("maximized")).toBool(), false);
}

void WindowGeometryTests::negativePositionsSurvive() {
    // A monitor left of or above the primary is a legitimate place to be.
    WG::save(QRect(-1200, -80, 800, 600), false);
    const QVariantMap map = WG::toVariantMap();
    QCOMPARE(map.value(QStringLiteral("valid")).toBool(), true);
    QCOMPARE(map.value(QStringLiteral("x")).toInt(), -1200);
    QCOMPARE(map.value(QStringLiteral("y")).toInt(), -80);
}

void WindowGeometryTests::maximizedIsRememberedSeparately() {
    WG::save(QRect(0, 0, 1024, 768), true);
    QVERIFY(WG::maximized());
    QCOMPARE(WG::rect(), QRect(0, 0, 1024, 768));
    QCOMPARE(WG::toVariantMap().value(QStringLiteral("maximized")).toBool(), true);

    // The windowed rect is what is stored, whatever the flag says.
    WG::save(QRect(0, 0, 1024, 768), false);
    QVERIFY(!WG::maximized());
}
