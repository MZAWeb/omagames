#include "bridgetests.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

#include "oma2048game.h"

namespace {

constexpr quint32 kSeed = 20260831u;

struct Cell {
    int row;
    int col;
    int value;
};

// The run a bridge under test wakes up to: written where the previous launch
// would have left it, under state/v1.
void seedState(const QVector<Cell> &cells, int score = 0, bool won = false) {
    QJsonArray tiles;
    for (const Cell &cell : cells)
        tiles.append(QJsonObject{{QStringLiteral("row"), cell.row},
                                 {QStringLiteral("col"), cell.col},
                                 {QStringLiteral("value"), cell.value}});
    const QJsonObject state{{QStringLiteral("tiles"), tiles},
                            {QStringLiteral("score"), score},
                            {QStringLiteral("won"), won}};
    QSettings settings;
    settings.setValue(QStringLiteral("state/v1"),
                      QString::fromUtf8(QJsonDocument(state).toJson(QJsonDocument::Compact)));
    settings.sync();
}

void seedFullScoreTable() {
    QJsonArray entries;
    for (int i = 10; i > 0; --i)
        entries.append(QJsonObject{{QStringLiteral("date"), QStringLiteral("2026-08-30")},
                                   {QStringLiteral("score"), 1000 + i},
                                   {QStringLiteral("tile"), 128}});
    const QJsonObject table{{QStringLiteral("classic"), entries}};
    QSettings settings;
    settings.setValue(QStringLiteral("scores/v1"),
                      QString::fromUtf8(QJsonDocument(table).toJson(QJsonDocument::Compact)));
    settings.sync();
}

// One empty cell at (3,3); sliding row 3 right fills the board and leaves no
// adjacent pair whether the spawn at (3,0) is a 2 or a 4.
QVector<Cell> oneMoveFromOver() {
    return {{0, 0, 2},   {0, 1, 4},    {0, 2, 8},  {0, 3, 16},
            {1, 0, 32},  {1, 1, 64},   {1, 2, 128}, {1, 3, 256},
            {2, 0, 512}, {2, 1, 1024}, {2, 2, 2},  {2, 3, 4},
            {3, 0, 8},   {3, 1, 16},   {3, 2, 32}};
}

int cellValue(QAbstractListModel *tiles, int row, int col) {
    for (int i = 0; i < tiles->rowCount(); ++i) {
        const QModelIndex index = tiles->index(i, 0);
        if (tiles->data(index, Oma2048TileModel::RowRole).toInt() == row
            && tiles->data(index, Oma2048TileModel::ColRole).toInt() == col)
            return tiles->data(index, Oma2048TileModel::ValueRole).toInt();
    }
    return 0;
}

int cellId(QAbstractListModel *tiles, int row, int col) {
    for (int i = 0; i < tiles->rowCount(); ++i) {
        const QModelIndex index = tiles->index(i, 0);
        if (tiles->data(index, Oma2048TileModel::RowRole).toInt() == row
            && tiles->data(index, Oma2048TileModel::ColRole).toInt() == col)
            return tiles->data(index, Oma2048TileModel::TileIdRole).toInt();
    }
    return -1;
}

// The board as a comparable set: ids are per-bridge, positions and values are
// the run.
QSet<QString> cellSet(QAbstractListModel *tiles) {
    QSet<QString> cells;
    for (int i = 0; i < tiles->rowCount(); ++i) {
        const QModelIndex index = tiles->index(i, 0);
        cells.insert(QStringLiteral("%1,%2:%3")
                         .arg(tiles->data(index, Oma2048TileModel::RowRole).toInt())
                         .arg(tiles->data(index, Oma2048TileModel::ColRole).toInt())
                         .arg(tiles->data(index, Oma2048TileModel::ValueRole).toInt()));
    }
    return cells;
}

}  // namespace

void BridgeTests::initTestCase() {
    static QTemporaryDir dir;
    QVERIFY(dir.isValid());
    m_settingsDir = dir.path();
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_settingsDir);
}

void BridgeTests::init() {
    QSettings settings;
    settings.clear();
    settings.sync();
}

