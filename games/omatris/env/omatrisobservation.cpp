#include "omatrisobservation.h"

#include <QStringList>

using OmaGames::ObservationLayout;

namespace {

constexpr int kLandingColumns = 9;

int pieceNumber(PieceType type) {
    return type == PieceType::None ? 0 : int(type) + 1;
}

QStringList labels(std::initializer_list<const char *> names) {
    QStringList list;
    for (const char *name : names)
        list << QString::fromLatin1(name);
    return list;
}

}  // namespace

OmatrisObservation::OmatrisObservation(int candidates) : m_candidates(candidates) {
    using DType = ObservationLayout::DType;
    m_board = m_layout.add(QStringLiteral("board"), DType::U8, {Board::kHeight, Board::kWidth});
    m_piece = m_layout.add(QStringLiteral("piece"), DType::I32, {4}, labels({"piece", "rotation", "x", "y"}));
    m_queue = m_layout.add(QStringLiteral("queue"), DType::U8, {Rules::kNextQueue});
    m_hold = m_layout.add(QStringLiteral("hold"), DType::U8, {2}, labels({"piece", "available"}));
    m_stats = m_layout.add(QStringLiteral("stats"), DType::I32, {11},
                           labels({"level", "gravity_level", "lines", "score", "combo", "back_to_back",
                                   "lock_ticks", "lock_resets", "ticks", "lines_left", "dealt_rows_left"}));
    if (candidates <= 0)
        return;
    m_landingRows = m_layout.add(QStringLiteral("candidates"), DType::I32, {candidates, kLandingColumns},
                                 labels({"valid", "hold", "piece", "rotation", "column", "row", "lines", "spin",
                                         "topped_out"}));
    m_afterstates = m_layout.add(QStringLiteral("afterstates"), DType::U8,
                                 {candidates, Board::kHeight, Board::kWidth});
}

void OmatrisObservation::write(std::byte *buffer, const Game &game, const std::vector<Landing> &landings) const {
    quint8 *board = m_layout.at<quint8>(buffer, m_board);
    for (int i = 0; i < Board::kCellCount; ++i)
        board[i] = quint8(pieceNumber(game.board().at(i)));

    if (game.hasPiece()) {
        qint32 *piece = m_layout.at<qint32>(buffer, m_piece);
        piece[0] = pieceNumber(game.piece().type);
        piece[1] = game.piece().rotation;
        piece[2] = game.piece().origin.x();
        piece[3] = game.piece().origin.y();
    }
    quint8 *queue = m_layout.at<quint8>(buffer, m_queue);
    const std::vector<PieceType> next = game.nextQueue();
    for (size_t i = 0; i < next.size(); ++i)
        queue[i] = quint8(pieceNumber(next[i]));
    quint8 *hold = m_layout.at<quint8>(buffer, m_hold);
    hold[0] = quint8(pieceNumber(game.heldPiece()));
    hold[1] = game.holdAvailable() ? 1 : 0;

    const qint32 stats[] = {game.level(), game.gravityLevel(), game.lines(), game.score(), game.combo(),
                            game.backToBack() ? 1 : 0, game.lockTicks(), game.lockResets(), game.ticks(),
                            game.linesLeft(), game.dealtRowsLeft()};
    std::copy(std::begin(stats), std::end(stats), m_layout.at<qint32>(buffer, m_stats));

    if (m_candidates <= 0)
        return;
    qint32 *rows = m_layout.at<qint32>(buffer, m_landingRows);
    quint8 *afterstates = m_layout.at<quint8>(buffer, m_afterstates);
    const int count = std::min(int(landings.size()), m_candidates);
    for (int i = 0; i < count; ++i) {
        const Landing &landing = landings[size_t(i)];
        const qint32 row[kLandingColumns] = {1,
                                             landing.hold ? 1 : 0,
                                             pieceNumber(landing.placement.type),
                                             landing.placement.rotation,
                                             Placements::leftColumn(landing.placement),
                                             Placements::bottomRow(landing.placement),
                                             landing.lines,
                                             int(landing.spin),
                                             landing.toppedOut ? 1 : 0};
        std::copy(std::begin(row), std::end(row), rows + i * kLandingColumns);
        quint8 *cells = afterstates + i * Board::kCellCount;
        for (int c = 0; c < Board::kCellCount; ++c)
            cells[c] = landing.afterstate.at(c) == PieceType::None ? 0 : 1;
    }
}
