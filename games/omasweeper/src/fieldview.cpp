#include "fieldview.h"

#include <QMouseEvent>
#include <QPainter>
#include <algorithm>
#include <cmath>

#include "omasweepergame.h"
#include "omarchytheme.h"

namespace {

// A cascade lights up from the cell that was opened: the outermost ring
// starts kRippleSpreadMs after the first one and every cell flips open over
// kCellFlipMs, so the whole ripple lasts about a quarter of a second.
constexpr int kRippleSpreadMs = 140;
constexpr int kCellFlipMs = 110;
// The cursor breathes rather than blinks, so it never competes with the board.
constexpr int kCursorPulseMs = 1600;
constexpr int kFrameMs = 16;

double progress(qint64 elapsed, int duration) {
    return std::clamp(double(elapsed) / duration, 0.0, 1.0);
}

}  // namespace

FieldView::FieldView(QQuickItem *parent) : QQuickPaintedItem(parent) {
    m_clock.start();
    m_animation.setInterval(kFrameMs);
    setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton | Qt::MiddleButton);
    connect(&m_animation, &QTimer::timeout, this, &FieldView::animate);
    connect(this, &FieldView::colorsChanged, this, &QQuickItem::update);
}

QObject *FieldView::source() const {
    return m_source;
}

void FieldView::setSource(QObject *source) {
    OmasweeperGame *game = qobject_cast<OmasweeperGame *>(source);
    if (m_source == game)
        return;
    if (m_source)
        disconnect(m_source, nullptr, this, nullptr);
    m_source = game;
    if (m_source) {
        connect(m_source, &OmasweeperGame::fieldChanged, this, &FieldView::onField);
        connect(m_source, &OmasweeperGame::revealRippled, this, &FieldView::onRipple);
    }
    m_rippleDelay.clear();
    m_rippleStart = -1;
    emit sourceChanged();
    syncAnimation();
    update();
}

void FieldView::setCellSize(int cellSize) {
    if (m_cellSize == cellSize)
        return;
    m_cellSize = cellSize;
    emit cellSizeChanged();
    update();
}

const Board *FieldView::board() const {
    return m_source ? m_source->board() : nullptr;
}

QPoint FieldView::origin() const {
    const Board *b = board();
    if (!b)
        return {};
    return {(int(width()) - m_cellSize * b->width()) / 2, (int(height()) - m_cellSize * b->height()) / 2};
}

QPoint FieldView::cellAt(const QPointF &pos) const {
    const Board *b = board();
    if (!b || m_cellSize <= 0)
        return {-1, -1};
    const QPoint o = origin();
    const QPoint cell(int(std::floor((pos.x() - o.x()) / m_cellSize)),
                      int(std::floor((pos.y() - o.y()) / m_cellSize)));
    return b->contains(cell) ? cell : QPoint(-1, -1);
}

FieldPalette FieldView::palette() const {
    return {m_hiddenColor,
            m_hiddenHighlightColor,
            m_hiddenShadowColor,
            m_revealedColor,
            m_gridColor,
            m_cursorColor,
            m_flagColor,
            m_mineColor,
            m_explodedColor,
            m_wrongFlagColor,
            {m_number1Color, m_number2Color, m_number3Color, m_number4Color, m_number5Color, m_number6Color,
             m_number7Color, m_number8Color}};
}

void FieldView::onField() {
    syncAnimation();
    update();
}

void FieldView::onRipple() {
    const Ripple &ripple = m_source->lastReveal();
    m_rippleDelay.clear();
    m_rippleDelay.reserve(ripple.cells.size());
    const int spread = std::max(1, ripple.maxDistance);
    for (int i = 0; i < ripple.cells.size(); ++i)
        m_rippleDelay.insert(ripple.cells.at(i), ripple.distances.at(i) * kRippleSpreadMs / spread);
    m_rippleStart = m_clock.elapsed();
    m_animation.start();
}

bool FieldView::rippling(qint64 now) const {
    return m_rippleStart >= 0 && now - m_rippleStart < kRippleSpreadMs + kCellFlipMs;
}

double FieldView::flip(int index, qint64 now) const {
    const auto delay = m_rippleDelay.constFind(index);
    if (delay == m_rippleDelay.constEnd())
        return 1.0;
    return progress(now - m_rippleStart - *delay, kCellFlipMs);
}

