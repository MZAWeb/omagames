#pragma once

#include <QColor>
#include <QFileSystemWatcher>
#include <QMap>
#include <QObject>

// Live view of the Omarchy theme, exposed to QML as the `theme` context
// property. Colors come from ~/.local/state/omarchy/current/theme/colors.toml
// and re-tint when the theme changes; dark mode and text scale are fed in by
// SystemTheme (portal / Qt style hints) through the setters.
//
// Games must only ever pull colors from here — never hardcode a color in QML —
// so the look can be reworked later without touching game logic.
class OmarchyTheme : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool darkMode READ darkMode WRITE setDarkMode NOTIFY darkModeChanged)
    Q_PROPERTY(qreal textScale READ textScale WRITE setTextScale NOTIFY textScaleChanged)

    Q_PROPERTY(QColor background READ background NOTIFY colorsChanged)
    Q_PROPERTY(QColor darkBackground READ darkBackground NOTIFY colorsChanged)
    Q_PROPERTY(QColor darkerBackground READ darkerBackground NOTIFY colorsChanged)
    Q_PROPERTY(QColor lighterBackground READ lighterBackground NOTIFY colorsChanged)
    Q_PROPERTY(QColor foreground READ foreground NOTIFY colorsChanged)
    Q_PROPERTY(QColor darkForeground READ darkForeground NOTIFY colorsChanged)
    Q_PROPERTY(QColor lightForeground READ lightForeground NOTIFY colorsChanged)
    Q_PROPERTY(QColor brightForeground READ brightForeground NOTIFY colorsChanged)
    Q_PROPERTY(QColor accent READ accent NOTIFY colorsChanged)
    Q_PROPERTY(QColor selection READ selection NOTIFY colorsChanged)
    Q_PROPERTY(QColor muted READ muted NOTIFY colorsChanged)
    Q_PROPERTY(QColor red READ red NOTIFY colorsChanged)
    Q_PROPERTY(QColor yellow READ yellow NOTIFY colorsChanged)
    Q_PROPERTY(QColor orange READ orange NOTIFY colorsChanged)
    Q_PROPERTY(QColor green READ green NOTIFY colorsChanged)
    Q_PROPERTY(QColor cyan READ cyan NOTIFY colorsChanged)
    Q_PROPERTY(QColor blue READ blue NOTIFY colorsChanged)
    Q_PROPERTY(QColor magenta READ magenta NOTIFY colorsChanged)

public:
    explicit OmarchyTheme(QObject *parent = nullptr);

    bool darkMode() const { return m_darkMode; }
    qreal textScale() const { return m_textScale; }

    QColor background() const { return color("background"); }
    QColor darkBackground() const { return color("dark_background"); }
    QColor darkerBackground() const { return color("darker_background"); }
    QColor lighterBackground() const { return color("lighter_background"); }
    QColor foreground() const { return color("foreground"); }
    QColor darkForeground() const { return color("dark_foreground"); }
    QColor lightForeground() const { return color("light_foreground"); }
    QColor brightForeground() const { return color("bright_foreground"); }
    QColor accent() const { return color("accent"); }
    QColor selection() const { return color("selection"); }
    QColor muted() const { return color("muted"); }
    QColor red() const { return color("red"); }
    QColor yellow() const { return color("yellow"); }
    QColor orange() const { return color("orange"); }
    QColor green() const { return color("green"); }
    QColor cyan() const { return color("cyan"); }
    QColor blue() const { return color("blue"); }
    QColor magenta() const { return color("magenta"); }

    // Any raw key from colors.toml (with fallback), handy for one-offs.
    Q_INVOKABLE QColor color(const QString &key) const;
    // Linear blend between two colors, amount in [0, 1].
    Q_INVOKABLE static QColor mix(const QColor &base, const QColor &tint, qreal amount);
    // Same color with a different alpha in [0, 1].
    Q_INVOKABLE static QColor alpha(const QColor &color, qreal alpha);

    // Override the file that is read; used by tests and by `--theme-file`.
    void setColorsPath(const QString &path);

public slots:
    void setDarkMode(bool darkMode);
    void setTextScale(qreal textScale);
    void reload();

signals:
    void darkModeChanged();
    void textScaleChanged();
    void colorsChanged();

private:
    void watch();
    QMap<QString, QColor> defaults() const;

    bool m_darkMode = true;
    qreal m_textScale = 1.0;
    QString m_colorsPath;
    QMap<QString, QColor> m_colors;
    QFileSystemWatcher m_watcher;
};
