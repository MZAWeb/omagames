#include "fieldview.h"

#include <QPainter>
#include <QtMath>
#include <algorithm>
#include <cmath>
#include <cstdlib>

#include "omanixgame.h"
#include "omarchytheme.h"

namespace {

// A claim sweeps outward from where the trail closed; each cell fades in
// once the sweep reaches it.
constexpr int kSweepMs = 200;
constexpr int kFadeMs = 120;
// A lost trail flashes red and the marker blinks while everything is frozen.
constexpr int kFlashMs = 450;
constexpr int kBlinkMs = 900;
constexpr int kBlinkPeriodMs = 150;
constexpr int kGhosts = 3;
// The trail pulses toward the marker's brightness while a ball is close.
constexpr int kPulseMs = 360;
constexpr int kFrameMs = 16;
constexpr int kMinGridCell = 5;

double progress(qint64 elapsed, int duration) {
    return std::clamp(double(elapsed) / duration, 0.0, 1.0);
}

}  // namespace

FieldView::FieldView(QQuickItem *parent) : QQuickPaintedItem(parent) {
    m_clock.start();
    m_animation.setInterval(kFrameMs);
    connect(&m_animation, &QTimer::timeout, this, &FieldView::animate);
    connect(this, &FieldView::colorsChanged, this, &FieldView::invalidateGround);
}

void FieldView::invalidateGround() {
    m_groundDirty = true;
    update();
}

void FieldView::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) {
    QQuickPaintedItem::geometryChange(newGeometry, oldGeometry);
    if (newGeometry.size() != oldGeometry.size())
        invalidateGround();
}

QObject *FieldView::source() const {
    return m_source;
}

void FieldView::setSource(QObject *source) {
    OmanixGame *game = qobject_cast<OmanixGame *>(source);
    if (m_source == game)
        return;
    if (m_source)
        disconnect(m_source, nullptr, this, nullptr);
    m_source = game;
    if (m_source) {
        connect(m_source, &OmanixGame::frameChanged, this, &FieldView::onFrame);
        connect(m_source, &OmanixGame::cellsClaimed, this, &FieldView::onClaimed);
        connect(m_source, &OmanixGame::trailLost, this, &FieldView::onTrailLost);
    }
    m_history.clear();
    m_lastChasers.clear();
    emit sourceChanged();
    invalidateGround();
}

void FieldView::setTrailThreatened(bool threatened) {
    if (m_trailThreatened == threatened)
        return;
    m_trailThreatened = threatened;
    if (threatened)
        m_animation.start();
    emit trailThreatenedChanged();
    update();
}

void FieldView::setCellSize(int cellSize) {
    if (m_cellSize == cellSize)
        return;
    m_cellSize = cellSize;
    emit cellSizeChanged();
    invalidateGround();
}

const Game *FieldView::engine() const {
    return m_source ? m_source->engine() : nullptr;
}

// The engine ticks sixty times a second but each mover steps every few
// ticks, so most ticks leave the picture exactly as it was: those cost no
// repaint, and with them no re-upload of the whole field.
void FieldView::onFrame() {
    const Game *game = engine();
    if (!game) {
        m_history.clear();
        m_lastChasers.clear();
        update();
        return;
    }
    bool moved = false;

    const std::vector<Ball> &balls = game->balls();
    if (m_history.size() != int(balls.size())) {
        m_history.resize(int(balls.size()));
        moved = true;
    }
    for (int i = 0; i < int(balls.size()); ++i) {
        QVector<QPoint> &trail = m_history[i];
        if (trail.isEmpty() || trail.first() != balls[size_t(i)].pos) {
            trail.prepend(balls[size_t(i)].pos);
            if (trail.size() > kGhosts + 1)
                trail.resize(kGhosts + 1);
            moved = true;
        }
    }

    const std::vector<Chaser> &chasers = game->chasers();
    if (m_lastChasers.size() != int(chasers.size())) {
        m_lastChasers.resize(int(chasers.size()));
        moved = true;
    }
    for (int i = 0; i < int(chasers.size()); ++i) {
        if (m_lastChasers[i] != chasers[size_t(i)].pos) {
            m_lastChasers[i] = chasers[size_t(i)].pos;
            moved = true;
        }
    }

    const Player &player = game->player();
    if (player.pos != m_lastPlayer || player.onTrail != m_lastOnTrail) {
        m_lastPlayer = player.pos;
        m_lastOnTrail = player.onTrail;
        moved = true;
    }
    if (game->field().revision() != m_fieldRevision) {
        m_fieldRevision = game->field().revision();
        moved = true;
    }

    if (moved)
        update();
}

