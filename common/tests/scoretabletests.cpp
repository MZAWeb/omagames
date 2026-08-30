#include "scoretabletests.h"

#include <QJsonDocument>
#include <QSettings>
#include <QTemporaryDir>
#include <QtTest>

#include "scoretable.h"

using OmaGames::ScoreEntry;
using OmaGames::ScoreTable;

namespace {

const auto kScoresKey = QStringLiteral("scores/v1");
const QDate kDay(2026, 8, 30);

// Omanix's table: the top ten per difficulty, ranked on the score, with the
// level reached riding along.
ScoreTable scoreTable() {
    return ScoreTable({QStringLiteral("score"), QStringLiteral("level")}, 10,
                      ScoreTable::sameOrder({QStringLiteral("easy"), QStringLiteral("normal"),
                                             QStringLiteral("hard")},
                                            ScoreTable::HigherIsBetter));
}

// Omasweeper's: the five fastest per preset.
ScoreTable timeTable() {
    return ScoreTable({QStringLiteral("seconds")}, 5,
                      ScoreTable::sameOrder({QStringLiteral("beginner"), QStringLiteral("expert")},
                                            ScoreTable::LowerIsBetter));
}

QList<int> valuesIn(const ScoreTable &table, const QString &category) {
    QList<int> list;
    for (const ScoreEntry &entry : table.entries(category))
        list << entry.value;
    return list;
}

// The JSON a game had on disk before the extraction, so what people already
// played for is still there afterwards.
void seedSettings(const char *json) {
    QSettings settings;
    settings.setValue(kScoresKey, QString::fromUtf8(json));
    settings.sync();
}

}  // namespace

void ScoreTableTests::initTestCase() {
    static QTemporaryDir dir;
    QVERIFY(dir.isValid());
    m_settingsDir = dir.path();
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_settingsDir);
}

void ScoreTableTests::init() {
    QSettings settings;
    settings.clear();
    settings.sync();
}

void ScoreTableTests::higherIsBetterRanksTheBiggestFirst() {
    ScoreTable table = scoreTable();
    QCOMPARE(table.insert(QStringLiteral("easy"), {100, kDay, {}}), 0);
    QCOMPARE(table.insert(QStringLiteral("easy"), {300, kDay, {}}), 0);
    QCOMPARE(table.insert(QStringLiteral("easy"), {200, kDay, {}}), 1);
    QCOMPARE(valuesIn(table, QStringLiteral("easy")), QList<int>({300, 200, 100}));
}

void ScoreTableTests::lowerIsBetterRanksTheFastestFirst() {
    ScoreTable table = timeTable();
    QCOMPARE(table.insert(QStringLiteral("beginner"), {100, kDay, {}}), 0);
    QCOMPARE(table.insert(QStringLiteral("beginner"), {40, kDay, {}}), 0);
    QCOMPARE(table.insert(QStringLiteral("beginner"), {70, kDay, {}}), 1);
    QCOMPARE(valuesIn(table, QStringLiteral("beginner")), QList<int>({40, 70, 100}));
}

void ScoreTableTests::tiesRankBelowTheResultAlreadyThere() {
    ScoreTable scores = scoreTable();
    scores.insert(QStringLiteral("easy"), {500, kDay, {}});
    QCOMPARE(scores.insert(QStringLiteral("easy"), {500, kDay, {}}), 1);

    ScoreTable times = timeTable();
    times.insert(QStringLiteral("beginner"), {60, kDay, {}});
    QCOMPARE(times.insert(QStringLiteral("beginner"), {60, kDay, {}}), 1);
}

void ScoreTableTests::theTableStopsAtItsCap() {
    ScoreTable table = timeTable();
    for (int i = 1; i <= 8; ++i)
        table.insert(QStringLiteral("beginner"), {i, kDay, {}});
    QCOMPARE(table.entries(QStringLiteral("beginner")).size(), size_t(5));
    QCOMPARE(valuesIn(table, QStringLiteral("beginner")), QList<int>({1, 2, 3, 4, 5}));
}

void ScoreTableTests::aResultThatMissesTheCapIsNotKept() {
    ScoreTable table = timeTable();
    for (int i = 1; i <= 5; ++i)
        table.insert(QStringLiteral("beginner"), {i, kDay, {}});
    QCOMPARE(table.insert(QStringLiteral("beginner"), {99, kDay, {}}), -1);
    QCOMPARE(table.entries(QStringLiteral("beginner")).size(), size_t(5));
}

