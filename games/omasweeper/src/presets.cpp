#include "presets.h"

namespace Presets {

QString id(Preset preset) {
    return QString::fromLatin1(spec(preset).key);
}

QString label(Preset preset) {
    return QString::fromLatin1(spec(preset).label);
}

bool fromId(const QString &id, Preset *preset) {
    for (const PresetSpec &spec : kAll) {
        if (QLatin1String(spec.key) == id) {
            *preset = spec.id;
            return true;
        }
    }
    return false;
}

}  // namespace Presets
