#include "painttests.h"

#include <QImage>
#include <QPainter>
#include <QtTest>

#include "field.h"
#include "fieldpainter.h"

namespace {

// The shapes run-coalescing has to get right: blocks, lone cells, an
// alternating comb, full rows, and cells against every edge of the frame.
Field trickyField() {
    Field field;
    for (int y = 5; y < 12; ++y)
        for (int x = 4; x < 20; ++x)
            field.set({x, y}, Cell::Claimed);
    field.set({30, 20}, Cell::Claimed);
    for (int x = 2; x < 30; x += 2)
        field.set({x, 25}, Cell::Claimed);
    for (int x = 0; x < field.width(); ++x)
        field.set({x, 30}, Cell::Claimed);
    for (int x = 10; x < 25; ++x)
        field.set({x, 15}, Cell::Trail);
    for (int y = 15; y < 34; y += 3)
        field.set({35, y}, Cell::Trail);
    return field;
}

// Painting every cell with its own fillRect is the obviously correct
// reference the optimised painters are held against.
void paintCellByCell(QPainter *painter, const Field &field, int c, Cell wanted, const QColor &color) {
    painter->setPen(Qt::NoPen);
    for (int y = 0; y < field.height(); ++y)
        for (int x = 0; x < field.width(); ++x)
            if (field.at({x, y}) == wanted)
                painter->fillRect(x * c, y * c, c, c, color);
}

QImage blank(QSizeF size, qreal dpr) {
    QImage image((size * dpr).toSize(), QImage::Format_ARGB32_Premultiplied);
    image.setDevicePixelRatio(dpr);
    image.fill(Qt::black);
    return image;
}

}  // namespace

// The guard #29 lacked: the optimised painters must produce the same pixels
// as the cell-by-cell reference, including at a fractional item size and a
// high-dpi scale.
void PaintTests::groundRunsArePixelIdenticalToCellByCell() {
    const Field field = trickyField();
    const QColor open("#101018");
    const QColor claimed("#5a6ea5");
    for (const QSizeF &itemSize : {QSizeF(512, 320), QSizeF(517, 323)}) {
        for (qreal dpr : {1.0, 2.0}) {
            QImage reference = blank(itemSize, dpr);
            {
                QPainter p(&reference);
                p.fillRect(QRectF(QPointF(0, 0), itemSize), open);
                paintCellByCell(&p, field, 8, Cell::Claimed, claimed);
            }
            QImage optimised = blank(itemSize, dpr);
            {
                QPainter p(&optimised);
                FieldPaint::paintGround(&p, field, {8, itemSize, open, claimed});
            }
            QCOMPARE(optimised, reference);
        }
    }
}

void PaintTests::trailRunsArePixelIdenticalToCellByCell() {
    const Field field = trickyField();
    const QColor trail("#e0af68");
    QImage reference = blank({520, 328}, 1.0);
    {
        QPainter p(&reference);
        paintCellByCell(&p, field, 8, Cell::Trail, trail);
    }
    QImage optimised = blank({520, 328}, 1.0);
    {
        QPainter p(&optimised);
        FieldPaint::paintTrail(&p, field, 8, trail);
    }
    QCOMPARE(optimised, reference);
}
