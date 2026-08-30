#include "generatortests.h"

#include <QElapsedTimer>
#include <QtTest>

#include "generator.h"
#include "solver.h"

namespace {

constexpr int kSeeds = 50;

QPoint centre(const PresetSpec &preset) { return {preset.width / 2, preset.height / 2}; }

}  // namespace

void GeneratorTests::everyPresetProducesNoGuessBoards() {
    for (const PresetSpec &preset : Presets::kAll) {
        for (quint32 seed = 1; seed <= 20; ++seed) {
            const GenerationResult result = Generator::generate(preset, centre(preset), seed);
            const QByteArray where = QStringLiteral("%1 seed %2").arg(preset.label).arg(seed).toUtf8();
            QVERIFY2(result.guaranteed, where.constData());
            QVERIFY(result.attempts >= 1);
            Board board = result.board;
            QVERIFY(board.minesPlaced());
            QCOMPARE(board.status(), Status::Ready);
            QCOMPARE(board.mineCount(), preset.mines);
            QCOMPARE(board.width(), preset.width);
            QCOMPARE(board.height(), preset.height);
            // The opening is a zero: the click and its ring hold no mine.
            QVERIFY(!board.cell(centre(preset)).mine);
            for (int n : board.neighbours(board.index(centre(preset))))
                QVERIFY(!board.cell(n).mine);
            QVERIFY(!board.reveal(centre(preset)).lost());
            QCOMPARE(int(board.cell(centre(preset)).adjacent), 0);
            // The oracle: the solver finishes it.
            QCOMPARE(Solver::solve(board).status, Status::Won);
        }
    }
}

void GeneratorTests::sameSeedSameBoard() {
    const PresetSpec &preset = Presets::spec(Preset::Intermediate);
    const GenerationResult a = Generator::generate(preset, centre(preset), 2026u);
    const GenerationResult b = Generator::generate(preset, centre(preset), 2026u);
    const GenerationResult c = Generator::generate(preset, centre(preset), 2027u);
    QCOMPARE(a.board.mines(), b.board.mines());
    QCOMPARE(a.attempts, b.attempts);
    QVERIFY(a.board.mines() != c.board.mines());
    // Custom dimensions go through the same path.
    const GenerationResult custom = Generator::generate(12, 8, 15, QPoint(3, 3), 5u);
    QVERIFY(custom.guaranteed);
    QCOMPARE(custom.board.width(), 12);
    QCOMPARE(custom.board.mineCount(), 15);
}

void GeneratorTests::attemptSeedsStrideApart() {
    QCOMPARE(Generator::attemptSeed(10u, 0), 10u);
    QVERIFY(Generator::attemptSeed(10u, 1) != 10u);
    QVERIFY(Generator::attemptSeed(10u, 1) != Generator::attemptSeed(10u, 2));
}

void GeneratorTests::exhaustedAttemptsReturnAnUnguaranteedBoard() {
    const PresetSpec &preset = Presets::spec(Preset::Expert);
    // Find a seed whose first board needs a guess, then cap it to one attempt.
    quint32 seed = 1;
    while (Generator::generate(preset, centre(preset), seed, 1).guaranteed)
        ++seed;
    const GenerationResult capped = Generator::generate(preset, centre(preset), seed, 1);
    QVERIFY(!capped.guaranteed);
    QCOMPARE(capped.attempts, 1);
    QVERIFY(capped.board.minesPlaced());
    QCOMPARE(capped.board.mineCount(), preset.mines);
    QVERIFY(!capped.board.cell(centre(preset)).mine);
    // Given room, the same seed keeps trying and finds one.
    const GenerationResult full = Generator::generate(preset, centre(preset), seed);
    QVERIFY(full.guaranteed);
    QVERIFY(full.attempts > 1);
    QVERIFY(full.board.mines() != capped.board.mines());
}

void GeneratorTests::generationIsFast() {
    for (const PresetSpec &preset : Presets::kAll) {
        QElapsedTimer timer;
        qint64 slowest = 0;
        int attempts = 0;
        int maxAttempts = 0;
        timer.start();
        for (quint32 seed = 1; seed <= kSeeds; ++seed) {
            const qint64 before = timer.elapsed();
            const GenerationResult result = Generator::generate(preset, centre(preset), seed * 7919u);
            QVERIFY(result.guaranteed);
            slowest = std::max(slowest, timer.elapsed() - before);
            attempts += result.attempts;
            maxAttempts = std::max(maxAttempts, result.attempts);
        }
        const double average = double(timer.elapsed()) / kSeeds;
        qInfo("%s: %d boards, avg %.1f ms, slowest %lld ms, avg %.1f attempts, max %d", preset.label, kSeeds, average,
              slowest, double(attempts) / kSeeds, maxAttempts);
        // Generous so CI stays green; Expert averages far less on a desktop.
        QVERIFY2(average < 2000.0, preset.label);
    }
}
