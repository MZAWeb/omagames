#pragma once

#include <QColor>
#include <QElapsedTimer>
#include <QImage>
#include <QPoint>
#include <QQuickPaintedItem>
#include <QTimer>
#include <QVector>

class Field;
class Game;
class OmanixGame;

// Paints the whole grid, the balls, the chasers and the marker straight from
// the engine state: 2,560 cells are far too many for a Repeater. The claimed
// ground and the grid only change when a claim lands, so they are kept in an
// image and blitted; a frame draws the trail, the animations and the movers
// over it. Every color arrives from QML so the palette stays the theme's;
// this class holds no rules, only the short animations a claim, a lost life
// and a moving ball deserve.
class FieldView : public QQuickPaintedItem {
    Q_OBJECT
    Q_PROPERTY(QObject *source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(int cellSize READ cellSize WRITE setCellSize NOTIFY cellSizeChanged)
    Q_PROPERTY(QColor openColor MEMBER m_openColor NOTIFY colorsChanged)
    Q_PROPERTY(QColor claimedColor MEMBER m_claimedColor NOTIFY colorsChanged)
    Q_PROPERTY(QColor trailColor MEMBER m_trailColor NOTIFY colorsChanged)
    Q_PROPERTY(QColor ballColor MEMBER m_ballColor NOTIFY colorsChanged)
    Q_PROPERTY(QColor chaserColor MEMBER m_chaserColor NOTIFY colorsChanged)
    Q_PROPERTY(QColor markerColor MEMBER m_markerColor NOTIFY colorsChanged)
    Q_PROPERTY(QColor gridColor MEMBER m_gridColor NOTIFY colorsChanged)
    Q_PROPERTY(QColor flashColor MEMBER m_flashColor NOTIFY colorsChanged)
    Q_PROPERTY(QColor accentColor MEMBER m_accentColor NOTIFY colorsChanged)
    Q_PROPERTY(bool trailThreatened READ trailThreatened WRITE setTrailThreatened NOTIFY trailThreatenedChanged)

public:
    explicit FieldView(QQuickItem *parent = nullptr);

    QObject *source() const;
    void setSource(QObject *source);
    int cellSize() const { return m_cellSize; }
    void setCellSize(int cellSize);
    bool trailThreatened() const { return m_trailThreatened; }
    void setTrailThreatened(bool threatened);

    void paint(QPainter *painter) override;

protected:
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;

signals:
    void sourceChanged();
    void cellSizeChanged();
    void colorsChanged();
    void trailThreatenedChanged();

private:
    struct SweepCell {
        int index;
        int delayMs;
    };

    const Game *engine() const;
    void invalidateGround();
    // Repaints the cached ground when it no longer matches the field, the
    // colors or the size it is drawn at. `scale` is item pixels to device
    // pixels, so the image stays sharp on a scaled display.
    void refreshGround(const Field &field, qreal scale);
    void onFrame();
    void onClaimed(const QVector<int> &cells, int x, int y);
    void onTrailLost(const QVector<int> &cells);
    void animate();
    // The grid lives in the cached ground, so a cell painted over it has to
    // put its own two grid edges back.
    void paintGridEdges(QPainter *painter, QPoint cell) const;
    void paintCells(QPainter *painter, const Game &game, qint64 now);
    void paintMovers(QPainter *painter, const Game &game, qint64 now);
    void paintMarker(QPainter *painter, const Game &game, qint64 now);
    QColor trailColorNow(qint64 now) const;

    OmanixGame *m_source = nullptr;
    int m_cellSize = 8;
    QColor m_openColor;
    QColor m_claimedColor;
    QColor m_trailColor;
    QColor m_ballColor;
    QColor m_chaserColor;
    QColor m_markerColor;
    QColor m_gridColor;
    QColor m_flashColor;
    QColor m_accentColor;
    bool m_trailThreatened = false;

    QElapsedTimer m_clock;
    QTimer m_animation {this};
    QVector<SweepCell> m_sweep;
    qint64 m_sweepStart = 0;
    QVector<int> m_flash;
    qint64 m_flashStart = -1;
    // The claimed ground and the grid, painted once per claim.
    QImage m_ground;
    int m_groundRevision = -1;
    bool m_groundDirty = true;
    // Last frame's field and movers, so a tick that moved nothing is free.
    int m_fieldRevision = -1;
    QPoint m_lastPlayer {-1, -1};
    bool m_lastOnTrail = false;
    QVector<QPoint> m_lastChasers;
    // Each ball's last few cells, newest first, for its motion trail.
    QVector<QVector<QPoint>> m_history;
};
