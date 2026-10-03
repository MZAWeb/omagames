#include "envabitests.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtTest>

#include <memory>
#include <vector>

#include "omagames_env.h"

namespace {

struct Destroy {
    void operator()(OgEnv *env) const { og_destroy(env); }
};
using EnvPtr = std::unique_ptr<OgEnv, Destroy>;

EnvPtr create(const QByteArray &config = {}) {
    return EnvPtr(og_create(config.isEmpty() ? nullptr : config.constData()));
}

QJsonObject parse(const char *json) {
    return QJsonDocument::fromJson(QByteArray(json)).object();
}

// A tensor read the way a trainer reads it: by the offset the spec gives.
struct Observation {
    std::vector<std::byte> bytes;
    QJsonObject spec;

    qint32 int32(const QString &name) const {
        qint32 value = 0;
        std::memcpy(&value, bytes.data() + offset(name), sizeof value);
        return value;
    }
    quint8 uint8(const QString &name, int index) const { return quint8(bytes[size_t(offset(name) + index)]); }

    int offset(const QString &name) const {
        for (const QJsonValue &tensor : spec.value(QStringLiteral("tensors")).toArray()) {
            if (tensor.toObject().value(QStringLiteral("name")).toString() == name)
                return tensor.toObject().value(QStringLiteral("offset")).toInt();
        }
        return -1;
    }
};

QJsonObject observationSpec(OgEnv *env) {
    return parse(og_env_spec(env)).value(QStringLiteral("observation")).toObject();
}

Observation observe(OgEnv *env) {
    Observation obs {{}, observationSpec(env)};
    obs.bytes.resize(size_t(obs.spec.value(QStringLiteral("size")).toInt()));
    og_observe(env, obs.bytes.data());
    return obs;
}

std::vector<quint8> mask(OgEnv *env) {
    std::vector<quint8> bytes(3);
    og_action_mask(env, bytes.data());
    return bytes;
}

// Walks right from the left wall until the episode ends; how far it got
// gives away the hidden cell without asking the env for it.
int stepsToTarget(OgEnv *env) {
    OgStepResult result {};
    int steps = 0;
    do {
        if (og_step(env, 2, &result) != 0)
            return -1;
        ++steps;
    } while (!result.terminated);
    return steps;
}

constexpr int kLeft = 0;
constexpr int kStay = 1;
constexpr int kRight = 2;

}  // namespace

void EnvAbiTests::gameSpecAddsTheAbiAndMaxSteps() {
    QCOMPARE(og_abi_version(), OG_ABI_VERSION);
    const QJsonObject spec = parse(og_game_spec());
    QCOMPARE(spec.value(QStringLiteral("abi")).toInt(), OG_ABI_VERSION);
    QCOMPARE(spec.value(QStringLiteral("game")).toString(), QStringLiteral("walk"));
    QCOMPARE(spec.value(QStringLiteral("rules_version")).toInt(), 1);
    const QJsonObject config = spec.value(QStringLiteral("config")).toObject();
    QVERIFY(config.contains(QStringLiteral("length")));
    QCOMPARE(config.value(QStringLiteral("max_steps")).toObject().value(QStringLiteral("default")).toInt(), 0);
}

void EnvAbiTests::createRefusesABadConfig() {
    QVERIFY(!create("{not json"));
    QVERIFY(QByteArray(og_last_error()).size() > 0);
    QVERIFY(!create("[1, 2]"));
    QCOMPARE(QByteArray(og_last_error()), QByteArray("the config must be a JSON object"));
    QVERIFY(!create(R"({"lenght": 5})"));
    QVERIFY(QByteArray(og_last_error()).contains("lenght"));
    QVERIFY(!create(R"({"max_steps": -1})"));
    QVERIFY(create());
}

void EnvAbiTests::envSpecFollowsTheConfig() {
    EnvPtr env = create(R"({"length": 20})");
    const QJsonObject spec = parse(og_env_spec(env.get()));
    QCOMPARE(spec.value(QStringLiteral("config")).toObject().value(QStringLiteral("length")).toInt(), 20);
    QCOMPARE(spec.value(QStringLiteral("config")).toObject().value(QStringLiteral("start")).toString(),
             QStringLiteral("middle"));
    const QJsonArray tensors = spec.value(QStringLiteral("observation")).toObject().value(QStringLiteral("tensors")).toArray();
    QCOMPARE(tensors[1].toObject().value(QStringLiteral("shape")).toArray(), QJsonArray{20});
    const QJsonObject actions = spec.value(QStringLiteral("actions")).toObject();
    QCOMPARE(actions.value(QStringLiteral("count")).toInt(), 3);
    QCOMPARE(actions.value(QStringLiteral("labels")).toArray().at(2).toString(), QStringLiteral("right"));
    QCOMPARE(spec.value(QStringLiteral("signals")).toArray(), QJsonArray{QStringLiteral("moved")});
}