void ScoreTableTests::tablesAreKeptPerCategory() {
    ScoreTable table = scoreTable();
    table.insert(QStringLiteral("easy"), {100, kDay, {}});
    table.insert(QStringLiteral("hard"), {50, kDay, {}});
    QCOMPARE(valuesIn(table, QStringLiteral("easy")), QList<int>({100}));
    QCOMPARE(valuesIn(table, QStringLiteral("hard")), QList<int>({50}));
    QVERIFY(table.entries(QStringLiteral("normal")).empty());
}

void ScoreTableTests::anUnknownCategoryIsIgnored() {
    ScoreTable table = scoreTable();
    QCOMPARE(table.insert(QStringLiteral("nightmare"), {100, kDay, {}}), -1);
    QVERIFY(table.entries(QStringLiteral("nightmare")).empty());
    QCOMPARE(table.best(QStringLiteral("nightmare")), 0);
    QVERIFY(table.toVariantList(QStringLiteral("nightmare")).isEmpty());
}

void ScoreTableTests::bestIsTheTopOfTheTable() {
    ScoreTable scores = scoreTable();
    QCOMPARE(scores.best(QStringLiteral("easy")), 0);  // never played
    scores.insert(QStringLiteral("easy"), {100, kDay, {}});
    scores.insert(QStringLiteral("easy"), {700, kDay, {}});
    QCOMPARE(scores.best(QStringLiteral("easy")), 700);

    ScoreTable times = timeTable();
    times.insert(QStringLiteral("beginner"), {90, kDay, {}});
    times.insert(QStringLiteral("beginner"), {30, kDay, {}});
    QCOMPARE(times.best(QStringLiteral("beginner")), 30);
}

void ScoreTableTests::extraFieldsSurviveARoundTrip() {
    ScoreTable table = scoreTable();
    table.insert(QStringLiteral("easy"), {900, QDate(2026, 1, 2), {{QStringLiteral("level"), 6}}});

    ScoreTable read = scoreTable();
    read.fromJson(table.toJson());
    QCOMPARE(read.entries(QStringLiteral("easy")).size(), size_t(1));
    const ScoreEntry &entry = read.entries(QStringLiteral("easy")).front();
    QCOMPARE(entry.value, 900);
    QCOMPARE(entry.date, QDate(2026, 1, 2));
    QCOMPARE(entry.extra.value(QStringLiteral("level")).toInt(), 6);
}

void ScoreTableTests::everyCategoryRanksOnItsOwnField() {
    // Omatris: Marathon ranks on the score, Sprint on the clock, and both
    // keep every field.
    ScoreTable table({QStringLiteral("score"), QStringLiteral("millis")}, 10,
                     {{QStringLiteral("marathon"), ScoreTable::HigherIsBetter, {}},
                      {QStringLiteral("sprint"), ScoreTable::LowerIsBetter, QStringLiteral("millis")}});

    table.insert(QStringLiteral("marathon"), {100, kDay, {{QStringLiteral("millis"), 5000}}});
    table.insert(QStringLiteral("marathon"), {900, kDay, {{QStringLiteral("millis"), 1000}}});
    QCOMPARE(valuesIn(table, QStringLiteral("marathon")), QList<int>({900, 100}));

    table.insert(QStringLiteral("sprint"), {9000, kDay, {{QStringLiteral("score"), 100}}});
    table.insert(QStringLiteral("sprint"), {4000, kDay, {{QStringLiteral("score"), 80}}});
    QCOMPARE(valuesIn(table, QStringLiteral("sprint")), QList<int>({4000, 9000}));

    // On disk both tables carry both numbers under their own names.
    const QJsonObject json = table.toJson();
    const QJsonObject sprint = json.value(QStringLiteral("sprint")).toArray().at(0).toObject();
    QCOMPARE(sprint.value(QStringLiteral("millis")).toInt(), 4000);
    QCOMPARE(sprint.value(QStringLiteral("score")).toInt(), 80);
}

void ScoreTableTests::variantEntriesAreWhatQmlReads() {
    ScoreTable table = scoreTable();
    table.insert(QStringLiteral("easy"), {320, QDate(2026, 8, 30), {{QStringLiteral("level"), 4}}});

    const QVariantList list = table.toVariantList(QStringLiteral("easy"));
    QCOMPARE(list.size(), 1);
    const QVariantMap row = list.at(0).toMap();
    QCOMPARE(row.value(QStringLiteral("score")).toInt(), 320);
    QCOMPARE(row.value(QStringLiteral("level")).toInt(), 4);
    QCOMPARE(row.value(QStringLiteral("date")).toString(), QStringLiteral("2026-08-30"));
}

