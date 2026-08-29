#include "seatlayout.h"

namespace {
struct Anchor {
    qreal x;
    qreal y;
};

// Up to four mates share the upper arc in two columns per side, far enough
// apart horizontally that their height never matters. Five and six add a third
// row below, which is why those two tables want a taller window: rows in the
// same column are spaced by more than a dealt seat's grown height.
constexpr Anchor kOne[] = {{0.26, 0.00}};
constexpr Anchor kTwo[] = {{0.22, 0.00}, {0.78, 0.00}};
constexpr Anchor kThree[] = {{0.26, 0.00}, {0.74, 0.00}, {0.00, 0.50}};
constexpr Anchor kFour[] = {{0.26, 0.00}, {0.74, 0.00}, {0.00, 0.50}, {1.00, 0.50}};
constexpr Anchor kFive[] = {{0.30, 0.00}, {0.70, 0.00}, {0.04, 0.28}, {0.96, 0.28}, {0.04, 0.72}};
constexpr Anchor kSix[] = {{0.30, 0.00}, {0.70, 0.00}, {0.04, 0.28}, {0.96, 0.28},
                           {0.04, 0.72}, {0.96, 0.72}};

const Anchor *anchorsFor(int count) {
    switch (count) {
    case 1: return kOne;
    case 2: return kTwo;
    case 3: return kThree;
    case 4: return kFour;
    case 5: return kFive;
    case 6: return kSix;
    }
    return nullptr;
}
}

QPointF SeatLayout::anchor(int count, int index) {
    const Anchor *row = anchorsFor(count);
    if (!row || index < 0 || index >= count)
        return {};
    return {row[index].x, row[index].y};
}

QRectF SeatLayout::rect(int count, int index, const QSizeF &table, const QSizeF &seat) {
    const QPointF a = anchor(count, index);
    return {a.x() * qMax(qreal(0), table.width() - seat.width()),
            a.y() * qMax(qreal(0), table.height() - seat.height()),
            seat.width(), seat.height()};
}
