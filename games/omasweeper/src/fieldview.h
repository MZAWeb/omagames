#pragma once

#include <QColor>
#include <QElapsedTimer>
#include <QHash>
#include <QQuickPaintedItem>
#include <QTimer>

class Board;
class OmasweeperGame;

// Paints the minefield straight from the engine state: an Expert board is
// 480 cells, far too many for a Repeater. Every color arrives from QML so the
// palette stays the theme's, and this class holds no rules — only the short
// animations a cascade and the cursor deserve, plus the mouse gestures every
// Minesweeper player expects, each forwarded to the bridge unchanged.
class FieldView : public QQuickPaintedItem {
    Q_OBJECT
    Q_PROPERTY(QObject *source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(int cellSize READ cellSize WRITE setCellSize NOTIFY cellSizeChanged)
    Q_PROPERTY(QColor hiddenColor MEMBER m_hiddenColor NOTIFY colorsChanged)
    Q_PROPERTY(QColor hiddenHighlightColor MEMBER m_hiddenHighlightColor NOTIFY colorsChanged)
    Q_PROPERTY(QColor hiddenShadowColor MEMBER m_hiddenShadowColor NOTIFY colorsChanged)
    Q_PROPERTY(QColor revealedColor MEMBER m_revealedColor NOTIFY colorsChanged)
    Q_PROPERTY(QColor gridColor MEMBER m_gridColor NOTIFY colorsChanged)
    Q_PROPERTY(QColor cursorColor MEMBER m_cursorColor NOTIFY colorsChanged)
    Q_PROPERTY(QColor flagColor MEMBER m_flagColor NOTIFY colorsChanged)
    Q_PROPERTY(QColor mineColor MEMBER m_mineColor NOTIFY colorsChanged)
    Q_PROPERTY(QColor explodedColor MEMBER m_explodedColor NOTIFY colorsChanged)
    Q_PROPERTY(QColor wrongFlagColor MEMBER m_wrongFlagColor NOTIFY colorsChanged)
    Q_PROPERTY(QColor number1Color MEMBER m_number1Color NOTIFY colorsChanged)
    Q_PROPERTY(QColor number2Color MEMBER m_number2Color NOTIFY colorsChanged)
    Q_PROPERTY(QColor number3Color MEMBER m_number3Color NOTIFY colorsChanged)
    Q_PROPERTY(QColor number4Color MEMBER m_number4Color NOTIFY colorsChanged)
    Q_PROPERTY(QColor number5Color MEMBER m_number5Color NOTIFY colorsChanged)
    Q_PROPERTY(QColor number6Color MEMBER m_number6Color NOTIFY colorsChanged)
    Q_PROPERTY(QColor number7Color MEMBER m_number7Color NOTIFY colorsChanged)
    Q_PROPERTY(QColor number8Color MEMBER m_number8Color NOTIFY colorsChanged)

public:
    explicit FieldView(QQuickItem *parent = nullptr);

    QObject *source() const;
    void setSource(QObject *source);
    int cellSize() const { return m_cellSize; }
    void setCellSize(int cellSize);

    void paint(QPainter *painter) override;

signals:
    void sourceChanged();
    void cellSizeChanged();
    void colorsChanged();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    const Board *board() const;
    // Top-left of the grid: a whole number of pixels per cell, centred in
    // whatever room the item was given.
    QPoint origin() const;
    QPoint cellAt(const QPointF &pos) const;
    const QColor &numberColor(int adjacent) const;

    void onField();
    void onRipple();
    void animate();
    void syncAnimation();
    bool rippling(qint64 now) const;
    // 0 while the cell is still a lid, 1 once it has finished flipping open.
    double flip(int index, qint64 now) const;

    void paintCell(QPainter *painter, const Board &board, int index, qint64 now);
    void paintLid(QPainter *painter, const QRect &box, double opacity);
    void paintNumber(QPainter *painter, const QRect &box, int adjacent, double opacity);
    void paintFlag(QPainter *painter, const QRect &box, const QColor &color);
    void paintMine(QPainter *painter, const QRect &box);
    void paintCross(QPainter *painter, const QRect &box);
    void paintGrid(QPainter *painter, const Board &board);
    void paintCursor(QPainter *painter, qint64 now);

    OmasweeperGame *m_source = nullptr;
    int m_cellSize = 16;
    QColor m_hiddenColor;
    QColor m_hiddenHighlightColor;
    QColor m_hiddenShadowColor;
    QColor m_revealedColor;
    QColor m_gridColor;
    QColor m_cursorColor;
    QColor m_flagColor;
    QColor m_mineColor;
    QColor m_explodedColor;
    QColor m_wrongFlagColor;
    QColor m_number1Color;
    QColor m_number2Color;
    QColor m_number3Color;
    QColor m_number4Color;
    QColor m_number5Color;
    QColor m_number6Color;
    QColor m_number7Color;
    QColor m_number8Color;

    QElapsedTimer m_clock;
    QTimer m_animation {this};
    // Cell index -> milliseconds after m_rippleStart at which it flips.
    QHash<int, int> m_rippleDelay;
    qint64 m_rippleStart = -1;
    // A chord starts the moment both buttons are down, and swallows the
    // release that would otherwise reveal or flag.
    bool m_chorded = false;
};
