#pragma once

#include <QStringList>

#include "game.h"

// What one locked piece is told it earned. The numbers are the engine's; this
// is only what rises off the board and in which order.
namespace Bonuses {

// The popups one clear earns: what it is called, then the streaks it kept
// going. Empty when the placement is not worth saying anything about.
QStringList texts(const ClearInfo &clear);

}  // namespace Bonuses
