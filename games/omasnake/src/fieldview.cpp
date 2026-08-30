#include "fieldview.h"

#include <QPainter>
#include <QPainterPath>
#include <QPolygonF>
#include <algorithm>
#include <cmath>
#include <vector>

#include "omarchytheme.h"
#include "omasnakegame.h"

namespace {

// The dot breathes, the crash flashes, and the grid only appears once a cell
// is big enough for the lines not to be the loudest thing on screen.
constexpr int kFrameMs = 16;
constexpr int kPulseMs = 900;
constexpr int kCrashMs = 700;
constexpr int kCrashBlinkMs = 140;
constexpr int kMinGridCell = 9;

// Fractions of a cell.
constexpr double kBodyWidth = 0.78;
constexpr double kHeadSize = 0.9;
constexpr double kEyeRadius = 0.1;
constexpr double kFoodRadius = 0.28;
constexpr double kRingRadius = 0.44;

double progress(qint64 elapsed, int duration) {
    return std::clamp(double(elapsed) / duration, 0.0, 1.0);
}

double wave(qint64 now, int period) {
    return 0.5 + 0.5 * std::sin(2 * M_PI * double(now % period) / period);
}

}  // namespace

FieldView::FieldView(QQuickItem *parent) : QQuickPaintedItem(parent) {
    m_clock.start();
    m_animation.setInterval(kFrameMs);
    connect(&m_animation, &QTimer::timeout, this, &FieldView::animate);
    connect(this, &FieldView::colorsChanged, this, &QQuickItem::update);
}

QObject *FieldView::source() const {
    return m_source;
}

void FieldView::setSource(QObject *source) {
    OmasnakeGame *game = qobject_cast<OmasnakeGame *>(source);
    if (m_source == game)
        return;
    if (m_source)
        disconnect(m_source, nullptr, this, nullptr);
    m_source = game;
    if (m_source) {
        connect(m_source, &OmasnakeGame::frameChanged, this, &QQuickItem::update);
        connect(m_source, &OmasnakeGame::crashed, this, &FieldView::onCrashed);
    }
    m_crashStart = -1;
    emit sourceChanged();
    update();
}

void FieldView::setCellSize(int cellSize) {
    if (m_cellSize == cellSize)
        return;
    m_cellSize = cellSize;
    emit cellSizeChanged();
    update();
}

const Game *FieldView::engine() const {
    return m_source ? m_source->engine() : nullptr;
}

void FieldView::onCrashed() {
    m_crashStart = m_clock.elapsed();
    m_animation.start();
}

// The engine's own frames stop the moment a run ends, so the crash flash
// needs a clock of its own.
void FieldView::animate() {
    if (m_crashStart >= 0 && m_clock.elapsed() - m_crashStart >= kCrashMs) {
        m_crashStart = -1;
        m_animation.stop();
    }
    update();
}

QPointF FieldView::centre(QPoint cell) const {
    return {(cell.x() + 0.5) * m_cellSize, (cell.y() + 0.5) * m_cellSize};
}

void FieldView::paint(QPainter *painter) {
    painter->fillRect(boundingRect(), m_fieldColor);
    const Game *game = engine();
    if (!game)
        return;
    painter->setRenderHint(QPainter::Antialiasing);
    const qint64 now = m_clock.elapsed();
    paintGrid(painter, Game::kWidth, Game::kHeight);
    paintDots(painter, *game, now);
    paintSnake(painter, *game, now);
}

void FieldView::paintGrid(QPainter *painter, int width, int height) const {
    if (m_cellSize < kMinGridCell)
        return;
    painter->setPen(QPen(m_gridColor, 1));
    for (int x = 1; x < width; ++x)
        painter->drawLine(QPointF(x * m_cellSize + 0.5, 0), QPointF(x * m_cellSize + 0.5, this->height()));
    for (int y = 1; y < height; ++y)
        painter->drawLine(QPointF(0, y * m_cellSize + 0.5), QPointF(this->width(), y * m_cellSize + 0.5));
}