void FieldView::onClaimed(const QVector<int> &cells, int x, int y) {
    const Game *game = engine();
    if (!game)
        return;
    const Field &field = game->field();
    m_sweep.clear();
    int furthest = 1;
    for (int index : cells) {
        const QPoint p = field.point(index);
        furthest = std::max(furthest, std::abs(p.x() - x) + std::abs(p.y() - y));
    }
    for (int index : cells) {
        const QPoint p = field.point(index);
        const int distance = std::abs(p.x() - x) + std::abs(p.y() - y);
        m_sweep.append({index, distance * kSweepMs / furthest});
    }
    m_sweepStart = m_clock.elapsed();
    m_animation.start();
}

void FieldView::onTrailLost(const QVector<int> &cells) {
    m_flash = cells;
    m_flashStart = m_clock.elapsed();
    m_animation.start();
}

void FieldView::animate() {
    const qint64 now = m_clock.elapsed();
    const bool sweeping = !m_sweep.isEmpty() && now - m_sweepStart < kSweepMs + kFadeMs;
    const bool flashing = m_flashStart >= 0 && now - m_flashStart < kBlinkMs;
    if (!sweeping)
        m_sweep.clear();
    if (!flashing) {
        m_flash.clear();
        m_flashStart = -1;
    }
    if (!sweeping && !flashing && !m_trailThreatened)
        m_animation.stop();
    update();
}

QColor FieldView::trailColorNow(qint64 now) const {
    if (!m_trailThreatened)
        return m_trailColor;
    const double phase = 0.5 + 0.5 * std::sin(2 * M_PI * double(now % kPulseMs) / kPulseMs);
    return OmarchyTheme::mix(m_trailColor, m_markerColor, 0.15 + 0.55 * phase);
}

void FieldView::paint(QPainter *painter) {
    const Game *game = engine();
    if (!game) {
        painter->fillRect(boundingRect(), m_openColor);
        return;
    }
    const qint64 now = m_clock.elapsed();
    paintCells(painter, *game, now);
    paintMovers(painter, *game, now);
}

void FieldView::refreshGround(const Field &field, qreal scale) {
    const qreal dpr = scale > 0 ? scale : 1.0;
    const QSize pixels(qMax(1, qCeil(width() * dpr)), qMax(1, qCeil(height() * dpr)));
    if (!m_groundDirty && m_groundRevision == field.groundRevision() && m_ground.size() == pixels)
        return;
    if (m_ground.size() != pixels)
        m_ground = QImage(pixels, QImage::Format_ARGB32_Premultiplied);
    m_ground.setDevicePixelRatio(dpr);
    m_groundRevision = field.groundRevision();
    m_groundDirty = false;

    const int c = m_cellSize;
    QPainter painter(&m_ground);
    painter.scale(dpr, dpr);
    painter.fillRect(QRectF(0, 0, width(), height()), m_openColor);
    painter.setPen(Qt::NoPen);
    for (int y = 0; y < field.height(); ++y) {
        for (int x = 0; x < field.width(); ++x) {
            if (field.at({x, y}) == Cell::Claimed)
                painter.fillRect(x * c, y * c, c, c, m_claimedColor);
        }
    }
    if (c >= kMinGridCell) {
        painter.setPen(QPen(m_gridColor, 1));
        for (int x = 1; x < field.width(); ++x)
            painter.drawLine(QPointF(x * c + 0.5, 0), QPointF(x * c + 0.5, height()));
        for (int y = 1; y < field.height(); ++y)
            painter.drawLine(QPointF(0, y * c + 0.5), QPointF(width(), y * c + 0.5));
    }
}

void FieldView::paintGridEdges(QPainter *painter, QPoint cell) const {
    if (m_cellSize < kMinGridCell)
        return;
    const int c = m_cellSize;
    if (cell.x() >= 1)
        painter->fillRect(cell.x() * c, cell.y() * c, 1, c, m_gridColor);
    if (cell.y() >= 1)
        painter->fillRect(cell.x() * c, cell.y() * c, c, 1, m_gridColor);
}

