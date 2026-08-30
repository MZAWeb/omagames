#include "windowgeometry.h"

#include <QSettings>

namespace OmaGames {
namespace WindowGeometry {

namespace {

const auto kGeometryKey = QStringLiteral("window/geometry");
const auto kMaximizedKey = QStringLiteral("window/maximized");

}  // namespace

QRect rect() {
    return QSettings().value(kGeometryKey).toRect();
}

bool maximized() {
    return QSettings().value(kMaximizedKey, false).toBool();
}

QVariantMap toVariantMap() {
    const QSettings settings;
    const QRect saved = settings.value(kGeometryKey).toRect();
    return {
        {QStringLiteral("valid"), saved.isValid()},
        {QStringLiteral("x"), saved.x()},
        {QStringLiteral("y"), saved.y()},
        {QStringLiteral("width"), saved.width()},
        {QStringLiteral("height"), saved.height()},
        {QStringLiteral("maximized"), settings.value(kMaximizedKey, false).toBool()},
    };
}

void save(const QRect &saved, bool maximized) {
    QSettings settings;
    settings.setValue(kGeometryKey, saved);
    settings.setValue(kMaximizedKey, maximized);
}

}  // namespace WindowGeometry
}  // namespace OmaGames