void ScoreTableTests::aStoredTableIsResortedAndTrimmed() {
    // Nothing guarantees the file was ours.
    ScoreTable table = timeTable();
    table.fromJson(QJsonDocument::fromJson(
                       R"({"beginner":[{"seconds":90,"date":"2026-08-30"},
                                       {"seconds":10,"date":"2026-08-30"},
                                       {"seconds":50,"date":"2026-08-30"},
                                       {"seconds":20,"date":"2026-08-30"},
                                       {"seconds":80,"date":"2026-08-30"},
                                       {"seconds":30,"date":"2026-08-30"}]})")
                       .object());
    QCOMPARE(valuesIn(table, QStringLiteral("beginner")), QList<int>({10, 20, 30, 50, 80}));
}

void ScoreTableTests::valuesBelowTheMinimumAreDropped() {
    ScoreTable table = timeTable();
    table.setMinimumValue(1);
    table.fromJson(QJsonDocument::fromJson(
                       R"({"beginner":[{"seconds":0,"date":"2026-08-30"},
                                       {"seconds":-4,"date":"2026-08-30"},
                                       {"seconds":25,"date":"2026-08-30"}]})")
                       .object());
    QCOMPARE(valuesIn(table, QStringLiteral("beginner")), QList<int>({25}));
}

void ScoreTableTests::savedTablesComeBackAfterARestart() {
    ScoreTable table = scoreTable();
    table.insert(QStringLiteral("normal"), {450, QDate(2026, 5, 6), {{QStringLiteral("level"), 3}}});
    table.save();

    ScoreTable reopened = scoreTable();
    reopened.load();
    QCOMPARE(valuesIn(reopened, QStringLiteral("normal")), QList<int>({450}));
    QCOMPARE(reopened.entries(QStringLiteral("normal")).front().extra.value(QStringLiteral("level")).toInt(), 3);
    QCOMPARE(reopened.toJson(), table.toJson());
}

// The five fixtures below are the literal output of each game's own toJson()
// before its table moved here, captured with a table of a few entries. They
// are what is sitting in ~/.config/Omacom/<game>.conf on a machine that has
// been playing, so reading them is the whole of the migration.

void ScoreTableTests::readsOmanixScores() {
    seedSettings(R"({"easy":[{"date":"2026-08-29","level":7,"score":4500},)"
                 R"({"date":"2026-08-30","level":3,"score":1200}],"hard":[],)"
                 R"("normal":[{"date":"2026-07-01","level":2,"score":800}]})");

    ScoreTable table = scoreTable();
    table.load();
    QCOMPARE(valuesIn(table, QStringLiteral("easy")), QList<int>({4500, 1200}));
    QCOMPARE(table.best(QStringLiteral("normal")), 800);
    QVERIFY(table.entries(QStringLiteral("hard")).empty());
    QCOMPARE(table.entries(QStringLiteral("easy")).front().extra.value(QStringLiteral("level")).toInt(), 7);
    QCOMPARE(table.entries(QStringLiteral("easy")).front().date, QDate(2026, 8, 29));
    // And it writes the same file back, so a downgrade loses nothing either.
    QCOMPARE(QJsonDocument(table.toJson()).toJson(QJsonDocument::Compact),
             QSettings().value(kScoresKey).toString().toUtf8());
}

void ScoreTableTests::readsOmasnakeScores() {
    seedSettings(R"({"classic-fast":[],"classic-normal":[{"date":"2026-08-30","length":24,"score":310},)"
                 R"({"date":"2026-08-28","length":11,"score":90}],"classic-slow":[],)"
                 R"("wrap-fast":[{"date":"2026-06-15","length":40,"score":770}],)"
                 R"("wrap-normal":[],"wrap-slow":[]})");

    ScoreTable table({QStringLiteral("score"), QStringLiteral("length")}, 10,
                     ScoreTable::sameOrder({QStringLiteral("classic-slow"), QStringLiteral("classic-normal"),
                                            QStringLiteral("classic-fast"), QStringLiteral("wrap-slow"),
                                            QStringLiteral("wrap-normal"), QStringLiteral("wrap-fast")},
                                           ScoreTable::HigherIsBetter));
    table.load();
    QCOMPARE(valuesIn(table, QStringLiteral("classic-normal")), QList<int>({310, 90}));
    QCOMPARE(table.best(QStringLiteral("wrap-fast")), 770);
    QCOMPARE(table.entries(QStringLiteral("wrap-fast")).front().extra.value(QStringLiteral("length")).toInt(), 40);
    QCOMPARE(QJsonDocument(table.toJson()).toJson(QJsonDocument::Compact),
             QSettings().value(kScoresKey).toString().toUtf8());
}