void BridgeTests::startsANewGameWhenNothingIsSaved() {
    Oma2048Game game(kSeed);
    QCOMPARE(game.boardSize(), 4);
    QCOMPARE(game.score(), 0);
    QCOMPARE(game.bestScore(), 0);
    QVERIFY(!game.over());
    QVERIFY(!game.won());
    QVERIFY(!game.keepPlaying());
    QVERIFY(!game.canUndo());
    QCOMPARE(game.lastRank(), -1);
    QVERIFY(game.scores().isEmpty());

    QAbstractListModel *tiles = game.tiles();
    QCOMPARE(tiles->rowCount(), 2);
    const QHash<int, QByteArray> roles = tiles->roleNames();
    QCOMPARE(roles.value(Oma2048TileModel::TileIdRole), QByteArrayLiteral("tileId"));
    QCOMPARE(roles.value(Oma2048TileModel::ValueRole), QByteArrayLiteral("value"));
    QCOMPARE(roles.value(Oma2048TileModel::RowRole), QByteArrayLiteral("row"));
    QCOMPARE(roles.value(Oma2048TileModel::ColRole), QByteArrayLiteral("col"));
    for (int i = 0; i < tiles->rowCount(); ++i) {
        const QModelIndex index = tiles->index(i, 0);
        const int value = tiles->data(index, Oma2048TileModel::ValueRole).toInt();
        QVERIFY(value == 2 || value == 4);
        QVERIFY(tiles->data(index, Oma2048TileModel::RowRole).toInt() >= 0);
        QVERIFY(tiles->data(index, Oma2048TileModel::ColRole).toInt() < 4);
    }
}

void BridgeTests::plainMoveUpdatesRowsWithoutAReset() {
    seedState({{0, 0, 2}, {1, 1, 4}});
    Oma2048Game game(kSeed);
    QAbstractListModel *tiles = game.tiles();

    QSignalSpy resets(tiles, &QAbstractItemModel::modelReset);
    QSignalSpy removals(tiles, &QAbstractItemModel::rowsRemoved);
    QSignalSpy insertions(tiles, &QAbstractItemModel::rowsInserted);
    QSignalSpy changes(tiles, &QAbstractItemModel::dataChanged);
    QSignalSpy undoable(&game, &Oma2048Game::canUndoChanged);

    game.moveRight();
    QCOMPARE(resets.count(), 0);
    QCOMPARE(removals.count(), 0);
    QCOMPARE(insertions.count(), 1);
    QCOMPARE(changes.count(), 2);
    // Nothing merged, so the slide is a column change and nothing else.
    const QList<int> roles = changes.at(0).at(2).value<QList<int>>();
    QCOMPARE(roles, QList<int>{Oma2048TileModel::ColRole});
    QCOMPARE(tiles->rowCount(), 3);
    QCOMPARE(cellValue(tiles, 0, 3), 2);
    QCOMPARE(cellValue(tiles, 1, 3), 4);
    QVERIFY(game.canUndo());
    QCOMPARE(undoable.count(), 1);
}

void BridgeTests::mergeRemovesTheMergedTileAndAppendsTheSpawn() {
    seedState({{0, 2, 2}, {0, 3, 2}});
    Oma2048Game game(kSeed);
    QAbstractListModel *tiles = game.tiles();
    const int survivorId = cellId(tiles, 0, 3);

    QSignalSpy resets(tiles, &QAbstractItemModel::modelReset);
    QSignalSpy removals(tiles, &QAbstractItemModel::rowsRemoved);
    QSignalSpy insertions(tiles, &QAbstractItemModel::rowsInserted);
    QSignalSpy changes(tiles, &QAbstractItemModel::dataChanged);
    QSignalSpy scored(&game, &Oma2048Game::scoreChanged);

    game.moveRight();
    QCOMPARE(resets.count(), 0);
    QCOMPARE(removals.count(), 1);
    QCOMPARE(insertions.count(), 1);
    // The tile that was slid onto keeps its id and place; only its value pops.
    QCOMPARE(changes.count(), 1);
    QCOMPARE(changes.at(0).at(2).value<QList<int>>(), QList<int>{Oma2048TileModel::ValueRole});
    QCOMPARE(cellId(tiles, 0, 3), survivorId);
    QCOMPARE(cellValue(tiles, 0, 3), 4);
    QCOMPARE(tiles->rowCount(), 2);
    QCOMPARE(game.score(), 4);
    QCOMPARE(scored.count(), 1);
}

