// Oma2048TileModel: the tile rows QML animates. A row's identity is its tile
// id for the tile's whole life, which is why rows are never reordered — a
// delegate that saw a slide as dataChanged must not also see its row move.
#include "oma2048game.h"

namespace {

const Tile *tileById(const QVector<Tile> &tiles, int id) {
    for (const Tile &tile : tiles) {
        if (tile.id == id)
            return &tile;
    }
    return nullptr;
}

}  // namespace

Oma2048TileModel::Oma2048TileModel(QObject *parent) : QAbstractListModel(parent) {}

int Oma2048TileModel::rowCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : m_tiles.size();
}

QVariant Oma2048TileModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_tiles.size())
        return {};
    const Tile &tile = m_tiles.at(index.row());
    switch (role) {
    case TileIdRole:
        return tile.id;
    case ValueRole:
        return tile.value;
    case RowRole:
        return tile.row;
    case ColRole:
        return tile.col;
    }
    return {};
}

QHash<int, QByteArray> Oma2048TileModel::roleNames() const {
    return {{TileIdRole, QByteArrayLiteral("tileId")},
            {ValueRole, QByteArrayLiteral("value")},
            {RowRole, QByteArrayLiteral("row")},
            {ColRole, QByteArrayLiteral("col")}};
}

void Oma2048TileModel::sync(const QVector<Tile> &tiles) {
    // Walk backwards so removing a merged-away row leaves earlier rows put.
    for (int i = m_tiles.size() - 1; i >= 0; --i) {
        const Tile *now = tileById(tiles, m_tiles.at(i).id);
        if (!now) {
            beginRemoveRows(QModelIndex(), i, i);
            m_tiles.removeAt(i);
            endRemoveRows();
            continue;
        }
        QList<int> roles;
        if (now->value != m_tiles.at(i).value)
            roles.append(ValueRole);
        if (now->row != m_tiles.at(i).row)
            roles.append(RowRole);
        if (now->col != m_tiles.at(i).col)
            roles.append(ColRole);
        if (roles.isEmpty())
            continue;
        m_tiles[i] = *now;
        emit dataChanged(index(i), index(i), roles);
    }
    // Whatever the board has that the rows do not — the spawn, or on an undo
    // the tiles the move had merged away — is appended, keeping every
    // surviving row where it was.
    for (const Tile &tile : tiles) {
        if (rowOf(tile.id) >= 0)
            continue;
        beginInsertRows(QModelIndex(), m_tiles.size(), m_tiles.size());
        m_tiles.append(tile);
        endInsertRows();
    }
}

void Oma2048TileModel::reset(const QVector<Tile> &tiles) {
    beginResetModel();
    m_tiles = tiles;
    endResetModel();
}

int Oma2048TileModel::rowOf(int tileId) const {
    for (int i = 0; i < m_tiles.size(); ++i) {
        if (m_tiles.at(i).id == tileId)
            return i;
    }
    return -1;
}
