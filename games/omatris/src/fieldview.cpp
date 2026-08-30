#include "fieldview.h"

#include <QPainter>
#include <algorithm>

#include "omarchytheme.h"
#include "omatrisgame.h"

namespace {

// A piece flares as it locks; a full row flares for as long as the engine
// holds it on the board before the stack falls.
constexpr int kLockFlashMs = 120;
constexpr int kRowFlashMs = Rules::kClearDelayMs;
constexpr int kFrameMs = 16;
// Below this a cell is too small to carry a bevel or sit on a grid.
constexpr int kMinDetailCell = 8;

double fade(qint64 elapsed, int duration) {
    return std::clamp(1.0 - double(elapsed) / duration, 0.0, 1.0);
}

}  // namespace

FieldView::FieldView(QQuickItem *parent) : QQuickPaintedItem(parent) {
    m_clock.start();
    m_animation.setInterval(kFrameMs);
    connect(&m_animation, &QTimer::timeout, this, &FieldView::animate);
    connect(this, &FieldView::colorsChanged, this, &QQuickItem::update);
}

QObject *FieldView::source() const {
    return m_source;
}

void FieldView::setSource(QObject *source) {
    OmatrisGame *game = qobject_cast<OmatrisGame *>(source);
    if (m_source == game)
        return;
    if (m_source)
        disconnect(m_source, nullptr, this, nullptr);
    m_source = game;
    if (m_source) {
        connect(m_source, &OmatrisGame::frameChanged, this, &QQuickItem::update);
        connect(m_source, &OmatrisGame::pieceLocked, this, &FieldView::onLocked);
        connect(m_source, &OmatrisGame::linesCleared, this, &FieldView::onCleared);
    }
    emit sourceChanged();
    update();
}

void FieldView::setCellSize(int cellSize) {
    if (m_cellSize == cellSize)
        return;
    m_cellSize = cellSize;
    emit cellSizeChanged();
    update();
}

void FieldView::setShowGhost(bool show) {
    if (m_showGhost == show)
        return;
    m_showGhost = show;
    emit showGhostChanged();
    update();
}

void FieldView::setPieceColors(const QVariantList &colors) {
    if (m_pieceColors == colors)
        return;
    m_pieceColors = colors;
    m_colors.clear();
    for (const QVariant &color : colors)
        m_colors.append(color.value<QColor>());
    emit colorsChanged();
}

const Game *FieldView::engine() const {
    return m_source ? m_source->engine() : nullptr;
}

void FieldView::onLocked(const QVector<int> &cells) {
    m_lockFlash = cells;
    m_lockFlashStart = m_clock.elapsed();
    m_animation.start();
}

void FieldView::onCleared(const QVector<int> &rows) {
    m_rowFlash = rows;
    m_rowFlashStart = m_clock.elapsed();
    m_animation.start();
}

void FieldView::animate() {
    const qint64 now = m_clock.elapsed();
    if (m_lockFlashStart >= 0 && now - m_lockFlashStart >= kLockFlashMs) {
        m_lockFlash.clear();
        m_lockFlashStart = -1;
    }
    if (m_rowFlashStart >= 0 && now - m_rowFlashStart >= kRowFlashMs) {
        m_rowFlash.clear();
        m_rowFlashStart = -1;
    }
    if (m_lockFlashStart < 0 && m_rowFlashStart < 0)
        m_animation.stop();
    update();
}

QColor FieldView::colorFor(int piece) const {
    return piece >= 0 && piece < m_colors.size() ? m_colors.at(piece) : m_emptyColor;
}

