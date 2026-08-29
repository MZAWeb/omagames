#include "persistencetests.h"

#include <QSettings>
#include <QTemporaryDir>
#include <QtTest>

#include "cellmodel.h"
#include "savedgame.h"
#include "sudoku.h"
#include "sudokugame.h"

namespace {

constexpr quint32 kSeed = 20260829u;

int cellInt(QAbstractListModel *model, int cell, CellModel::Role role) {
    return model->data(model->index(cell, 0), role).toInt();
}

bool cellBool(QAbstractListModel *model, int cell, CellModel::Role role) {
    return model->data(model->index(cell, 0), role).toBool();
}

}  // namespace

void PersistenceTests::initTestCase() {
    static QTemporaryDir dir;
    QVERIFY(dir.isValid());
    m_settingsDir = dir.path();
    TestSupport::redirectSettings(m_settingsDir);
}

void PersistenceTests::init() {
    QSettings settings;
    settings.clear();
    settings.sync();
}

void PersistenceTests::winningSwitchesStateAndClearsTheSave() {
    const SudokuBoard expected = TestSupport::installSavedGame(1, kSeed);
    const int last = TestSupport::firstEmptyCell(expected);

    SudokuGame game;
    QVERIFY(game.hasSavedGame());
    game.resumeSavedGame();
    QCOMPARE(game.state(), QStringLiteral("playing"));
    QCOMPARE(game.elapsedSeconds(), 42);
    QCOMPARE(game.selectedIndex(), last);

    // A wrong entry is flagged immediately and does not win.
    game.enterValue(expected.puzzle().solution[size_t(last)] % 9 + 1);
    QVERIFY(cellBool(game.cells(), last, CellModel::WrongRole));
    QCOMPARE(game.state(), QStringLiteral("playing"));

    game.enterValue(expected.puzzle().solution[size_t(last)]);
    QVERIFY(!cellBool(game.cells(), last, CellModel::WrongRole));
    QCOMPARE(game.state(), QStringLiteral("won"));
    QVERIFY(!game.hasSavedGame());
    QVERIFY(QSettings().value(QStringLiteral("state/v1")).toString().isEmpty());
}

void PersistenceTests::validateAsYouGoIsRemembered() {
    {
        SudokuGame game;
        game.newGame(QStringLiteral("easy"));
        game.setValidateAsYouGo(false);
        QVERIFY(!game.validateAsYouGo());
        game.backToStart();
    }
    SudokuGame game;
    QVERIFY(!game.validateAsYouGo());
}

void PersistenceTests::savedGameSurvivesRestart() {
    int cell = -1;
    QString technique;
    {
        SudokuGame game;
        game.newGame(QStringLiteral("medium"));
        technique = game.techniqueLabel();
        QVERIFY(!technique.isEmpty());
        cell = game.selectedIndex();
        game.enterValue(7);
        game.backToStart();  // saves on the way out
        QVERIFY(game.hasSavedGame());
    }

    SudokuGame resumed;
    QVERIFY(resumed.hasSavedGame());
    QCOMPARE(resumed.state(), QStringLiteral("start"));
    resumed.resumeSavedGame();
    QCOMPARE(resumed.state(), QStringLiteral("playing"));
    QCOMPARE(resumed.difficulty(), QStringLiteral("medium"));
    QCOMPARE(resumed.techniqueLabel(), technique);
    QCOMPARE(cellInt(resumed.cells(), cell, CellModel::ValueRole), 7);

    resumed.newGame(QStringLiteral("easy"));  // a new game drops the old save
    QVERIFY(!resumed.hasSavedGame());
}

void PersistenceTests::solvedSaveIsNotOffered() {
    TestSupport::installSavedGame(0, kSeed);
    SudokuGame game;
    QVERIFY(!game.hasSavedGame());
    QCOMPARE(game.state(), QStringLiteral("start"));
}
