#pragma once

#include <QColor>
#include <QElapsedTimer>
#include <QQuickPaintedItem>
#include <QTimer>
#include <QVector>

class Game;
class OmanixGame;

// Paints the whole grid, the balls, the chasers and the marker straight from
// the engine state: 2,560 cells are far too many for a Repeater. Every color
// arrives from QML so the palette stays the theme's; this class holds no
// rules, only the short animations a claim, a lost life and a moving ball
// deserve.
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

private:
    struct SweepCell {
        int index;
        int delayMs;
    };

    const Game *engine() const;
    void onFrame();
    void onClaimed(const QVector<int> &cells, int x, int y);
    void onTrailLost(const QVector<int> &cells);
    void animate();
    void paintCells(QPainter *painter, const Game &game, qint64 now);
    void paintMovers(QPainter *painter, const Game &game, qint64 now);

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

    QElapsedTimer m_clock;
    QTimer m_animation {this};
    QVector<SweepCell> m_sweep;
    qint64 m_sweepStart = 0;
    QVector<int> m_flash;
    qint64 m_flashStart = -1;
    // Each ball's last few cells, newest first, for its motion trail.
    QVector<QVector<QPoint>> m_history;
};
