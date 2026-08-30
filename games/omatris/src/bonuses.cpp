#include "bonuses.h"

#include <QCoreApplication>

namespace Bonuses {
namespace {

// A namespace cannot carry Q_OBJECT's tr(), so the words below name their
// translation context themselves.
QString tr(const char *text) {
    return QCoreApplication::translate("Bonuses", text);
}

// What a clear is called, or nothing when a plain one to three lines is all
// it was.
QString clearName(const ClearInfo &clear) {
    const QString lineNames[5] = {{}, tr("Single"), tr("Double"), tr("Triple"), tr("Tetris")};
    if (clear.spin == Spin::None)
        return clear.lines == 4 ? lineNames[4] : QString();
    const QString spin = clear.spin == Spin::Mini ? tr("T-Spin Mini") : tr("T-Spin");
    return clear.lines == 0 ? spin : spin + QLatin1Char(' ') + lineNames[clear.lines];
}

}  // namespace

QStringList texts(const ClearInfo &clear) {
    QStringList popups;
    const QString name = clearName(clear);
    if (!name.isEmpty())
        popups << name;
    if (clear.backToBack)
        popups << tr("Back-to-Back");
    if (clear.combo >= 1)
        popups << tr("Combo x%1").arg(clear.combo);
    return popups;
}

}  // namespace Bonuses
