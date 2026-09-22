#include "handlingtests.h"

#include <QtTest>

#include "autoshift.h"
#include "handling.h"
#include "rules.h"

namespace {

// Steps a setting as far as it goes and reports how many steps that took.
int stepToTheEnd(Handling &handling, Handling::Setting setting, int delta) {
    int steps = 0;
    while (handling.step(setting, delta))
        ++steps;
    return steps;
}

QVariantMap row(const Handling &handling, int index) {
    return handling.rows().at(index).toMap();
}

}  // namespace

void HandlingTests::defaultsAreTheValuesOmatrisAlwaysPlayedWith() {
    const Handling handling = Handling::defaults();
    QCOMPARE(handling.dasTicks, AutoShift::kDelayTicks);
    QCOMPARE(handling.arrTicks, AutoShift::kRepeatTicks);
    QCOMPARE(handling.softDropFactor, Rules::kSoftDropFactor);
    QVERIFY(handling.isDefault());
    // The numbers the panel shows for them.
    QCOMPARE(Handling::ticksToMs(handling.dasTicks), 167);
    QCOMPARE(Handling::ticksToMs(handling.arrTicks), 33);
}

void HandlingTests::delayAndRepeatStopAtTheEndsOfTheirRange() {
    Handling handling = Handling::defaults();
    QCOMPARE(stepToTheEnd(handling, Handling::Das, -1), AutoShift::kDelayTicks - Handling::kMinDasTicks);
    QCOMPARE(handling.dasTicks, Handling::kMinDasTicks);
    QVERIFY(!handling.canStep(Handling::Das, -1));
    QVERIFY(!handling.isDefault());
    QCOMPARE(stepToTheEnd(handling, Handling::Das, 1), Handling::kMaxDasTicks - Handling::kMinDasTicks);
    QCOMPARE(handling.dasTicks, Handling::kMaxDasTicks);

    QCOMPARE(stepToTheEnd(handling, Handling::Arr, -1), AutoShift::kRepeatTicks);
    QCOMPARE(handling.arrTicks, 0);
    QCOMPARE(stepToTheEnd(handling, Handling::Arr, 1), Handling::kMaxArrTicks);
    // A refused step changes nothing, and neither does a zero one.
    const Handling before = handling;
    QVERIFY(!handling.step(Handling::Arr, 1));
    QVERIFY(!handling.step(Handling::Das, 0));
    QVERIFY(handling == before);
}

void HandlingTests::softDropStepsThroughItsSpeedsToInstant() {
    Handling handling = Handling::defaults();
    QVERIFY(handling.step(Handling::SoftDrop, 1));
    QCOMPARE(handling.softDropFactor, 40);
    QVERIFY(handling.step(Handling::SoftDrop, 1));
    QCOMPARE(handling.softDropFactor, Handling::kInstantSoftDrop);
    QVERIFY(!handling.step(Handling::SoftDrop, 1));

    handling = Handling::defaults();
    QVERIFY(handling.step(Handling::SoftDrop, -1));
    QCOMPARE(handling.softDropFactor, 10);
    QVERIFY(handling.step(Handling::SoftDrop, -1));
    QCOMPARE(handling.softDropFactor, 5);
    QVERIFY(!handling.canStep(Handling::SoftDrop, -1));
}

void HandlingTests::storedFormRoundTripsAndRejectsNonsense() {
    Handling tuned = Handling::defaults();
    tuned.dasTicks = 6;
    tuned.arrTicks = 0;
    tuned.softDropFactor = Handling::kInstantSoftDrop;
    QVERIFY(Handling::fromJson(tuned.toJson()) == tuned);

    // Nothing stored is the defaults.
    QVERIFY(Handling::fromJson(QJsonObject()).isDefault());
    // Each value out of its range, or not one of the steps, falls back on its
    // own; the good one survives.
    const Handling mixed = Handling::fromJson(
        {{QStringLiteral("das"), 999}, {QStringLiteral("arr"), 3}, {QStringLiteral("softDrop"), 7}});
    QCOMPARE(mixed.dasTicks, AutoShift::kDelayTicks);
    QCOMPARE(mixed.arrTicks, 3);
    QCOMPARE(mixed.softDropFactor, Rules::kSoftDropFactor);
    QVERIFY(Handling::fromJson({{QStringLiteral("das"), 0}, {QStringLiteral("arr"), -1}}).isDefault());
    QVERIFY(Handling::fromJson({{QStringLiteral("das"), QStringLiteral("fast")}}).isDefault());
}

void HandlingTests::rowsDescribeEachSetting() {
    Handling handling = Handling::defaults();
    QCOMPARE(handling.rows().size(), Handling::kSettingCount);
    QCOMPARE(row(handling, 0).value(QStringLiteral("id")).toString(), QStringLiteral("das"));
    QCOMPARE(row(handling, 1).value(QStringLiteral("id")).toString(), QStringLiteral("arr"));
    QCOMPARE(row(handling, 2).value(QStringLiteral("id")).toString(), QStringLiteral("softDrop"));
    QCOMPARE(row(handling, 0).value(QStringLiteral("value")).toString(), QStringLiteral("167 ms"));
    QCOMPARE(row(handling, 1).value(QStringLiteral("value")).toString(), QStringLiteral("33 ms"));
    QCOMPARE(row(handling, 2).value(QStringLiteral("value")).toString(), QStringLiteral("20×"));
    for (int i = 0; i < Handling::kSettingCount; ++i) {
        QVERIFY(!row(handling, i).value(QStringLiteral("label")).toString().isEmpty());
        QVERIFY(!row(handling, i).value(QStringLiteral("description")).toString().isEmpty());
        QVERIFY(row(handling, i).value(QStringLiteral("isDefault")).toBool());
        QVERIFY(row(handling, i).value(QStringLiteral("canLower")).toBool());
        QVERIFY(row(handling, i).value(QStringLiteral("canRaise")).toBool());
    }

    stepToTheEnd(handling, Handling::Arr, -1);
    stepToTheEnd(handling, Handling::SoftDrop, 1);
    QCOMPARE(row(handling, 1).value(QStringLiteral("value")).toString(), QStringLiteral("Instant"));
    QVERIFY(!row(handling, 1).value(QStringLiteral("canLower")).toBool());
    QVERIFY(!row(handling, 1).value(QStringLiteral("isDefault")).toBool());
    QCOMPARE(row(handling, 2).value(QStringLiteral("value")).toString(), QStringLiteral("Instant"));
    QVERIFY(!row(handling, 2).value(QStringLiteral("canRaise")).toBool());
    // The untouched setting still reads as its default.
    QVERIFY(row(handling, 0).value(QStringLiteral("isDefault")).toBool());
}
