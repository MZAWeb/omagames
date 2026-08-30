#include "seatlayouttests.h"

#include <QtMath>
#include <QtTest>

#include "blackjackrules.h"
#include "seatlayout.h"
#include "table.h"
#include "testhelpers.h"

namespace {

// The oval's inner rectangle for a window: the window loses 24x162 to the
// margins, the header and the action dock, then 2 more each way to the
// table's own inset. 1280x800 is the default window, 1040x650 the smallest
// one that still draws the oval with three or four mates.
QSizeF tableFor(qreal windowWidth, qreal windowHeight) {
    return {windowWidth - 24 - 4, windowHeight - 162 - 4};
}

QString seatMessage(int count, int i, int j, const QRectF &a, const QRectF &b) {
    return QStringLiteral("%1 mates: seat %2 (%3,%4) meets seat %5 (%6,%7)")
        .arg(count).arg(i).arg(a.x()).arg(a.y()).arg(j).arg(b.x()).arg(b.y());
}

// How far round the oval a seat sits, in degrees of the sweep: -90 is the
// dealer, 0 the dealer's left (the right of the screen), 90 the human's
// tray at the bottom and 270 the dealer's right.
qreal sweepAngle(const QPointF &spot, const QSizeF &table) {
    const qreal degrees = qRadiansToDegrees(
        qAtan2(spot.y() - table.height() / 2, spot.x() - table.width() / 2));
    return degrees < -90 ? degrees + 360 : degrees;
}
QString sweepMessage(int count, int seat, qreal angle, qreal previous) {
    return QStringLiteral("%1 mates: seat %2 sits at %3 degrees, back from %4")
        .arg(count).arg(seat).arg(angle).arg(previous);
}

}

// No two mates ever share a patch of felt: an empty 190x132 seat at both
// window sizes, and the ~175 tall seat a dealt hand grows into at the
// smallest window each table size is drawn on (five and six mates reflow to
// the roster below 1264x762, which is why they are checked there).
void SeatLayoutTests::seatSlotsNeverOverlap() {
    struct Case {
        QSizeF table;
        QSizeF seat;
        int maxCount;
    };
    const QList<Case> cases = {
        {tableFor(1280, 800), QSizeF(190, 132), BlackjackRules::kMaxBots},
        {tableFor(1040, 650), QSizeF(190, 132), BlackjackRules::kMaxBots},
        {tableFor(1040, 650), QSizeF(190, 175), 4},
        {tableFor(1264, 762), QSizeF(190, 175), BlackjackRules::kMaxBots},
    };
    for (const Case &c : cases) {
        const QRectF felt(QPointF(0, 0), c.table);
        for (int count = 0; count <= c.maxCount; ++count) {
            for (int i = 0; i < count; ++i) {
                const QRectF a = SeatLayout::rect(count, i, c.table, c.seat);
                QVERIFY(felt.contains(a));
                for (int j = i + 1; j < count; ++j) {
                    const QRectF b = SeatLayout::rect(count, j, c.table, c.seat);
                    QVERIFY2(!a.intersects(b),
                             qPrintable(seatMessage(count, i, j, a, b)));
                }
            }
        }
    }
}

// The slots are listed the way the sweep travels: first the mates on the
// dealer's left, from the top down, then — past the human's tray — the far
// side from the bottom up. The seats still mirror in pairs, with an odd
// extra mate on the far side, where the sweep ends.
void SeatLayoutTests::seatSlotsSweepAroundTheArc() {
    for (int count = 1; count <= BlackjackRules::kMaxBots; ++count) {
        const int before = SeatLayout::matesBeforeHuman(count);
        QCOMPARE(before, count / 2);              // half the table plays ahead of you
        for (int i = 0; i < count; ++i) {
            const QPointF seat = SeatLayout::anchor(count, i);
            QVERIFY(i < before ? seat.x() > 0.5 : seat.x() < 0.5);
            if (i > 0 && i != before) {           // down one side, up the other
                const QPointF previous = SeatLayout::anchor(count, i - 1);
                QVERIFY(i < before ? seat.y() > previous.y() : seat.y() < previous.y());
            }
            const QPointF mirror = SeatLayout::anchor(count, count - 1 - i);
            if (i != count - 1 - i) {
                QCOMPARE(seat.x() + mirror.x(), 1.0);
                QCOMPARE(seat.y(), mirror.y());
            } else {
                QVERIFY(seat.x() < 0.5);          // the odd mate sits out the far side
            }
        }
    }
    QCOMPARE(SeatLayout::matesBeforeHuman(0), 0);
    QVERIFY(SeatLayout::anchor(3, 3).isNull());   // out of range asks for nothing
    QVERIFY(SeatLayout::anchor(0, 0).isNull());
}

// Seat order, play order and the seats' places on the felt are one and the
// same: the bets, the pitch and the turns all walk the array, and walking
// the array walks the oval — from the dealer's left, down that side, through
// the tray at the bottom and up the far side to the dealer's right.
void SeatLayoutTests::playOrderMatchesTheSeatingArc() {
    Table t(7);
    for (const QString &name : {QStringLiteral("Zed"), QStringLiteral("Mona"),
                                QStringLiteral("Bucky"), QStringLiteral("Ivy")})
        QVERIFY(t.addBot(perfect(name)));
    const QVector<int> seatOrder{0, 1, 2, 3, 4};
    QCOMPARE(t.humanSeat(), 2);                   // the tray is halfway round
    t.stackDeck(cards({10, 10, 10, 10, 10, 10, 9, 9, 9, 9, 9, 7}));

    QVector<int> betOrder;
    for (const TableEvent &e : t.placeBets(20))
        if (e.type == TableEvent::BetPlaced)
            betOrder.append(e.seat);
    QCOMPARE(betOrder, seatOrder);

    QVector<int> pitch;
    for (const TableEvent &e : dealOut(t))
        if (e.type == TableEvent::Dealt && e.seat != Table::kDealerSeat)
            pitch.append(e.seat);
    QCOMPARE(pitch, seatOrder + seatOrder);

    QVector<int> turnOrder;
    while (t.phase() == Table::Phase::PlayerTurns) {
        if (turnOrder.isEmpty() || turnOrder.last() != t.currentSeat())
            turnOrder.append(t.currentSeat());
        if (t.waitingForHuman())
            t.act(Table::Action::Stand);
        else
            t.advance();
    }
    QCOMPARE(turnOrder, seatOrder);

    // Every table size sweeps one way round the felt: the angle each seat
    // sits at, the human's tray included, only ever grows along the order.
    const QSizeF felt = tableFor(1280, 800);
    for (int count = 0; count <= BlackjackRules::kMaxBots; ++count) {
        qreal travelled = -90;
        for (int seat = 0; seat <= count; ++seat) {
            const int mate = seat < SeatLayout::matesBeforeHuman(count) ? seat : seat - 1;
            const QPointF spot = seat == SeatLayout::matesBeforeHuman(count)
                ? QPointF(felt.width() / 2, felt.height())          // your tray, bottom centre
                : SeatLayout::rect(count, mate, felt, QSizeF(190, 132)).center();
            const qreal angle = sweepAngle(spot, felt);
            QVERIFY2(angle > travelled, qPrintable(sweepMessage(count, seat, angle, travelled)));
            travelled = angle;
        }
    }
}