void BridgeTests::winningShowsTheOverlayOnceAndKeepGoingClearsIt() {
    seedState({{0, 2, 1024}, {0, 3, 1024}}, 500);
    Oma2048Game game(kSeed);
    QSignalSpy wins(&game, &Oma2048Game::wonChanged);
    QSignalSpy continues(&game, &Oma2048Game::keepPlayingChanged);

    game.moveRight();
    QVERIFY(game.won());
    QVERIFY(!game.keepPlaying());
    QCOMPARE(wins.count(), 1);
    QCOMPARE(game.score(), 500 + 2048);

    game.keepGoing();
    QVERIFY(!game.won());
    QVERIFY(game.keepPlaying());
    QCOMPARE(wins.count(), 2);
    QCOMPARE(continues.count(), 1);
    game.keepGoing();
    QCOMPARE(wins.count(), 2);
    QCOMPARE(continues.count(), 1);

    // A relaunched winning run was already congratulated.
    seedState({{0, 0, 2048}, {1, 1, 2}}, 20000, true);
    Oma2048Game restored(kSeed);
    QVERIFY(!restored.won());
    QVERIFY(restored.keepPlaying());
}

void BridgeTests::gameOverRecordsTheScoreOnce() {
    seedState(oneMoveFromOver(), 300);
    Oma2048Game game(kSeed);
    QSignalSpy overs(&game, &Oma2048Game::overChanged);
    QSignalSpy tables(&game, &Oma2048Game::scoresChanged);
    QSignalSpy ranks(&game, &Oma2048Game::lastRankChanged);

    game.moveRight();
    QVERIFY(game.over());
    QCOMPARE(overs.count(), 1);
    QCOMPARE(tables.count(), 1);
    QCOMPARE(game.lastRank(), 0);
    QCOMPARE(game.bestScore(), 300);
    QVERIFY(!game.canUndo());
    QCOMPARE(game.scores().size(), 1);
    const QVariantMap entry = game.scores().first().toMap();
    QCOMPARE(entry.value(QStringLiteral("score")).toInt(), 300);
    QCOMPARE(entry.value(QStringLiteral("tile")).toInt(), 1024);
    QCOMPARE(entry.value(QStringLiteral("category")).toString(), QStringLiteral("classic"));
    QCOMPARE(entry.value(QStringLiteral("date")).toString(), QDate::currentDate().toString(Qt::ISODate));

    // The end of the run: nothing moves, nothing records again.
    game.moveLeft();
    game.moveUp();
    game.undo();
    QCOMPARE(game.score(), 300);
    QCOMPARE(tables.count(), 1);
    QCOMPARE(game.scores().size(), 1);

    // The finished run cleared state/v1; the table it made outlives it.
    Oma2048Game relaunched(kSeed + 1);
    QCOMPARE(relaunched.score(), 0);
    QCOMPARE(relaunched.tiles()->rowCount(), 2);
    QCOMPARE(relaunched.bestScore(), 300);

    game.newGame();
    QCOMPARE(game.lastRank(), -1);
    QCOMPARE(ranks.count(), 2);
}

void BridgeTests::aRunBelowTheTableGetsNoRank() {
    seedFullScoreTable();
    seedState(oneMoveFromOver(), 300);
    Oma2048Game game(kSeed);
    QCOMPARE(game.scores().size(), 10);
    QCOMPARE(game.bestScore(), 1010);

    QSignalSpy tables(&game, &Oma2048Game::scoresChanged);
    game.moveRight();
    QVERIFY(game.over());
    QCOMPARE(game.lastRank(), -1);
    QCOMPARE(tables.count(), 0);
    QCOMPARE(game.scores().size(), 10);
    QCOMPARE(game.bestScore(), 1010);
}