void FieldView::animate() {
    const qint64 now = m_clock.elapsed();
    if (!rippling(now)) {
        m_rippleDelay.clear();
        m_rippleStart = -1;
    }
    syncAnimation();
    update();
}

// The cursor pulses for as long as the board can still be played, so the
// timer only runs while something on screen is actually moving.
void FieldView::syncAnimation() {
    const Board *b = board();
    const bool live = b && (b->status() == Status::Ready || b->status() == Status::Playing);
    if (live || rippling(m_clock.elapsed()))
        m_animation.start();
    else
        m_animation.stop();
}

void FieldView::paint(QPainter *painter) {
    const Board *b = board();
    if (!b || m_cellSize <= 0)
        return;
    const qint64 now = m_clock.elapsed();
    const QPoint o = origin();
    const FieldPainter cells(painter, m_cellSize, palette());
    painter->fillRect(QRect(o, QSize(m_cellSize * b->width(), m_cellSize * b->height())), m_revealedColor);
    painter->setRenderHint(QPainter::Antialiasing);
    for (int index = 0; index < b->cellCount(); ++index)
        paintCell(cells, *b, index, now);
    cells.grid(o, b->width(), b->height());
    paintCursor(cells, now);
}

void FieldView::paintCell(const FieldPainter &cells, const Board &board, int index, qint64 now) {
    const QPoint p = board.point(index);
    const QPoint o = origin();
    const QRect box(o.x() + p.x() * m_cellSize, o.y() + p.y() * m_cellSize, m_cellSize, m_cellSize);
    const Cell &cell = board.cell(index);
    const bool lost = board.status() == Status::Lost;
    const bool won = board.status() == Status::Won;

    if (lost && index == m_source->explodedCell())
        cells.exploded(box);

    if (cell.state == CellState::Revealed) {
        const double t = flip(index, now);
        cells.number(box, cell.adjacent, t);
        if (t < 1.0)
            cells.lid(box, 1.0 - t);
        return;
    }

    // A mine the player never found is turned over at the end; on a win the
    // ones still hidden are flagged for them.
    const bool showMine = lost && cell.mine && cell.state != CellState::Flagged;
    const bool showFlag = cell.state == CellState::Flagged || (won && cell.mine);
    if (!showMine || cell.state == CellState::Flagged)
        cells.lid(box, 1.0);
    if (showFlag)
        cells.flag(box, lost && !cell.mine ? m_wrongFlagColor : m_flagColor);
    if (lost && cell.state == CellState::Flagged && !cell.mine)
        cells.cross(box);
    if (showMine)
        cells.mine(box);
}

void FieldView::paintCursor(const FieldPainter &cells, qint64 now) {
    const Board *b = board();
    if (!b || b->status() == Status::Won || b->status() == Status::Lost)
        return;
    const QPoint o = origin();
    const QRectF box(o.x() + m_source->cursorX() * m_cellSize + 1.0, o.y() + m_source->cursorY() * m_cellSize + 1.0,
                     m_cellSize - 2.0, m_cellSize - 2.0);
    const double phase = 0.5 + 0.5 * std::sin(2 * M_PI * double(now % kCursorPulseMs) / kCursorPulseMs);
    cells.cursor(box, 0.55 + 0.45 * phase);
}

// Left reveals, right flags, middle chords, and both together chord as well.
// Both are decided on the release so a chord can swallow the click that
// started it; every gesture also plants the keyboard cursor where it landed.
void FieldView::mousePressEvent(QMouseEvent *event) {
    const QPoint cell = cellAt(event->position());
    if (cell.x() < 0 || !m_source) {
        event->ignore();
        return;
    }
    if ((event->buttons() & (Qt::LeftButton | Qt::RightButton)) == (Qt::LeftButton | Qt::RightButton)) {
        m_chorded = true;
        m_source->chord(cell.x(), cell.y());
    }
    event->accept();
}

void FieldView::mouseReleaseEvent(QMouseEvent *event) {
    const QPoint cell = cellAt(event->position());
    if (cell.x() >= 0 && m_source && !m_chorded) {
        if (event->button() == Qt::LeftButton)
            m_source->reveal(cell.x(), cell.y());
        else if (event->button() == Qt::RightButton)
            m_source->toggleFlag(cell.x(), cell.y());
        else if (event->button() == Qt::MiddleButton)
            m_source->chord(cell.x(), cell.y());
    }
    if (event->buttons() == Qt::NoButton)
        m_chorded = false;
    event->accept();
}