void FieldView::paintCell(QPainter *painter, int column, int row, const QColor &color) {
    const int c = m_cellSize;
    const QRect box(column * c, row * c, c, c);
    painter->fillRect(box, color);
    if (c < kMinDetailCell)
        return;
    // A one-cell bevel: lit along the top and left, shaded along the other two.
    const int lip = std::max(1, c / 8);
    painter->fillRect(box.x(), box.y(), box.width(), lip, OmarchyTheme::mix(color, m_lightColor, 0.38));
    painter->fillRect(box.x(), box.y(), lip, box.height(), OmarchyTheme::mix(color, m_lightColor, 0.22));
    painter->fillRect(box.x(), box.bottom() - lip + 1, box.width(), lip, OmarchyTheme::mix(color, m_shadeColor, 0.38));
    painter->fillRect(box.right() - lip + 1, box.y(), lip, box.height(), OmarchyTheme::mix(color, m_shadeColor, 0.22));
}

void FieldView::paintGrid(QPainter *painter) {
    if (m_cellSize < kMinDetailCell)
        return;
    painter->setPen(QPen(m_gridColor, 1));
    for (int x = 1; x < Board::kWidth; ++x)
        painter->drawLine(QPointF(x * m_cellSize + 0.5, 0), QPointF(x * m_cellSize + 0.5, height()));
    for (int y = 1; y < Board::kVisibleHeight; ++y)
        painter->drawLine(QPointF(0, y * m_cellSize + 0.5), QPointF(width(), y * m_cellSize + 0.5));
}

void FieldView::paintStack(QPainter *painter, const Game &game) {
    const Board &board = game.board();
    for (int y = Board::kHiddenRows; y < Board::kHeight; ++y) {
        for (int x = 0; x < Board::kWidth; ++x) {
            const PieceType cell = board.at({x, y});
            if (cell != PieceType::None)
                paintCell(painter, x, y - Board::kHiddenRows, colorFor(int(cell)));
        }
    }
}

void FieldView::paintGhostAndPiece(QPainter *painter, const Game &game) {
    if (!game.hasPiece())
        return;
    const int c = m_cellSize;
    const QColor color = colorFor(int(game.piece().type));

    if (m_showGhost) {
        const double pen = std::max(1.0, c / 10.0);
        painter->setBrush(Qt::NoBrush);
        painter->setPen(QPen(OmarchyTheme::mix(m_ghostColor, color, 0.5), pen));
        for (QPoint cell : game.ghost().cells()) {
            const int row = cell.y() - Board::kHiddenRows;
            if (row >= 0)
                painter->drawRect(QRectF(cell.x() * c + pen / 2, row * c + pen / 2, c - pen, c - pen));
        }
        painter->setPen(Qt::NoPen);
    }

    for (QPoint cell : game.piece().cells()) {
        const int row = cell.y() - Board::kHiddenRows;
        if (row >= 0)
            paintCell(painter, cell.x(), row, color);
    }
}

void FieldView::paintFlashes(QPainter *painter, qint64 now) {
    const int c = m_cellSize;
    if (m_rowFlashStart >= 0) {
        QColor tint = m_flashColor;
        tint.setAlphaF(float(fade(now - m_rowFlashStart, kRowFlashMs)));
        for (int y : m_rowFlash) {
            const int row = y - Board::kHiddenRows;
            if (row >= 0)
                painter->fillRect(0, row * c, int(width()), c, tint);
        }
    }
    if (m_lockFlashStart >= 0) {
        QColor tint = m_flashColor;
        tint.setAlphaF(float(0.7 * fade(now - m_lockFlashStart, kLockFlashMs)));
        for (int index : m_lockFlash) {
            const QPoint cell = Board::point(index);
            const int row = cell.y() - Board::kHiddenRows;
            if (row >= 0)
                painter->fillRect(cell.x() * c, row * c, c, c, tint);
        }
    }
}

void FieldView::paint(QPainter *painter) {
    painter->fillRect(boundingRect(), m_emptyColor);
    const Game *game = engine();
    if (!game)
        return;
    painter->setPen(Qt::NoPen);
    paintGrid(painter);
    painter->setPen(Qt::NoPen);
    paintStack(painter, *game);
    paintGhostAndPiece(painter, *game);
    paintFlashes(painter, m_clock.elapsed());
}
