#include "fieldview.h"

#include <QFont>
#include <QPainter>
#include <QPen>
#include <QPolygonF>
#include <algorithm>
#include <cmath>

#include "dropengine.h"
#include "omadropgame.h"

DropFieldView::DropFieldView(QQuickItem *parent) : QQuickPaintedItem(parent) {
    connect(this, &DropFieldView::colorsChanged, this, &QQuickItem::update);
}

QObject *DropFieldView::source() const {
    return m_source;
}

void DropFieldView::setSource(QObject *source) {
    OmadropGame *game = qobject_cast<OmadropGame *>(source);
    if (m_source == game)
        return;
    if (m_source)
        disconnect(m_source, nullptr, this, nullptr);
    m_source = game;
    if (m_source)
        connect(m_source, &OmadropGame::frameChanged, this, &QQuickItem::update);
    emit sourceChanged();
    update();
}

const DropEngine *DropFieldView::engine() const {
    return m_source ? m_source->engine() : nullptr;
}

double DropFieldView::scale() const {
    return std::min(width() / DropEngine::kWidth, height() / DropEngine::kHeight);
}

QPointF DropFieldView::point(QPointF logical) const {
    return {logical.x() / DropEngine::kWidth * width(),
            logical.y() / DropEngine::kHeight * height()};
}

QColor DropFieldView::pegColor(int hits) const {
    switch (hits) {
    case 1: return m_oneColor;
    case 2: return m_twoColor;
    case 3: return m_threeColor;
    case 4: return m_fourColor;
    case 5: return m_fiveColor;
    case 6: return m_sixColor;
    default: return m_sevenColor;
    }
}

void DropFieldView::paint(QPainter *painter) {
    painter->fillRect(boundingRect(), m_fieldColor);
    const DropEngine *game = engine();
    if (!game)
        return;
    painter->setRenderHint(QPainter::Antialiasing);

    QFont scoreFont = painter->font();
    scoreFont.setBold(true);
    scoreFont.setPixelSize(int(scale() * 0.20));
    painter->setFont(scoreFont);
    painter->setPen(m_faintColor);
    painter->drawText(boundingRect(), Qt::AlignCenter, QString::number(game->score()));

    const double danger = DropEngine::kDangerY * height();
    QPen dangerPen(m_borderColor, std::max(1.0, scale() * 0.003));
    dangerPen.setStyle(Qt::DashLine);
    painter->setPen(dangerPen);
    painter->drawLine(QPointF(0, danger), QPointF(width(), danger));

    paintGuide(painter, *game);
    paintPegs(painter, *game);
    paintBall(painter, *game);

    painter->setPen(QPen(m_borderColor, std::max(1.5, scale() * 0.004)));
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(boundingRect().adjusted(1, 1, -1, -1));
}

void DropFieldView::paintGuide(QPainter *painter, const DropEngine &game) {
    const QPointF launcher = point({DropEngine::kLauncherX, DropEngine::kLauncherY});
    const QVector<QPointF> arc = game.guide();
    painter->setPen(Qt::NoPen);
    const double dotSize = std::max(1.5, scale() * 0.0045);
    for (const QPointF &sample : arc) {
        painter->setBrush(m_guideColor);
        painter->drawEllipse(point(sample), dotSize, dotSize);
    }
    if (arc.size() >= 2) {
        const QPointF end = point(arc.last());
        const QPointF direction = end - point(arc[arc.size() - 2]);
        const double size = scale() * 0.014;
        const double angle = std::atan2(direction.y(), direction.x());
        const QPointF left = end - QPointF(std::cos(angle - 0.55), std::sin(angle - 0.55)) * size;
        const QPointF right = end - QPointF(std::cos(angle + 0.55), std::sin(angle + 0.55)) * size;
        painter->setBrush(m_guideColor);
        painter->drawPolygon(QPolygonF({end, left, right}));
    }

    const double radius = scale() * DropEngine::kBallRadius;
    painter->setPen(QPen(m_ballColor, std::max(1.5, scale() * 0.005)));
    painter->setBrush(game.ready() ? m_ballColor : m_fieldColor);
    painter->drawEllipse(launcher, radius, radius);
}

void DropFieldView::paintPegs(QPainter *painter, const DropEngine &game) {
    const double radius = scale() * DropEngine::kPegRadius;
    QFont numberFont = painter->font();
    numberFont.setBold(true);
    numberFont.setPixelSize(int(radius * 0.85));
    painter->setFont(numberFont);

    for (const DropPeg &peg : game.pegs()) {
        const QPointF at = point(peg.position);
        const QRectF box(at.x() - radius, at.y() - radius, radius * 2, radius * 2);
        painter->save();
        painter->translate(at);
        painter->rotate(peg.angle * 180.0 / 3.141592653589793);
        painter->translate(-at);
        painter->setPen(Qt::NoPen);
        painter->setBrush(pegColor(peg.hits));
        if (peg.shape == PegShape::Square)
            painter->drawRoundedRect(box, radius * 0.14, radius * 0.14);
        else
            painter->drawEllipse(box);
        painter->setPen(m_textColor);
        painter->drawText(box, Qt::AlignCenter, QString::number(peg.hits));
        painter->restore();
    }
}

void DropFieldView::paintBall(QPainter *painter, const DropEngine &game) {
    if (!game.ball().active)
        return;
    const double radius = scale() * DropEngine::kBallRadius;
    const QVector<QPointF> &trail = game.trail();
    for (int i = trail.size() - 1; i >= 1; --i) {
        QColor color = m_ballColor;
        color.setAlphaF(0.08 + 0.5 * double(trail.size() - i) / trail.size());
        painter->setBrush(color);
        painter->setPen(Qt::NoPen);
        const double part = radius * (0.45 + 0.55 * double(trail.size() - i) / trail.size());
        painter->drawEllipse(point(trail[i]), part, part);
    }
    painter->setBrush(m_ballColor);
    painter->setPen(Qt::NoPen);
    painter->drawEllipse(point(game.ball().position), radius, radius);
}