void EnvAbiTests::nothingIsSteppedBeforeAReset() {
    EnvPtr env = create();
    OgStepResult result {};
    QCOMPARE(og_step(env.get(), kStay, &result), -1);
    QVERIFY(QByteArray(og_last_error()).contains("reset"));
    QCOMPARE(mask(env.get()), (std::vector<quint8>{0, 0, 0}));
    og_reset(env.get(), 1);
    QCOMPARE(og_step(env.get(), kStay, &result), 0);
}

void EnvAbiTests::theObservationIsWhereTheSpecSays() {
    EnvPtr env = create(R"({"length": 9})");
    og_reset(env.get(), 1);
    OgStepResult result {};
    QCOMPARE(og_step(env.get(), kStay, &result), 0);
    QCOMPARE(og_step(env.get(), kStay, &result), 0);
    const Observation obs = observe(env.get());
    QCOMPARE(obs.int32(QStringLiteral("position")), 4);
    QCOMPARE(obs.int32(QStringLiteral("steps")), 2);
    QCOMPARE(obs.uint8(QStringLiteral("visited"), 4), quint8(1));
    QCOMPARE(obs.uint8(QStringLiteral("visited"), 3), quint8(0));
}

void EnvAbiTests::maskedAndOutOfRangeActionsAreRefused() {
    EnvPtr env = create(R"({"start": "left"})");
    og_reset(env.get(), 1);
    QCOMPARE(mask(env.get()), (std::vector<quint8>{0, 1, 1}));
    OgStepResult result {};
    QCOMPARE(og_step(env.get(), kLeft, &result), -1);
    QVERIFY(QByteArray(og_last_error()).contains("masked"));
    QCOMPARE(og_step(env.get(), 3, &result), -1);
    QVERIFY(QByteArray(og_last_error()).contains("out of range"));
    QCOMPARE(og_step(env.get(), -1, &result), -1);
    QCOMPARE(observe(env.get()).int32(QStringLiteral("steps")), 0);  // refused means untouched
}

void EnvAbiTests::anEpisodeEndsWhenTheRulesSaySo() {
    EnvPtr env = create(R"({"start": "left"})");
    og_reset(env.get(), 3);
    OgStepResult result {};
    do {
        QCOMPARE(og_step(env.get(), kRight, &result), 0);
        QCOMPARE(result.signal_values[0], 1.0);
        QCOMPARE(result.ticks, qint64(1));
    } while (!result.terminated);
    QCOMPARE(result.reward, 1.0);
    QCOMPARE(result.truncated, 0);
    QCOMPARE(mask(env.get()), (std::vector<quint8>{0, 0, 0}));
    QCOMPARE(og_step(env.get(), kStay, &result), -1);
    QVERIFY(QByteArray(og_last_error()).contains("over"));
    og_reset(env.get(), 3);
    QCOMPARE(og_step(env.get(), kStay, &result), 0);
}

void EnvAbiTests::maxStepsTruncates() {
    EnvPtr env = create(R"({"max_steps": 2})");
    og_reset(env.get(), 1);
    OgStepResult result {};
    QCOMPARE(og_step(env.get(), kStay, &result), 0);
    QCOMPARE(result.truncated, 0);
    QCOMPARE(og_step(env.get(), kStay, &result), 0);
    QCOMPARE(result.truncated, 1);
    QCOMPARE(result.terminated, 0);
    QCOMPARE(og_step(env.get(), kStay, &result), -1);
}

void EnvAbiTests::aCloneContinuesIdentically() {
    EnvPtr env = create(R"({"start": "left", "max_steps": 50})");
    og_reset(env.get(), 11);
    OgStepResult result {};
    QCOMPARE(og_step(env.get(), kStay, &result), 0);
    EnvPtr copy(og_clone(env.get(), 0, 0));
    QCOMPARE(observe(copy.get()).bytes, observe(env.get()).bytes);
    QCOMPARE(stepsToTarget(copy.get()), stepsToTarget(env.get()));
    QCOMPARE(observe(copy.get()).bytes, observe(env.get()).bytes);
}

