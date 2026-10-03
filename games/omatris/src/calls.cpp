#include "calls.h"

namespace Calls {

namespace {

// In Call order, Tick excepted.
const char *const kTokens[] = {"L", "R", "CW", "CCW", "SD+", "SD-", "HD", "H"};
constexpr int kTokenCount = int(sizeof kTokens / sizeof kTokens[0]);

}  // namespace

QString token(Call call) {
    Q_ASSERT(call != Call::Tick);
    return QString::fromLatin1(kTokens[int(call)]);
}

std::optional<Call> fromToken(const QString &token) {
    for (int i = 0; i < kTokenCount; ++i) {
        if (token == QLatin1String(kTokens[i]))
            return Call(i);
    }
    return std::nullopt;
}

std::vector<Event> apply(Game &game, Call call) {
    switch (call) {
    case Call::Left:
        game.moveLeft();
        break;
    case Call::Right:
        game.moveRight();
        break;
    case Call::RotateCW:
        game.rotate(1);
        break;
    case Call::RotateCCW:
        game.rotate(-1);
        break;
    case Call::SoftDropOn:
        game.setSoftDrop(true);
        break;
    case Call::SoftDropOff:
        game.setSoftDrop(false);
        break;
    case Call::HardDrop:
        return game.hardDrop();
    case Call::Hold:
        return game.hold();
    case Call::Tick:
        return game.tick();
    }
    return {};
}

}  // namespace Calls