void ScoreTableTests::readsOmatrisScores() {
    seedSettings(R"({"marathon":[{"date":"2026-08-30","level":7,"lines":62,"millis":305000,"score":15400},)"
                 R"({"date":"2026-08-20","level":2,"lines":12,"millis":61000,"score":2200}],)"
                 R"("sprint":[{"date":"2026-08-26","level":4,"lines":40,"millis":78040,"score":2900},)"
                 R"({"date":"2026-08-25","level":5,"lines":40,"millis":92310,"score":3100}],)"
                 R"("zen":[{"date":"2026-05-02","level":1,"lines":8,"millis":44000,"score":640}]})");

    ScoreTable table({QStringLiteral("score"), QStringLiteral("lines"), QStringLiteral("level"),
                      QStringLiteral("millis")},
                     10,
                     {{QStringLiteral("marathon"), ScoreTable::HigherIsBetter, {}},
                      {QStringLiteral("sprint"), ScoreTable::LowerIsBetter, QStringLiteral("millis")},
                      {QStringLiteral("zen"), ScoreTable::HigherIsBetter, {}}});
    table.load();
    QCOMPARE(valuesIn(table, QStringLiteral("marathon")), QList<int>({15400, 2200}));
    // Sprint ranks on the clock, so the slower-scoring run is first.
    QCOMPARE(valuesIn(table, QStringLiteral("sprint")), QList<int>({78040, 92310}));
    QCOMPARE(table.best(QStringLiteral("sprint")), 78040);
    QCOMPARE(table.entries(QStringLiteral("sprint")).front().extra.value(QStringLiteral("score")).toInt(), 2900);
    QCOMPARE(QJsonDocument(table.toJson()).toJson(QJsonDocument::Compact),
             QSettings().value(kScoresKey).toString().toUtf8());
}

void ScoreTableTests::readsOmadokuTimes() {
    seedSettings(R"({"easy":[{"date":"2026-08-29","seconds":193},{"date":"2026-08-30","seconds":248}],)"
                 R"("extrahard":[],"hard":[{"date":"2026-07-04","seconds":1502}],"medium":[]})");

    ScoreTable table({QStringLiteral("seconds")}, 5,
                     ScoreTable::sameOrder({QStringLiteral("easy"), QStringLiteral("medium"),
                                            QStringLiteral("hard"), QStringLiteral("extrahard")},
                                           ScoreTable::LowerIsBetter));
    table.setMinimumValue(1);
    table.load();
    QCOMPARE(valuesIn(table, QStringLiteral("easy")), QList<int>({193, 248}));
    QCOMPARE(table.best(QStringLiteral("hard")), 1502);
    QCOMPARE(table.best(QStringLiteral("medium")), 0);
    QCOMPARE(QJsonDocument(table.toJson()).toJson(QJsonDocument::Compact),
             QSettings().value(kScoresKey).toString().toUtf8());
}

void ScoreTableTests::readsOmasweeperTimes() {
    seedSettings(R"({"beginner":[{"date":"2026-08-27","seconds":22},{"date":"2026-08-30","seconds":41}],)"
                 R"("expert":[{"date":"2026-03-09","seconds":602}],"intermediate":[]})");

    ScoreTable table({QStringLiteral("seconds")}, 5,
                     ScoreTable::sameOrder({QStringLiteral("beginner"), QStringLiteral("intermediate"),
                                            QStringLiteral("expert")},
                                           ScoreTable::LowerIsBetter));
    table.load();
    QCOMPARE(valuesIn(table, QStringLiteral("beginner")), QList<int>({22, 41}));
    QCOMPARE(table.best(QStringLiteral("expert")), 602);
    QCOMPARE(QJsonDocument(table.toJson()).toJson(QJsonDocument::Compact),
             QSettings().value(kScoresKey).toString().toUtf8());
}
