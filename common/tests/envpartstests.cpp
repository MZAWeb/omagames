#include "envpartstests.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <QtTest>

#include "envconfig.h"
#include "observationlayout.h"
#include "replay.h"
#include "replaypace.h"
#include "replayplaylist.h"

using OmaGames::ObservationLayout;
using OmaGames::Replay;

namespace {

QJsonObject schema() {
    return {
        {QStringLiteral("mode"),
         QJsonObject{{QStringLiteral("default"), QStringLiteral("marathon")},
                     {QStringLiteral("choices"), QJsonArray{QStringLiteral("marathon"), QStringLiteral("sprint")}}}},
        {QStringLiteral("frame_skip"),
         QJsonObject{{QStringLiteral("default"), 1}, {QStringLiteral("min"), 1}, {QStringLiteral("max"), 60}}},
        {QStringLiteral("ghost"), QJsonObject{{QStringLiteral("default"), true}}},
    };
}

QString refusal(const QJsonObject &given) {
    QString error;
    const auto resolved = OmaGames::EnvConfig::resolve(schema(), given, &error);
    return resolved ? QString() : error;
}

}  // namespace

void EnvPartsTests::layoutAlignsEveryTensor() {
    ObservationLayout layout;
    QCOMPARE(layout.add(QStringLiteral("cells"), ObservationLayout::DType::U8, {3, 3}), 0);
    QCOMPARE(layout.add(QStringLiteral("score"), ObservationLayout::DType::I32, {1}), 1);
    QCOMPARE(layout.add(QStringLiteral("odds"), ObservationLayout::DType::F32, {2}), 2);
    // Nine bytes round up to sixteen, so the int that follows is aligned,
    // and the total rounds up too, so every row of a batch stays aligned.
    QCOMPARE(layout.size(), 32);
    const QJsonArray tensors = layout.toJson().value(QStringLiteral("tensors")).toArray();
    QCOMPARE(tensors.size(), 3);
    QCOMPARE(tensors[0].toObject().value(QStringLiteral("offset")).toInt(), 0);
    QCOMPARE(tensors[0].toObject().value(QStringLiteral("shape")).toArray(), (QJsonArray{3, 3}));
    QCOMPARE(tensors[1].toObject().value(QStringLiteral("offset")).toInt(), 16);
    QCOMPARE(tensors[1].toObject().value(QStringLiteral("dtype")).toString(), QStringLiteral("int32"));
    QCOMPARE(tensors[2].toObject().value(QStringLiteral("offset")).toInt(), 24);
    QCOMPARE(tensors[2].toObject().value(QStringLiteral("dtype")).toString(), QStringLiteral("float32"));
    QCOMPARE(layout.toJson().value(QStringLiteral("size")).toInt(), 32);
}

void EnvPartsTests::layoutWritesWhereItSays() {
    ObservationLayout layout;
    const int cells = layout.add(QStringLiteral("cells"), ObservationLayout::DType::U8, {5});
    const int score = layout.add(QStringLiteral("score"), ObservationLayout::DType::I32, {1});
    std::vector<std::byte> buffer(size_t(layout.size()));
    layout.at<quint8>(buffer.data(), cells)[4] = 7;
    *layout.at<qint32>(buffer.data(), score) = 1234;
    QCOMPARE(int(buffer[4]), 7);
    qint32 read = 0;
    std::memcpy(&read, buffer.data() + 8, sizeof read);
    QCOMPARE(read, 1234);
    QCOMPARE(layout.elements(cells), 5);
}

void EnvPartsTests::layoutNamesTheColumnsItIsGiven() {
    ObservationLayout layout;
    layout.add(QStringLiteral("board"), ObservationLayout::DType::U8, {2, 2});
    layout.add(QStringLiteral("stats"), ObservationLayout::DType::I32, {2},
               {QStringLiteral("score"), QStringLiteral("lines")});
    const QJsonArray tensors = layout.toJson().value(QStringLiteral("tensors")).toArray();
    QVERIFY(!tensors[0].toObject().contains(QStringLiteral("labels")));
    QCOMPARE(tensors[1].toObject().value(QStringLiteral("labels")).toArray(),
             (QJsonArray{QStringLiteral("score"), QStringLiteral("lines")}));
}

void EnvPartsTests::configFillsInDefaults() {
    QString error;
    const auto resolved = OmaGames::EnvConfig::resolve(schema(), {{QStringLiteral("frame_skip"), 4}}, &error);
    QVERIFY2(resolved, qPrintable(error));
    QCOMPARE(resolved->value(QStringLiteral("mode")).toString(), QStringLiteral("marathon"));
    QCOMPARE(resolved->value(QStringLiteral("frame_skip")).toInt(), 4);
    QCOMPARE(resolved->value(QStringLiteral("ghost")).toBool(), true);
    QCOMPARE(resolved->size(), 3);
}

