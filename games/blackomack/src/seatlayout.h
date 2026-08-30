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

// Anchor of the `index`-th mate of `count`, in play order: the seats are
// ordered by where they sit on the arc, starting at the dealer's left — the
// right of the screen — running down that side to the human's tray at the
// bottom and on up the far side to the dealer's right, so the action sweeps
// round the table instead of jumping about it.
QPointF anchor(int count, int index);
// The same seat as a rectangle on a table of `table` size.
QRectF rect(int count, int index, const QSizeF &table, const QSizeF &seat);
// How many mates the sweep reaches before the human's tray, which is also the
// human's index in the table's seat order.
int matesBeforeHuman(int count);

}
