#pragma once

#include <QColor>
#include <QRect>
#include <array>

class QPainter;

// Every color the field is painted with. They all arrive from QML, so the
// palette is the theme's and this file names none of them.
struct FieldPalette {
    QColor hidden;
    QColor hiddenHighlight;
    QColor hiddenShadow;
    QColor revealed;
    QColor grid;
    QColor cursor;
    QColor flag;
    QColor mine;
    QColor exploded;
    QColor wrongFlag;
    std::array<QColor, 8> numbers;

    // What a number is drawn in; eight and anything past it share the last.
    const QColor &number(int adjacent) const;
};

// Draws the field: the marks that go inside one cell, and the decoration over
// the board. Holds nothing but the painter, the cell size and the palette, so
// the view keeps the board, the mouse and the animations to itself.
class FieldPainter {
public:
    FieldPainter(QPainter *painter, int cellSize, const FieldPalette &palette);

    // A hidden cell is a raised lid: a lit top-left edge and a shaded
    // bottom-right one, which is all the depth a flat theme wants.
    void lid(const QRect &box, double opacity) const;
    void number(const QRect &box, int adjacent, double opacity) const;
    // A pennant on a short pole, drawn rather than typed so it scales with the
    // cell and never depends on a font having the glyph.
    void flag(const QRect &box, const QColor &color) const;
    void mine(const QRect &box) const;
    void cross(const QRect &box) const;
    // The mine that ended the game, under everything else on its cell.
    void exploded(const QRect &box) const;

    void grid(QPoint origin, int columns, int rows) const;
    void cursor(const QRectF &box, double opacity) const;

private:
    QPainter *m_painter;
    int m_cellSize;
    FieldPalette m_palette;
};
