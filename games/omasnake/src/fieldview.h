#pragma once

#include <QColor>
#include <QElapsedTimer>
#include <QQuickPaintedItem>
#include <QTimer>

class Game;
class OmasnakeGame;

// Paints the grid, the snake, the dot and the bonus straight from the engine
// state. Every color arrives from QML so the palette stays the theme's; this
// class holds no rules, only the small animations a pulsing dot, a shrinking
// bonus ring and a crash deserve.
class FieldView : public QQuickPaintedItem {
    Q_OBJECT
    Q_PROPERTY(QObject *source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(int cellSize READ cellSize WRITE setCellSize NOTIFY cellSizeChanged)
    Q_PROPERTY(QColor fieldColor MEMBER m_fieldColor NOTIFY colorsChanged)
    Q_PROPERTY(QColor gridColor MEMBER m_gridColor NOTIFY colorsChanged)
    Q_PROPERTY(QColor snakeColor MEMBER m_snakeColor NOTIFY colorsChanged)
    Q_PROPERTY(QColor headColor MEMBER m_headColor NOTIFY colorsChanged)
    Q_PROPERTY(QColor eyeColor MEMBER m_eyeColor NOTIFY colorsChanged)
    Q_PROPERTY(QColor foodColor MEMBER m_foodColor NOTIFY colorsChanged)
    Q_PROPERTY(QColor bonusColor MEMBER m_bonusColor NOTIFY colorsChanged)
    Q_PROPERTY(QColor ringColor MEMBER m_ringColor NOTIFY colorsChanged)
    Q_PROPERTY(QColor crashColor MEMBER m_crashColor NOTIFY colorsChanged)

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
    const Game *engine() const;
    void onCrashed();
    void animate();
    void paintGrid(QPainter *painter, int width, int height) const;
    void paintDots(QPainter *painter, const Game &game, qint64 now);
    void paintSnake(QPainter *painter, const Game &game, qint64 now);
    void paintHead(QPainter *painter, const Game &game, const QColor &body);
    QPointF centre(QPoint cell) const;

    OmasnakeGame *m_source = nullptr;
    int m_cellSize = 16;
    QColor m_fieldColor;
    QColor m_gridColor;
    QColor m_snakeColor;
    QColor m_headColor;
    QColor m_eyeColor;
    QColor m_foodColor;
    QColor m_bonusColor;
    QColor m_ringColor;
    QColor m_crashColor;

    QElapsedTimer m_clock;
    QTimer m_animation {this};
    qint64 m_crashStart = -1;
};
