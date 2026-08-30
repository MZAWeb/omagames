#include "fieldpainter.h"

#include <QPainter>
#include <algorithm>

namespace {

// Below this the grid lines and the bevel eat the cell instead of shaping it.
constexpr int kMinDetailCell = 9;

QColor faded(QColor color, double opacity) {
    color.setAlphaF(color.alphaF() * opacity);
    return color;
}

}  // namespace

const QColor &FieldPalette::number(int adjacent) const {
    const int index = std::clamp(adjacent, 1, int(numbers.size())) - 1;
    return numbers[size_t(index)];
}

FieldPainter::FieldPainter(QPainter *painter, int cellSize, const FieldPalette &palette)
    : m_painter(painter), m_cellSize(cellSize), m_palette(palette) {}

void FieldPainter::lid(const QRect &box, double opacity) const {
    m_painter->setPen(Qt::NoPen);
    m_painter->fillRect(box, faded(m_palette.hidden, opacity));
    if (m_cellSize < kMinDetailCell)
        return;
    const double bevel = std::max(1.0, m_cellSize / 12.0);
    m_painter->setPen(QPen(faded(m_palette.hiddenHighlight, opacity), bevel));
    m_painter->drawLine(QPointF(box.left(), box.top() + bevel / 2),
                        QPointF(box.right() + 1, box.top() + bevel / 2));
    m_painter->drawLine(QPointF(box.left() + bevel / 2, box.top()),
                        QPointF(box.left() + bevel / 2, box.bottom() + 1));
    m_painter->setPen(QPen(faded(m_palette.hiddenShadow, opacity), bevel));
    m_painter->drawLine(QPointF(box.left(), box.bottom() + 1 - bevel / 2),
                        QPointF(box.right() + 1, box.bottom() + 1 - bevel / 2));
    m_painter->drawLine(QPointF(box.right() + 1 - bevel / 2, box.top()),
                        QPointF(box.right() + 1 - bevel / 2, box.bottom() + 1));
}

void FieldPainter::number(const QRect &box, int adjacent, double opacity) const {
    if (adjacent <= 0 || opacity <= 0.0)
        return;
    QFont font = m_painter->font();
    font.setPixelSize(std::max(7, int(m_cellSize * 0.62)));
    font.setBold(true);
    m_painter->setFont(font);
    m_painter->setPen(faded(m_palette.number(adjacent), opacity));
    m_painter->drawText(box, Qt::AlignCenter, QString::number(adjacent));
}

void FieldPainter::flag(const QRect &box, const QColor &color) const {
    const double c = m_cellSize;
    const QPointF top(box.left() + c * 0.38, box.top() + c * 0.20);
    const QPointF foot(box.left() + c * 0.38, box.top() + c * 0.76);
    m_painter->setPen(QPen(color, std::max(1.0, c * 0.09), Qt::SolidLine, Qt::RoundCap));
    m_painter->drawLine(top, foot);
    m_painter->drawLine(QPointF(box.left() + c * 0.24, foot.y()), QPointF(box.left() + c * 0.72, foot.y()));
    const QPointF pennant[] = {top, QPointF(box.left() + c * 0.76, box.top() + c * 0.34),
                               QPointF(box.left() + c * 0.38, box.top() + c * 0.48)};
    m_painter->setPen(Qt::NoPen);
    m_painter->setBrush(color);
    m_painter->drawPolygon(pennant, 3);
}

void FieldPainter::mine(const QRect &box) const {
    const double r = m_cellSize * 0.28;
    m_painter->setPen(Qt::NoPen);
    m_painter->setBrush(m_palette.mine);
    m_painter->drawEllipse(QPointF(box.center()) + QPointF(0.5, 0.5), r, r);
}

void FieldPainter::cross(const QRect &box) const {
    const double inset = m_cellSize * 0.24;
    m_painter->setBrush(Qt::NoBrush);
    m_painter->setPen(QPen(m_palette.wrongFlag, std::max(1.0, m_cellSize * 0.09), Qt::SolidLine, Qt::RoundCap));
    m_painter->drawLine(QPointF(box.left() + inset, box.top() + inset),
                        QPointF(box.right() + 1 - inset, box.bottom() + 1 - inset));
    m_painter->drawLine(QPointF(box.right() + 1 - inset, box.top() + inset),
                        QPointF(box.left() + inset, box.bottom() + 1 - inset));
}

void FieldPainter::exploded(const QRect &box) const {
    m_painter->fillRect(box, m_palette.exploded);
}

void FieldPainter::grid(QPoint origin, int columns, int rows) const {
    if (m_cellSize < kMinDetailCell)
        return;
    m_painter->setBrush(Qt::NoBrush);
    m_painter->setPen(QPen(m_palette.grid, 1));
    for (int x = 0; x <= columns; ++x)
        m_painter->drawLine(QPointF(origin.x() + x * m_cellSize + 0.5, origin.y()),
                            QPointF(origin.x() + x * m_cellSize + 0.5, origin.y() + rows * m_cellSize));
    for (int y = 0; y <= rows; ++y)
        m_painter->drawLine(QPointF(origin.x(), origin.y() + y * m_cellSize + 0.5),
                            QPointF(origin.x() + columns * m_cellSize, origin.y() + y * m_cellSize + 0.5));
}

void FieldPainter::cursor(const QRectF &box, double opacity) const {
    m_painter->setBrush(Qt::NoBrush);
    m_painter->setPen(QPen(faded(m_palette.cursor, opacity), 2));
    m_painter->drawRoundedRect(box, 3, 3);
}