void BridgeTests::undoRevertsThroughTheBridgeAndSyncsTheModel() {
    seedState({{0, 0, 2}, {0, 1, 2}}, 10);
    Oma2048Game game(kSeed);
    QAbstractListModel *tiles = game.tiles();
    game.moveRight();
    QCOMPARE(game.score(), 14);

    QSignalSpy resets(tiles, &QAbstractItemModel::modelReset);
    QSignalSpy removals(tiles, &QAbstractItemModel::rowsRemoved);
    QSignalSpy insertions(tiles, &QAbstractItemModel::rowsInserted);
    QSignalSpy scored(&game, &Oma2048Game::scoreChanged);
    QSignalSpy undoable(&game, &Oma2048Game::canUndoChanged);

    game.undo();
    QCOMPARE(game.score(), 10);
    QVERIFY(!game.canUndo());
    QCOMPARE(scored.count(), 1);
    QCOMPARE(undoable.count(), 1);
    // The spawn goes, the merged-away tile comes back; no reset.
    QCOMPARE(resets.count(), 0);
    QCOMPARE(removals.count(), 1);
    QCOMPARE(insertions.count(), 1);
    QCOMPARE(tiles->rowCount(), 2);
    QCOMPARE(cellValue(tiles, 0, 0), 2);
    QCOMPARE(cellValue(tiles, 0, 1), 2);

    game.undo();
    QCOMPARE(game.score(), 10);
    QCOMPARE(scored.count(), 1);

    // The undone position is what a relaunch comes back to.
    Oma2048Game relaunched(kSeed + 1);
    QCOMPARE(relaunched.score(), 10);
    QCOMPARE(cellSet(relaunched.tiles()), cellSet(tiles));
}

void BridgeTests::stateRoundTripsThroughAFreshBridge() {
    Oma2048Game game(kSeed);
    game.moveLeft();
    game.moveUp();
    game.moveRight();
    game.moveDown();
    QVERIFY(game.canUndo());

    Oma2048Game relaunched(kSeed + 99);
    QCOMPARE(relaunched.score(), game.score());
    QCOMPARE(relaunched.won(), game.won());
    QCOMPARE(relaunched.over(), game.over());
    QCOMPARE(cellSet(relaunched.tiles()), cellSet(game.tiles()));
}

void BridgeTests::restoreLeavesUndoUnavailable() {
    seedState({{0, 0, 2}, {0, 1, 2}}, 10);
    Oma2048Game game(kSeed);
    QVERIFY(!game.canUndo());

    QSignalSpy scored(&game, &Oma2048Game::scoreChanged);
    game.undo();
    QCOMPARE(game.score(), 10);
    QCOMPARE(scored.count(), 0);
    QCOMPARE(game.tiles()->rowCount(), 2);
}

void BridgeTests::newGameClearsTheRun() {
    seedState({{0, 2, 1024}, {0, 3, 1024}}, 500);
    Oma2048Game game(kSeed);
    game.moveRight();
    game.keepGoing();
    QVERIFY(game.keepPlaying());
    QVERIFY(game.canUndo());

    QAbstractListModel *tiles = game.tiles();
    QSignalSpy resets(tiles, &QAbstractItemModel::modelReset);
    game.newGame();
    QCOMPARE(resets.count(), 1);
    QCOMPARE(tiles->rowCount(), 2);
    QCOMPARE(game.score(), 0);
    QVERIFY(!game.won());
    QVERIFY(!game.keepPlaying());
    QVERIFY(!game.over());
    QVERIFY(!game.canUndo());
    QCOMPARE(game.lastRank(), -1);

    // The fresh run replaced the old one on disk too.
    Oma2048Game relaunched(kSeed + 1);
    QCOMPARE(relaunched.score(), 0);
    QCOMPARE(cellSet(relaunched.tiles()), cellSet(tiles));
}

void BridgeTests::windowGeometryRoundTrips() {
    Oma2048Game game(kSeed);
    QVERIFY(!game.windowGeometry().value(QStringLiteral("valid")).toBool());
    game.saveWindowGeometry(-20, 30, 1000, 700, true);
    const QVariantMap geometry = game.windowGeometry();
    QVERIFY(geometry.value(QStringLiteral("valid")).toBool());
    QCOMPARE(geometry.value(QStringLiteral("x")).toInt(), -20);
    QCOMPARE(geometry.value(QStringLiteral("width")).toInt(), 1000);
    QVERIFY(geometry.value(QStringLiteral("maximized")).toBool());
}
