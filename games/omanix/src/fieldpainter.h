#pragma once

#include <QColor>
#include <QSizeF>

class QPainter;
class Field;

// The bottom layer of the board — the open background and every claimed
// cell — and the trail, as pure functions painting one rectangle per
// horizontal run, with tests proving the runs pixel-identical to painting
// cell by cell. Everything animated stays in FieldView.
namespace FieldPaint {

struct GroundStyle {
    int cellSize = 8;
    QSizeF itemSize;
    QColor open;
    QColor claimed;
};

void paintGround(QPainter *painter, const Field &field, const GroundStyle &style);
void paintTrail(QPainter *painter, const Field &field, int cellSize, const QColor &color);

}  // namespace FieldPaint
