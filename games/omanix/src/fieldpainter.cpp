#include "fieldpainter.h"

#include <QPainter>

#include "field.h"

namespace FieldPaint {

// One fillRect per horizontal run of claimed cells instead of one per cell:
// claimed ground is made of large blocks, so a nearly finished level paints
// a few dozen rectangles rather than two thousand.
void paintGround(QPainter *painter, const Field &field, const GroundStyle &style) {
    painter->fillRect(QRectF(QPointF(0, 0), style.itemSize), style.open);
    painter->setPen(Qt::NoPen);
    const int c = style.cellSize;
    for (int y = 0; y < field.height(); ++y) {
        int runStart = -1;
        for (int x = 0; x <= field.width(); ++x) {
            const bool claimed = x < field.width() && field.at({x, y}) == Cell::Claimed;
            if (claimed && runStart < 0)
                runStart = x;
            else if (!claimed && runStart >= 0) {
                painter->fillRect(runStart * c, y * c, (x - runStart) * c, c, style.claimed);
                runStart = -1;
            }
        }
    }
}

// The same coalescing for the trail, painted over the ground every frame in
// whatever color the pulse asks for.
void paintTrail(QPainter *painter, const Field &field, int cellSize, const QColor &color) {
    painter->setPen(Qt::NoPen);
    const int c = cellSize;
    for (int y = 0; y < field.height(); ++y) {
        int runStart = -1;
        for (int x = 0; x <= field.width(); ++x) {
            const bool trail = x < field.width() && field.at({x, y}) == Cell::Trail;
            if (trail && runStart < 0)
                runStart = x;
            else if (!trail && runStart >= 0) {
                painter->fillRect(runStart * c, y * c, (x - runStart) * c, c, color);
                runStart = -1;
            }
        }
    }
}

}  // namespace FieldPaint