void EnvPartsTests::configRejectsAnUnknownKey() {
    const QString error = refusal({{QStringLiteral("frameskip"), 4}});
    QVERIFY(error.contains(QStringLiteral("frameskip")));
    QVERIFY(error.contains(QStringLiteral("frame_skip")));  // names the keys it knows
}

void EnvPartsTests::configRejectsABadChoice() {
    QVERIFY(refusal({{QStringLiteral("mode"), QStringLiteral("zen")}}).contains(QStringLiteral("sprint")));
    QVERIFY(!refusal({{QStringLiteral("mode"), 1}}).isEmpty());
}

void EnvPartsTests::configRejectsOutOfRangeAndFractions() {
    QVERIFY(!refusal({{QStringLiteral("frame_skip"), 0}}).isEmpty());
    QVERIFY(!refusal({{QStringLiteral("frame_skip"), 61}}).isEmpty());
    QVERIFY(!refusal({{QStringLiteral("frame_skip"), 1.5}}).isEmpty());
    QVERIFY(refusal({{QStringLiteral("frame_skip"), 60}}).isEmpty());
}

void EnvPartsTests::configRejectsTheWrongType() {
    QVERIFY(!refusal({{QStringLiteral("ghost"), 1}}).isEmpty());
    QVERIFY(!refusal({{QStringLiteral("frame_skip"), true}}).isEmpty());
    QVERIFY(!refusal({{QStringLiteral("frame_skip"), QStringLiteral("4")}}).isEmpty());
}

void EnvPartsTests::replayMergesIdleTicks() {
    Replay replay(QStringLiteral("omatris"), 1, {}, 9);
    replay.advance(60);
    replay.input(QStringLiteral("L"));
    replay.advance(3);
    replay.advance(2);
    replay.advance(0);
    replay.input(QStringLiteral("HD"));
    QCOMPARE(replay.calls(), QStringLiteral("t60 L t5 HD"));
    QCOMPARE(replay.steps().size(), size_t(4));
}

void EnvPartsTests::replayRoundTrips() {
    Replay replay(QStringLiteral("omatris"), 3, {{QStringLiteral("mode"), QStringLiteral("sprint")}}, 4000000000u);
    replay.setAgent(QStringLiteral("cem-gen42"));
    replay.input(QStringLiteral("CW"));
    replay.advance(7);
    replay.input(QStringLiteral("SD+"));

    QString error;
    const auto back = Replay::fromJson(replay.toJson(), &error);
    QVERIFY2(back, qPrintable(error));
    QCOMPARE(back->game(), QStringLiteral("omatris"));
    QCOMPARE(back->rulesVersion(), 3);
    QCOMPARE(back->seed(), 4000000000u);
    QCOMPARE(back->agent(), QStringLiteral("cem-gen42"));
    QCOMPARE(back->config(), replay.config());
    QCOMPARE(back->calls(), QStringLiteral("CW t7 SD+"));
    QCOMPARE(replay.toJson().value(QStringLiteral("format")).toString(), QStringLiteral("replay/v1"));
}

void EnvPartsTests::replayRefusesWhatItCannotPlay() {
    const QJsonObject good = Replay(QStringLiteral("walk"), 1, {}, 5).toJson();
    QString error;
    QVERIFY(Replay::fromJson(good, &error));

    auto with = [&good](const QString &key, const QJsonValue &value) {
        QJsonObject json = good;
        json.insert(key, value);
        return json;
    };
    QVERIFY(!Replay::fromJson(with(QStringLiteral("format"), QStringLiteral("replay/v2")), &error));
    QVERIFY(!Replay::fromJson(with(QStringLiteral("seed"), -1), &error));
    QVERIFY(!Replay::fromJson(with(QStringLiteral("seed"), 5000000000.0), &error));
    QVERIFY(!Replay::fromJson(with(QStringLiteral("game"), QString()), &error));
    QVERIFY(!Replay::fromJson(with(QStringLiteral("calls"), QStringLiteral("L t0")), &error));
    QVERIFY(!Replay::fromJson(with(QStringLiteral("calls"), QStringLiteral("tick")), &error));
    QVERIFY(error.contains(QStringLiteral("tick")));
    QVERIFY(!Replay::validInput(QStringLiteral("two words")));
}

void EnvPartsTests::replayKnowsWhoCanPlayIt() {
    const Replay replay(QStringLiteral("omatris"), 2, {}, 1);
    QString error;
    QVERIFY(replay.playableBy(QStringLiteral("omatris"), 2, &error));
    QVERIFY(!replay.playableBy(QStringLiteral("omasnake"), 2, &error));
    QCOMPARE(error, QStringLiteral("this is a replay of omatris, not omasnake"));
    QVERIFY(!replay.playableBy(QStringLiteral("omatris"), 3, &error));
    QVERIFY(error.contains(QStringLiteral("version 2")));
}

