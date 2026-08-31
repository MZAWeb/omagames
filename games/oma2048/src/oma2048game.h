#pragma once

#include <QAbstractListModel>
#include <QObject>
#include <QVariantList>
#include <QVariantMap>

#include "game.h"
#include "scoretable.h"

// The board's tiles as rows QML delegates follow across moves. Rows keep
// their identity by tile id: a plain move only updates surviving rows,
// removes merged-away ones and appends the spawn; only a new game or a
// restore resets. Implemented in oma2048gamemodel.cpp.
class Oma2048TileModel : public QAbstractListModel {
    Q_OBJECT

public:
    enum Roles {
        TileIdRole = Qt::UserRole + 1,
        ValueRole,
        RowRole,
        ColRole,
    };

    explicit Oma2048TileModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Diffs the board against the rows by tile id, never reordering them.
    void sync(const QVector<Tile> &tiles);
    void reset(const QVector<Tile> &tiles);

private:
    int rowOf(int tileId) const;

    QVector<Tile> m_tiles;
};

// The only bridge between the engine and QML: the run's state as properties,
// the four slides and the buttons as invokables, the tile model, the kept
// scores and the state/v1 persistence.
class Oma2048Game : public QObject {
    Q_OBJECT
    Q_PROPERTY(int score READ score NOTIFY scoreChanged)
    Q_PROPERTY(int bestScore READ bestScore NOTIFY bestScoreChanged)
    Q_PROPERTY(bool over READ over NOTIFY overChanged)
    Q_PROPERTY(bool won READ won NOTIFY wonChanged)
    Q_PROPERTY(bool keepPlaying READ keepPlaying NOTIFY keepPlayingChanged)
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY canUndoChanged)
    Q_PROPERTY(int boardSize READ boardSize CONSTANT)
    Q_PROPERTY(QAbstractListModel *tiles READ tiles CONSTANT)
    Q_PROPERTY(QVariantList scores READ scores NOTIFY scoresChanged)
    Q_PROPERTY(int lastRank READ lastRank NOTIFY lastRankChanged)

public:
    explicit Oma2048Game(QObject *parent = nullptr);
    // Tests fix the seed the spawns are drawn from.
    explicit Oma2048Game(quint32 seed, QObject *parent = nullptr);

    int score() const { return m_game.score(); }
    // The top of the kept table, 0 before any run made it.
    int bestScore() const;
    bool over() const { return m_game.over(); }
    // True while the win overlay should be up: the run reached 2048 and Keep
    // going has not been pressed. A restored win never re-shows it.
    bool won() const { return m_game.won() && !m_game.keepPlaying(); }
    bool keepPlaying() const { return m_game.keepPlaying(); }
    bool canUndo() const { return m_game.canUndo(); }
    static int boardSize() { return Board::kSize; }
    QAbstractListModel *tiles() { return &m_tiles; }
    // {score, tile, date, category} per kept run, best first.
    QVariantList scores() const;
    // Where the finished run landed in the table (0 = top), or -1.
    int lastRank() const { return m_lastRank; }

    Q_INVOKABLE void moveLeft() { applyMove(Direction::Left); }
    Q_INVOKABLE void moveRight() { applyMove(Direction::Right); }
    Q_INVOKABLE void moveUp() { applyMove(Direction::Up); }
    Q_INVOKABLE void moveDown() { applyMove(Direction::Down); }
    Q_INVOKABLE void newGame();
    Q_INVOKABLE void undo();
    Q_INVOKABLE void keepGoing();

    Q_INVOKABLE QVariantMap windowGeometry() const;
    Q_INVOKABLE void saveWindowGeometry(int x, int y, int width, int height, bool maximized);

signals:
    void scoreChanged();
    void bestScoreChanged();
    void overChanged();
    void wonChanged();
    void keepPlayingChanged();
    void canUndoChanged();
    void scoresChanged();
    void lastRankChanged();

private:
    // What QML was last told, so every action announces exactly its diffs.
    struct Shown {
        int score = 0;
        bool over = false;
        bool won = false;
        bool keepPlaying = false;
        bool canUndo = false;
    };

    void applyMove(Direction direction);
    void announce();
    void finishRun();
    bool restoreSavedState();
    void saveState() const;
    void clearState() const;

    Game m_game;
    Oma2048TileModel m_tiles;
    OmaGames::ScoreTable m_scores;
    Shown m_shown;
    int m_lastRank = -1;
};
