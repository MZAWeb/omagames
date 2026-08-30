#pragma once

#include <QRect>
#include <QVariantMap>

namespace OmaGames {

// Where the window was when it was last closed, kept in QSettings under
// `window/geometry` and `window/maximized`. Every game restores it the same
// way, so the reading and writing live here rather than in each bridge.
namespace WindowGeometry {

// Invalid when nothing has been saved yet, which is not the same as zeroed.
QRect rect();
bool maximized();

// {valid, x, y, width, height, maximized}: what Main.qml restores from.
// Positions can legitimately be negative on monitors left of or above the
// primary, so validity travels separately instead of being encoded as -1.
QVariantMap toVariantMap();

void save(const QRect &rect, bool maximized);

}  // namespace WindowGeometry

}  // namespace OmaGames