void EnvPartsTests::replayFilesAreRead() {
    QTemporaryDir dir;
    QString error;
    QVERIFY(!Replay::readFile(dir.filePath(QStringLiteral("missing.json")), &error));
    QVERIFY(error.startsWith(QStringLiteral("cannot read")));

    QFile broken(dir.filePath(QStringLiteral("broken.json")));
    QVERIFY(broken.open(QIODevice::WriteOnly));
    broken.write("{\"format\": ");
    broken.close();
    QVERIFY(!Replay::readFile(broken.fileName(), &error));
    QVERIFY(error.contains(QStringLiteral("not a JSON object")));

    QFile good(dir.filePath(QStringLiteral("good.json")));
    QVERIFY(good.open(QIODevice::WriteOnly));
    good.write(QJsonDocument(Replay(QStringLiteral("walk"), 1, {}, 9).toJson()).toJson());
    good.close();
    const auto json = Replay::readFile(good.fileName(), &error);
    QVERIFY(json);
    QCOMPARE(json->value(QStringLiteral("seed")).toInt(), 9);
}

void EnvPartsTests::paceShowsBeatsAtEachSpeed() {
    // Beats shown over eight frames: two, four, eight and sixty-four.
    const int expected[] = {2, 4, 8, 64};
    for (int speed = 1; speed <= OmaGames::ReplayPace::kSpeeds; ++speed) {
        OmaGames::ReplayPace pace(speed);
        int beats = 0;
        for (int frame = 0; frame < 8; ++frame)
            beats += pace.nextFrame();
        QCOMPARE(beats, expected[speed - 1]);
    }
    OmaGames::ReplayPace quarter(1);
    QCOMPARE(quarter.nextFrame(), 0);  // a quarter beat owed, nothing shown yet
    QCOMPARE(quarter.nextFrame(), 0);
    QCOMPARE(quarter.nextFrame(), 0);
    QCOMPARE(quarter.nextFrame(), 1);
}

void EnvPartsTests::paceRefusesWhatIsNotASpeed() {
    OmaGames::ReplayPace pace;
    QCOMPARE(pace.speed(), OmaGames::ReplayPace::kRealSpeed);
    QCOMPARE(pace.label(), QStringLiteral("1×"));
    QVERIFY(!pace.fasterThanReal());
    QVERIFY(!pace.setSpeed(3));
    QVERIFY(!pace.setSpeed(0));
    QVERIFY(!pace.setSpeed(5));
    QVERIFY(pace.setSpeed(4));
    QCOMPARE(pace.label(), QStringLiteral("8×"));
    QVERIFY(pace.fasterThanReal());
    QVERIFY(pace.setSpeed(1));
    pace.nextFrame();
    pace.restart();
    QCOMPARE(pace.nextFrame() + pace.nextFrame() + pace.nextFrame(), 0);  // the owed quarter is gone
}

void EnvPartsTests::playlistTakesEveryReplayArgumentInOrder() {
    using OmaGames::ReplayPlaylist;
    QString error;
    const QString app = QStringLiteral("omatris");
    QCOMPARE(*ReplayPlaylist::fromArguments({app}, &error), QStringList());
    const QStringList args = {app, QStringLiteral("--replay"), QStringLiteral("a.json"), QStringLiteral("-x"),
                              QStringLiteral("--replay"), QStringLiteral("b.json")};
    QCOMPARE(*ReplayPlaylist::fromArguments(args, &error), (QStringList{QStringLiteral("a.json"), QStringLiteral("b.json")}));

    QVERIFY(!ReplayPlaylist::fromArguments({app, QStringLiteral("--replay")}, &error));
    QCOMPARE(error, QStringLiteral("--replay needs a file"));
    QVERIFY(!ReplayPlaylist::fromArguments({app, QStringLiteral("--replay"), QStringLiteral("--replay"),
                                            QStringLiteral("a.json")}, &error));
}

void EnvPartsTests::playlistStepsBetweenItsEnds() {
    OmaGames::ReplayPlaylist one({QStringLiteral("a.json")});
    QCOMPARE(one.current(), QStringLiteral("a.json"));
    QCOMPARE(one.position(), QString());  // one replay is not a list to step through
    QVERIFY(!one.next() && !one.previous());

    OmaGames::ReplayPlaylist three({QStringLiteral("a"), QStringLiteral("b"), QStringLiteral("c")});
    QCOMPARE(three.position(), QStringLiteral("1 of 3"));
    QVERIFY(!three.hasPrevious() && !three.previous());
    QVERIFY(three.next() && three.next());
    QCOMPARE(three.current(), QStringLiteral("c"));
    QCOMPARE(three.position(), QStringLiteral("3 of 3"));
    QVERIFY(!three.hasNext() && !three.next());
    QCOMPARE(three.index(), 2);
    QVERIFY(three.previous());
    QCOMPARE(three.current(), QStringLiteral("b"));

    QVERIFY(OmaGames::ReplayPlaylist().isEmpty());
    QCOMPARE(OmaGames::ReplayPlaylist().current(), QString());
}
