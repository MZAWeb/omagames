#include "omarchytheme.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

namespace {
QString defaultColorsPath() {
    return QDir::homePath() + QStringLiteral("/.local/state/omarchy/current/theme/colors.toml");
}

QString unquote(QString value) {
    if (value.size() >= 2
            && ((value.front() == QLatin1Char('"') && value.back() == QLatin1Char('"'))
                || (value.front() == QLatin1Char('\'') && value.back() == QLatin1Char('\''))))
        value = value.mid(1, value.size() - 2);
    return value;
}
}

OmarchyTheme::OmarchyTheme(QObject *parent)
    : QObject(parent), m_colorsPath(defaultColorsPath()) {
    reload();
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, &OmarchyTheme::reload);
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, &OmarchyTheme::reload);
}

QMap<QString, QColor> OmarchyTheme::defaults() const {
    QMap<QString, QColor> map;
    if (m_darkMode) {
        map[QStringLiteral("background")] = QColor("#101010");
        map[QStringLiteral("dark_background")] = QColor("#0a0a0a");
        map[QStringLiteral("darker_background")] = QColor("#050505");
        map[QStringLiteral("lighter_background")] = QColor("#1c1c1c");
        map[QStringLiteral("foreground")] = QColor("#eeeeee");
        map[QStringLiteral("dark_foreground")] = QColor("#8a8a8a");
        map[QStringLiteral("light_foreground")] = QColor("#f4f4f4");
        map[QStringLiteral("bright_foreground")] = QColor("#ffffff");
        map[QStringLiteral("accent")] = QColor("#5584aa");
        map[QStringLiteral("selection")] = QColor("#186a9a");
        map[QStringLiteral("muted")] = QColor("#555555");
    } else {
        map[QStringLiteral("background")] = QColor("#ffffff");
        map[QStringLiteral("dark_background")] = QColor("#f2f2f2");
        map[QStringLiteral("darker_background")] = QColor("#e6e6e6");
        map[QStringLiteral("lighter_background")] = QColor("#fafafa");
        map[QStringLiteral("foreground")] = QColor("#222324");
        map[QStringLiteral("dark_foreground")] = QColor("#6b6b6b");
        map[QStringLiteral("light_foreground")] = QColor("#111111");
        map[QStringLiteral("bright_foreground")] = QColor("#000000");
        map[QStringLiteral("accent")] = QColor("#2077b2");
        map[QStringLiteral("selection")] = QColor("#2077b2");
        map[QStringLiteral("muted")] = QColor("#9a9a9a");
    }
    map[QStringLiteral("red")] = QColor("#e06c75");
    map[QStringLiteral("yellow")] = QColor("#e5c07b");
    map[QStringLiteral("orange")] = QColor("#d19a66");
    map[QStringLiteral("green")] = QColor("#98c379");
    map[QStringLiteral("cyan")] = QColor("#56b6c2");
    map[QStringLiteral("blue")] = QColor("#61afef");
    map[QStringLiteral("magenta")] = QColor("#c678dd");
    return map;
}

QColor OmarchyTheme::color(const QString &key) const {
    const auto it = m_colors.constFind(key);
    if (it != m_colors.constEnd())
        return *it;
    return defaults().value(key, foreground());
}

QColor OmarchyTheme::mix(const QColor &base, const QColor &tint, qreal amount) {
    amount = qBound<qreal>(0.0, amount, 1.0);
    return QColor::fromRgbF(
        base.redF() + (tint.redF() - base.redF()) * amount,
        base.greenF() + (tint.greenF() - base.greenF()) * amount,
        base.blueF() + (tint.blueF() - base.blueF()) * amount, 1.0);
}

QColor OmarchyTheme::alpha(const QColor &color, qreal alpha) {
    QColor out = color;
    out.setAlphaF(qBound<qreal>(0.0, alpha, 1.0));
    return out;
}

void OmarchyTheme::setColorsPath(const QString &path) {
    m_colorsPath = path;
    reload();
}

void OmarchyTheme::setDarkMode(bool darkMode) {
    if (m_darkMode == darkMode)
        return;
    m_darkMode = darkMode;
    emit darkModeChanged();
    emit colorsChanged();
}

void OmarchyTheme::setTextScale(qreal textScale) {
    if (qFuzzyCompare(m_textScale, textScale))
        return;
    m_textScale = textScale;
    emit textScaleChanged();
}

void OmarchyTheme::reload() {
    QMap<QString, QColor> colors;
    QString mode;

    QFile file(m_colorsPath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
            const QString line = in.readLine().trimmed();
            if (line.isEmpty() || line.startsWith(QLatin1Char('#')))
                continue;
            const int equals = line.indexOf(QLatin1Char('='));
            if (equals < 0)
                continue;
            const QString key = line.left(equals).trimmed();
            const QString value = unquote(line.mid(equals + 1).trimmed());
            if (key == QStringLiteral("mode")) {
                mode = value;
                continue;
            }
            const QColor color(value);
            if (color.isValid())
                colors.insert(key, color);
        }
    }
    m_colors = colors;

    // The theme file wins over the portal for dark/light: a light Omarchy
    // theme is light even if the portal still says prefer-dark.
    bool modeKnown = false;
    bool dark = m_darkMode;
    if (mode == QStringLiteral("dark")) {
        dark = true;
        modeKnown = true;
    } else if (mode == QStringLiteral("light")) {
        dark = false;
        modeKnown = true;
    } else if (colors.contains(QStringLiteral("background"))) {
        const QColor bg = colors.value(QStringLiteral("background"));
        const double luminance = 0.299 * bg.redF() + 0.587 * bg.greenF() + 0.114 * bg.blueF();
        dark = luminance < 0.5;
        modeKnown = true;
    }
    if (modeKnown && dark != m_darkMode) {
        m_darkMode = dark;
        emit darkModeChanged();
    }

    emit colorsChanged();
    watch();
}

void OmarchyTheme::watch() {
    const QStringList watched = m_watcher.files() + m_watcher.directories();
    if (!watched.isEmpty())
        m_watcher.removePaths(watched);

    // Omarchy swaps the `current/theme` symlink on theme change, so watch the
    // directories above the file too, then re-arm after every reload.
    const QFileInfo info(m_colorsPath);
    const QDir themeDir = info.dir();
    QDir currentDir = themeDir;
    currentDir.cdUp();
    if (currentDir.exists())
        m_watcher.addPath(currentDir.absolutePath());
    if (themeDir.exists())
        m_watcher.addPath(themeDir.absolutePath());
    if (info.exists())
        m_watcher.addPath(info.absoluteFilePath());
}