void EnvAbiTests::aReseededCloneRedrawsWhatIsHidden() {
    EnvPtr env = create(R"({"start": "left", "length": 64})");
    og_reset(env.get(), 11);
    // What the player sees is copied as it stands; only the hidden cell moves.
    bool moved = false;
    for (quint32 seed = 0; seed < 8; ++seed) {
        EnvPtr copy(og_clone(env.get(), 1, seed));
        QCOMPARE(observe(copy.get()).bytes, observe(env.get()).bytes);
        EnvPtr original(og_clone(env.get(), 0, 0));
        moved = moved || stepsToTarget(copy.get()) != stepsToTarget(original.get());
    }
    QVERIFY(moved);
}

void EnvAbiTests::theReplayRecordsTheEpisode() {
    EnvPtr env = create(R"({"start": "left"})");
    og_reset(env.get(), 21);
    OgStepResult result {};
    og_step(env.get(), kStay, &result);
    og_step(env.get(), kRight, &result);
    const QJsonObject replay = parse(og_replay_json(env.get()));
    QCOMPARE(replay.value(QStringLiteral("format")).toString(), QStringLiteral("replay/v1"));
    QCOMPARE(replay.value(QStringLiteral("game")).toString(), QStringLiteral("walk"));
    QCOMPARE(replay.value(QStringLiteral("seed")).toInt(), 21);
    QCOMPARE(replay.value(QStringLiteral("calls")).toString(), QStringLiteral("S t1 R t1"));
    QCOMPARE(replay.value(QStringLiteral("config")).toObject().value(QStringLiteral("start")).toString(),
             QStringLiteral("left"));
    QCOMPARE(parse(og_info_json(env.get())).value(QStringLiteral("position")).toInt(), 1);
}

void EnvAbiTests::aBatchStepsAndResetsInPlace() {
    // One step is the whole episode, so every env in the batch ends at once.
    EnvPtr a = create(R"({"max_steps": 1})");
    EnvPtr b = create(R"({"max_steps": 1})");
    og_reset(a.get(), 1);
    og_reset(b.get(), 2);
    OgEnv *envs[] = {a.get(), b.get()};
    const int32_t actions[] = {kLeft, kRight};
    const uint32_t seeds[] = {7, 8};
    OgStepResult results[2] {};
    const int size = observationSpec(a.get()).value(QStringLiteral("size")).toInt();
    std::vector<std::byte> obs(size_t(2 * size)), finalObs(size_t(2 * size));
    std::vector<quint8> masks(6);
    QCOMPARE(og_step_batch(envs, 2, actions, seeds, results, obs.data(), finalObs.data(), masks.data()), 0);

    // Truncated, or the one step happened to find the hidden cell: ended either way.
    QVERIFY(results[0].terminated || results[0].truncated);
    QVERIFY(results[1].terminated || results[1].truncated);
    Observation last {{}, observationSpec(a.get())};
    last.bytes.assign(finalObs.begin(), finalObs.begin() + size);
    QCOMPARE(last.int32(QStringLiteral("position")), 3);
    last.bytes.assign(finalObs.begin() + size, finalObs.end());
    QCOMPARE(last.int32(QStringLiteral("position")), 5);
    // The rows the caller acts on next are already the new episodes.
    Observation fresh {{}, observationSpec(a.get())};
    fresh.bytes.assign(obs.begin(), obs.begin() + size);
    QCOMPARE(fresh.int32(QStringLiteral("position")), 4);
    QCOMPARE(fresh.int32(QStringLiteral("steps")), 0);
    QCOMPARE(masks, (std::vector<quint8>{1, 1, 1, 1, 1, 1}));
    QCOMPARE(parse(og_replay_json(b.get())).value(QStringLiteral("seed")).toInt(), 8);
}

void EnvAbiTests::aBatchChecksEveryActionFirst() {
    EnvPtr a = create(R"({"start": "left"})");
    EnvPtr b = create(R"({"start": "left"})");
    og_reset(a.get(), 1);
    og_reset(b.get(), 1);
    OgEnv *envs[] = {a.get(), b.get()};
    const int32_t actions[] = {kRight, kLeft};
    OgStepResult results[2] {};
    QCOMPARE(og_step_batch(envs, 2, actions, nullptr, results, nullptr, nullptr, nullptr), -1);
    QVERIFY(QByteArray(og_last_error()).startsWith("env 1"));
    QCOMPARE(observe(a.get()).int32(QStringLiteral("steps")), 0);

    EnvPtr longer = create(R"({"length": 30})");
    og_reset(longer.get(), 1);
    OgEnv *mixed[] = {a.get(), longer.get()};
    const int32_t stay[] = {kStay, kStay};
    QCOMPARE(og_step_batch(mixed, 2, stay, nullptr, results, nullptr, nullptr, nullptr), -1);
    QVERIFY(QByteArray(og_last_error()).contains("different config"));
}
