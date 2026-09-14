#include <QCoreApplication>
#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>
#include <algorithm>
#include <cmath>

#include "dropengine.h"
#include "omadropgame.h"

class OmadropTests : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void startsWithAReadyBallAndThreePegs();
    void aimIsPlayerControlledAndClamped();
    void launchFollowsTheGuide();
    void ballReboundsFromSideWall();
    void hitCountsDownAndScores();
    void zeroPegIsRemoved();
    void pegsStayStillUntilTheShotEnds();
    void finishedShotAddsOneToThreePegs();
    void shotDoesNotBounceForever();
    void pegCrossingTopEndsRun();
    void pauseFreezesSimulation();
    void bridgeEmitsScoreAndRecordsFinishedRun();

private:
    QTemporaryDir m_settings;
};

void OmadropTests::initTestCase() {
    QVERIFY(m_settings.isValid());
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_settings.path());
}

void OmadropTests::startsWithAReadyBallAndThreePegs() {
    DropEngine game(1);
    QCOMPARE(game.phase(), DropPhase::Playing);
    QVERIFY(game.ready());
    QCOMPARE(game.score(), 0);
    QCOMPARE(game.pegs().size(), 3);
}

void OmadropTests::aimIsPlayerControlledAndClamped() {
    DropEngine game(1);
    game.setAimAngle(0.5);
    game.tick(0.05);
    QCOMPARE(game.aimAngle(), 0.5);
    game.setAimAngle(2.0);
    QCOMPARE(game.aimAngle(), 1.08);
    game.nudgeAim(-1);
    QVERIFY(game.aimAngle() < 1.08);
}

void OmadropTests::launchFollowsTheGuide() {
    DropEngine game(1);
    game.setAimAngleForTests(0.5);
    QVERIFY(game.launch());
    QVERIFY(game.ball().active);
    QVERIFY(game.ball().velocity.x() > 0.0);
    QVERIFY(game.ball().velocity.y() > 0.0);
    QVERIFY(!game.launch());
    QCOMPARE(game.shots(), 1);
}

void OmadropTests::ballReboundsFromSideWall() {
    DropEngine game(1);
    game.clearPegsForTests();
    game.setBallForTests({DropEngine::kBallRadius / 2.0, 0.4}, {-0.5, 0.0});
    game.tick();
    QVERIFY(game.ball().velocity.x() > 0.0);
}

void OmadropTests::hitCountsDownAndScores() {
    DropEngine game(1);
    game.clearPegsForTests();
    game.addPegForTests({0.5, 0.5}, 2);
    game.setBallForTests({0.5, 0.44}, {0.0, 0.5});
    const QVector<DropEvent> events = game.tick();
    QCOMPARE(game.score(), 1);
    QCOMPARE(game.pegs().first().hits, 1);
    QVERIFY(std::any_of(events.begin(), events.end(), [](const DropEvent &event) {
        return event.type == DropEvent::PegHit;
    }));
}

void OmadropTests::zeroPegIsRemoved() {
    DropEngine game(1);
    game.clearPegsForTests();
    game.addPegForTests({0.5, 0.5}, 1);
    game.setBallForTests({0.5, 0.44}, {0.0, 0.5});
    game.tick();
    QCOMPARE(game.score(), 1);
    QVERIFY(game.pegs().isEmpty());
}

void OmadropTests::pegsStayStillUntilTheShotEnds() {
    DropEngine game(1);
    game.clearPegsForTests();
    game.addPegForTests({0.2, 0.7}, 2);
    game.setBallForTests({0.5, 0.3}, {0.0, 0.1});
    const QPointF before = game.pegs().first().position;
    game.tick();
    QCOMPARE(game.pegs().first().position, before);

    game.setBallForTests({0.5, 1.05}, {0.0, 0.5});
    game.tick();
    game.tick(0.05);
    QVERIFY(game.pegs().first().position.y() < before.y());
}

void OmadropTests::finishedShotAddsOneToThreePegs() {
    DropEngine game(2);
    game.clearPegsForTests();
    game.setBallForTests({0.5, 1.05}, {0.0, 0.5});
    const QVector<DropEvent> events = game.tick();
    QVERIFY(game.ready());
    QVERIFY(game.pegs().size() >= 1);
    QVERIFY(game.pegs().size() <= 3);
    QVERIFY(std::any_of(events.begin(), events.end(), [](const DropEvent &event) {
        return event.type == DropEvent::ShotFinished;
    }));
}

void OmadropTests::shotDoesNotBounceForever() {
    for (double angle : {-0.9, -0.45, 0.0, 0.45, 0.9}) {
        DropEngine game(4);
        game.setAimAngle(angle);
        QVERIFY(game.launch());
        for (int tick = 0; tick < 600 && !game.ready(); ++tick)
            game.tick();
        QVERIFY2(game.ready(), "a shot was still active after ten seconds");
    }
}

void OmadropTests::pegCrossingTopEndsRun() {
    DropEngine game(1);
    game.clearPegsForTests();
    game.addPegForTests({0.5, DropEngine::kDangerY + DropEngine::kPegRadius - 0.001}, 3);
    const QVector<DropEvent> events = game.tick();
    QCOMPARE(game.phase(), DropPhase::GameOver);
    QVERIFY(std::any_of(events.begin(), events.end(), [](const DropEvent &event) {
        return event.type == DropEvent::GameOver;
    }));
}

void OmadropTests::pauseFreezesSimulation() {
    DropEngine game(1);
    game.launch();
    const QPointF before = game.ball().position;
    game.setPaused(true);
    QVERIFY(game.tick().isEmpty());
    QCOMPARE(game.ball().position, before);
}

void OmadropTests::bridgeEmitsScoreAndRecordsFinishedRun() {
    OmadropGame game;
    game.setStepInterval(0);
    game.startGame(1);
    game.engineForTests()->clearPegsForTests();
    game.engineForTests()->addPegForTests({0.5, 0.5}, 1);
    game.engineForTests()->setBallForTests({0.5, 0.44}, {0.0, 0.5});
    QSignalSpy scoreSpy(&game, &OmadropGame::scoreChanged);
    game.step();
    QCOMPARE(game.score(), 1);
    QCOMPARE(scoreSpy.count(), 1);

    game.engineForTests()->addPegForTests(
        {0.5, DropEngine::kDangerY + DropEngine::kPegRadius - 0.001}, 2);
    game.step();
    QCOMPARE(game.phase(), QStringLiteral("gameover"));
    QCOMPARE(game.best(), 1);
    QCOMPARE(game.highScores().size(), 1);
}

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("Omacom"));
    QCoreApplication::setApplicationName(QStringLiteral("omadrop-tests"));
    OmadropTests tests;
    return QTest::qExec(&tests, argc, argv);
}

#include "tst_omadrop.moc"
