#pragma once

#include <QPointF>
#include <QRectF>
#include <QSizeF>

// Where the table mates sit on the oval. A seat is anchored by fractions of the
// free space (the table minus the seat), so a larger table only ever spreads
// seats further apart. The anchors are chosen per mate count: any two seats are
// kept apart by a full seat width or by more than a seat height, so a seat can
// never cover the one dealt before it, and the seats next to the dealer stay
// far enough inward to clear the dealer's cards.
namespace SeatLayout {

// Anchor of the `index`-th mate of `count`, in deal order: mates fill the arc
// dealer-outwards, left first, so an odd extra mate sits dealer-left.
QPointF anchor(int count, int index);
// The same seat as a rectangle on a table of `table` size.
QRectF rect(int count, int index, const QSizeF &table, const QSizeF &seat);

}