void FieldView::paintDots(QPainter *painter, const Game &game, qint64 now) {
    const double c = m_cellSize;
    painter->setPen(Qt::NoPen);

    // The food breathes so it is never mistaken for a piece of the snake.
    const double breath = 1.0 + 0.12 * wave(now, kPulseMs);
    const double r = c * kFoodRadius * breath;
    painter->setBrush(m_foodColor);
    painter->drawEllipse(centre(game.food()), r, r);

    if (!game.hasBonus())
        return;
    const QPointF at = centre(game.bonus());
    painter->setBrush(m_bonusColor);
    painter->drawEllipse(at, c * kFoodRadius, c * kFoodRadius);

    // The ring is the clock: a full circle when it lands, nothing when it goes.
    const double ring = c * kRingRadius;
    QPen pen(m_ringColor, std::max(1.5, c * 0.11));
    pen.setCapStyle(Qt::FlatCap);
    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);
    const int span = int(std::lround(360 * 16 * game.bonusRemaining()));
    painter->drawArc(QRectF(at.x() - ring, at.y() - ring, 2 * ring, 2 * ring), 90 * 16, -span);
}

void FieldView::paintSnake(QPainter *painter, const Game &game, qint64 now) {
    QColor body = m_snakeColor;
    if (m_crashStart >= 0) {
        // Blink toward the crash color, fading out as the flash runs down.
        const qint64 elapsed = now - m_crashStart;
        const double strength = 1.0 - progress(elapsed, kCrashMs);
        const bool on = (elapsed / kCrashBlinkMs) % 2 == 0;
        body = OmarchyTheme::mix(body, m_crashColor, on ? strength : 0.0);
    }

    const double c = m_cellSize;
    QPen pen(body, c * kBodyWidth);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    painter->setBrush(Qt::NoBrush);

    // One run per unbroken stretch of the body: in Wrap mode the snake is cut
    // wherever it steps over an edge, and each piece is drawn on its own.
    std::vector<QPolygonF> runs;
    QPoint previous;
    for (const QPoint &cell : game.snake().body()) {
        const bool joins = !runs.empty() && (std::abs(cell.x() - previous.x()) + std::abs(cell.y() - previous.y())) == 1;
        if (!joins)
            runs.emplace_back();
        runs.back().append(centre(cell));
        previous = cell;
    }
    for (const QPolygonF &run : runs) {
        if (run.size() == 1) {
            painter->setPen(Qt::NoPen);
            painter->setBrush(body);
            painter->drawEllipse(run.first(), c * kBodyWidth / 2, c * kBodyWidth / 2);
            painter->setBrush(Qt::NoBrush);
            continue;
        }
        painter->setPen(pen);
        painter->drawPolyline(run);
    }

    paintHead(painter, game, body);
}

void FieldView::paintHead(QPainter *painter, const Game &game, const QColor &body) {
    const double c = m_cellSize;
    const QPointF at = centre(game.snake().head());
    const double half = c * kHeadSize / 2;
    painter->setPen(Qt::NoPen);
    painter->setBrush(OmarchyTheme::mix(body, m_headColor, 0.75));
    painter->drawRoundedRect(QRectF(at.x() - half, at.y() - half, 2 * half, 2 * half), c * 0.3, c * 0.3);

    // Two eyes looking the way the snake is going.
    const QPoint step = delta(game.snake().heading());
    const QPointF ahead(step.x(), step.y());
    const QPointF side(-ahead.y(), ahead.x());
    const QPointF socket = at + ahead * (c * 0.16);
    const double r = c * kEyeRadius;
    painter->setBrush(m_eyeColor);
    painter->drawEllipse(socket + side * (c * 0.19), r, r);
    painter->drawEllipse(socket - side * (c * 0.19), r, r);
}
