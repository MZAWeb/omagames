#pragma once

#include <QColor>
#include <QElapsedTimer>
#include <QQuickPaintedItem>
#include <QTimer>
#include <QVariantList>
#include <QVector>

class Game;
class OmatrisGame;

// Paints the well straight from the engine state: the stack, the ghost when
// it is wanted, the falling piece and the two flashes a lock and a line clear
// deserve. Every
// colour arrives from QML so the palette stays the theme's; this class holds
// no rules.
class FieldView : public QQuickPaintedItem {
    Q_OBJECT
    Q_PROPERTY(QObject *source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(int cellSize READ cellSize WRITE setCellSize NOTIFY cellSizeChanged)
    // Whether to outline where the falling piece would land.
    Q_PROPERTY(bool showGhost READ showGhost WRITE setShowGhost NOTIFY showGhostChanged)
    // One colour per tetromino, in the order I, J, L, O, S, T, Z.
    Q_PROPERTY(QVariantList pieceColors READ pieceColors WRITE setPieceColors NOTIFY colorsChanged)
    Q_PROPERTY(QColor emptyColor MEMBER m_emptyColor NOTIFY colorsChanged)
    Q_PROPERTY(QColor gridColor MEMBER m_gridColor NOTIFY colorsChanged)
    Q_PROPERTY(QColor ghostColor MEMBER m_ghostColor NOTIFY colorsChanged)
    Q_PROPERTY(QColor flashColor MEMBER m_flashColor NOTIFY colorsChanged)
    // What a cell's bevel leans toward on its lit and its shaded edges.
    Q_PROPERTY(QColor lightColor MEMBER m_lightColor NOTIFY colorsChanged)
    Q_PROPERTY(QColor shadeColor MEMBER m_shadeColor NOTIFY colorsChanged)

public:
    explicit FieldView(QQuickItem *parent = nullptr);

    QObject *source() const;
    void setSource(QObject *source);
    int cellSize() const { return m_cellSize; }
    void setCellSize(int cellSize);
    bool showGhost() const { return m_showGhost; }
    void setShowGhost(bool show);
    QVariantList pieceColors() const { return m_pieceColors; }
    void setPieceColors(const QVariantList &colors);

    void paint(QPainter *painter) override;

signals:
    void sourceChanged();
    void cellSizeChanged();
    void showGhostChanged();
    void colorsChanged();

private:
    const Game *engine() const;
    void onLocked(const QVector<int> &cells);
    void onCleared(const QVector<int> &rows);
    void animate();
    QColor colorFor(int piece) const;
    void paintCell(QPainter *painter, int column, int row, const QColor &color);
    void paintGrid(QPainter *painter);
    void paintStack(QPainter *painter, const Game &game);
    void paintGhostAndPiece(QPainter *painter, const Game &game);
    void paintFlashes(QPainter *painter, qint64 now);

    OmatrisGame *m_source = nullptr;
    int m_cellSize = 20;
    bool m_showGhost = true;
    QVariantList m_pieceColors;
    QVector<QColor> m_colors;
    QColor m_emptyColor;
    QColor m_gridColor;
    QColor m_ghostColor;
    QColor m_flashColor;
    QColor m_lightColor;
    QColor m_shadeColor;

    QElapsedTimer m_clock;
    QTimer m_animation {this};
    QVector<int> m_lockFlash;
    qint64 m_lockFlashStart = -1;
    QVector<int> m_rowFlash;
    qint64 m_rowFlashStart = -1;
};