void FieldView::paintCells(QPainter *painter, const Game &game, qint64 now) {
    const Field &field = game.field();
    const int c = m_cellSize;
    refreshGround(field, painter->combinedTransform().m11());
    painter->drawImage(QPointF(0, 0), m_ground);

    const QColor trail = trailColorNow(now);
    painter->setPen(Qt::NoPen);
    for (int y = 0; y < field.height(); ++y) {
        for (int x = 0; x < field.width(); ++x) {
            if (field.at({x, y}) != Cell::Trail)
                continue;
            painter->fillRect(x * c, y * c, c, c, trail);
            paintGridEdges(painter, {x, y});
        }
    }
    // The ground already shows the claim; the sea fades off it instead of
    // the claim fading in, which is the same blend from the other side.
    for (const SweepCell &sweep : m_sweep) {
        const double t = progress(now - m_sweepStart - sweep.delayMs, kFadeMs);
        if (t >= 1.0)
            continue;
        const QPoint p = field.point(sweep.index);
        QColor sea = m_openColor;
        sea.setAlphaF(sea.alphaF() * (1.0 - t));
        painter->fillRect(p.x() * c, p.y() * c, c, c, sea);
        paintGridEdges(painter, p);
    }
    if (m_flashStart >= 0) {
        const double t = progress(now - m_flashStart, kFlashMs);
        QColor tint = m_flashColor;
        tint.setAlphaF(1.0 - t);
        for (int index : m_flash) {
            const QPoint p = field.point(index);
            painter->fillRect(p.x() * c, p.y() * c, c, c, tint);
            paintGridEdges(painter, p);
        }
    }
}

void FieldView::paintMovers(QPainter *painter, const Game &game, qint64 now) {
    const double c = m_cellSize;
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(Qt::NoPen);

    const std::vector<Ball> &balls = game.balls();
    for (int i = 0; i < int(balls.size()); ++i) {
        if (i < m_history.size()) {
            const QVector<QPoint> &trail = m_history[i];
            for (int g = 1; g < trail.size(); ++g) {
                QColor ghost = m_ballColor;
                ghost.setAlphaF(0.4 / g);
                painter->setBrush(ghost);
                const double inset = c * (0.2 + 0.08 * g);
                painter->drawEllipse(QRectF(trail[g].x() * c + inset, trail[g].y() * c + inset, c - 2 * inset, c - 2 * inset));
            }
        }
        painter->setBrush(m_ballColor);
        const QPoint p = balls[size_t(i)].pos;
        painter->drawEllipse(QRectF(p.x() * c + c * 0.1, p.y() * c + c * 0.1, c * 0.8, c * 0.8));
    }

    painter->setBrush(m_chaserColor);
    for (const Chaser &chaser : game.chasers()) {
        const QPointF centre((chaser.pos.x() + 0.5) * c, (chaser.pos.y() + 0.5) * c);
        const double r = c * 0.55;
        const QPointF diamond[] = {centre + QPointF(0, -r), centre + QPointF(r, 0), centre + QPointF(0, r), centre + QPointF(-r, 0)};
        painter->drawPolygon(diamond, 4);
    }

    paintMarker(painter, game, now);
}

// Solid on the ground; an accent-filled outline while cutting, so exposure
// is visible at a glance.
void FieldView::paintMarker(QPainter *painter, const Game &game, qint64 now) {
    const bool blinking = m_flashStart >= 0 && now - m_flashStart < kBlinkMs;
    if (blinking && ((now - m_flashStart) / kBlinkPeriodMs) % 2 == 1)
        return;
    const double c = m_cellSize;
    const QPoint p = game.player().pos;
    const double grow = c * 0.18;
    const QRectF box(p.x() * c - grow, p.y() * c - grow, c + 2 * grow, c + 2 * grow);
    if (game.player().onTrail) {
        painter->setBrush(m_accentColor);
        painter->setPen(QPen(m_markerColor, std::max(1.0, c * 0.16)));
    } else {
        painter->setBrush(m_markerColor);
        painter->setPen(QPen(m_openColor, std::max(1.0, c * 0.12)));
    }
    painter->drawRoundedRect(box, c * 0.25, c * 0.25);
}
