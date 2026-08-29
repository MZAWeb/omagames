#include <QColor>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

#include "omarchytheme.h"

namespace {
// The defaults OmarchyTheme falls back to, so a game outside Omarchy still
// renders. Kept here so a change to them has to be a deliberate one.
const QColor DarkBackground("#101010");
const QColor LightBackground("#ffffff");
const QColor DarkMuted("#555555");
const QColor LightMuted("#9a9a9a");
const QColor DefaultRed("#e06c75");

// QColor keeps its channels as 16-bit integers, so a float set and read back
// is only ever nearly the value that went in.
bool nearly(qreal actual, qreal expected) { return qAbs(actual - expected) < 0.001; }
}

class OmarchyThemeTest : public QObject {
    Q_OBJECT

    QTemporaryDir m_dir;

    // Every case gets its own file: the theme watches the file it reads, and
    // a shared one would let one case's colors leak into the next.
    QString writeColors(const QString &name, const QString &contents) {
        const QString path = m_dir.filePath(name);
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
            return QString();
        file.write(contents.toUtf8());
        return path;
    }

private slots:
    void initTestCase() { QVERIFY(m_dir.isValid()); }

    void parsesQuotedUnquotedCommentsAndBlankLines() {
        const QString path = writeColors(QStringLiteral("parse.toml"), QStringLiteral(
            "# Omarchy theme\n"
            "\n"
            "background = \"#101020\"\n"
            "foreground = #f0f0f0\n"
            "  accent   =   '#ff0000'  \n"
            "\n"
            "# selection is commented out and must not be picked up\n"
            "# selection = \"#00ff00\"\n"
            "this line has no equals sign\n"
            "green = \"#00ff00\"\n"));
        QVERIFY(!path.isEmpty());

        OmarchyTheme theme;
        theme.setColorsPath(path);

        QCOMPARE(theme.background(), QColor("#101020"));
        QCOMPARE(theme.foreground(), QColor("#f0f0f0"));
        QCOMPARE(theme.accent(), QColor("#ff0000"));
        // A garbage line neither aborts the parse nor invents a color.
        QCOMPARE(theme.green(), QColor("#00ff00"));
        QCOMPARE(theme.selection(), QColor("#186a9a"));
    }

    void ignoresUnknownKeysAndInvalidColors() {
        const QString path = writeColors(QStringLiteral("junk.toml"), QStringLiteral(
            "mode = \"dark\"\n"
            "background = \"#101020\"\n"
            "red = \"not-a-color\"\n"
            "terminal_bright_puce = \"#123456\"\n"
            "foreground = \"#f0f0f0\"\n"));
        QVERIFY(!path.isEmpty());

        OmarchyTheme theme;
        theme.setColorsPath(path);

        // An unparseable value is dropped and the default stands in.
        QCOMPARE(theme.red(), DefaultRed);
        // A key no game knows about disturbs nothing around it.
        QCOMPARE(theme.background(), QColor("#101020"));
        QCOMPARE(theme.foreground(), QColor("#f0f0f0"));
    }

    void fallsBackToDefaultsPerMode() {
        const QString dark = writeColors(QStringLiteral("bare-dark.toml"),
                                         QStringLiteral("mode = \"dark\"\n"));
        const QString light = writeColors(QStringLiteral("bare-light.toml"),
                                          QStringLiteral("mode = \"light\"\n"));
        QVERIFY(!dark.isEmpty() && !light.isEmpty());

        OmarchyTheme theme;
        theme.setColorsPath(dark);
        QCOMPARE(theme.background(), DarkBackground);
        QCOMPARE(theme.muted(), DarkMuted);
        QCOMPARE(theme.red(), DefaultRed);

        theme.setColorsPath(light);
        QCOMPARE(theme.background(), LightBackground);
        QCOMPARE(theme.muted(), LightMuted);
        // The ANSI colors do not depend on the mode.
        QCOMPARE(theme.red(), DefaultRed);
    }

    void readsModeFromTheFile() {
        // The theme file wins over the luminance guess: a light theme with a
        // black background is still light.
        const QString light = writeColors(QStringLiteral("mode-light.toml"), QStringLiteral(
            "mode = \"light\"\n"
            "background = \"#000000\"\n"));
        const QString dark = writeColors(QStringLiteral("mode-dark.toml"), QStringLiteral(
            "mode = \"dark\"\n"
            "background = \"#ffffff\"\n"));
        QVERIFY(!light.isEmpty() && !dark.isEmpty());

        OmarchyTheme theme;
        theme.setColorsPath(light);
        QCOMPARE(theme.darkMode(), false);

        theme.setColorsPath(dark);
        QCOMPARE(theme.darkMode(), true);
    }

    void guessesModeFromBackgroundLuminance() {
        const QString light = writeColors(QStringLiteral("lum-light.toml"),
                                          QStringLiteral("background = \"#fdf6e3\"\n"));
        const QString dark = writeColors(QStringLiteral("lum-dark.toml"),
                                         QStringLiteral("background = \"#1b1b2a\"\n"));
        QVERIFY(!light.isEmpty() && !dark.isEmpty());

        OmarchyTheme theme;
        theme.setColorsPath(light);
        QCOMPARE(theme.darkMode(), false);

        theme.setColorsPath(dark);
        QCOMPARE(theme.darkMode(), true);
    }

    void keepsModeWhenTheFileSaysNothing() {
        const QString path = writeColors(QStringLiteral("no-mode.toml"),
                                         QStringLiteral("accent = \"#ff0000\"\n"));
        QVERIFY(!path.isEmpty());

        OmarchyTheme theme;
        theme.setDarkMode(false);
        theme.setColorsPath(path);
        QCOMPARE(theme.darkMode(), false);

        theme.setDarkMode(true);
        theme.setColorsPath(path);
        QCOMPARE(theme.darkMode(), true);
    }

    void missingFileLeavesEveryColorAtItsDefault() {
        OmarchyTheme theme;
        theme.setDarkMode(true);
        theme.setColorsPath(m_dir.filePath(QStringLiteral("nothing-here.toml")));
        QCOMPARE(theme.background(), DarkBackground);
        QCOMPARE(theme.darkMode(), true);
    }

    void setColorsPathReloadsAndSignals() {
        const QString dark = writeColors(QStringLiteral("signal-dark.toml"), QStringLiteral(
            "mode = \"dark\"\n"
            "background = \"#101010\"\n"));
        const QString light = writeColors(QStringLiteral("signal-light.toml"), QStringLiteral(
            "mode = \"light\"\n"
            "background = \"#ffffff\"\n"));
        QVERIFY(!dark.isEmpty() && !light.isEmpty());

        OmarchyTheme theme;
        theme.setColorsPath(dark);

        QSignalSpy colors(&theme, &OmarchyTheme::colorsChanged);
        QSignalSpy mode(&theme, &OmarchyTheme::darkModeChanged);

        theme.setColorsPath(light);
        QCOMPARE(theme.background(), QColor("#ffffff"));
        QCOMPARE(colors.count(), 1);
        QCOMPARE(mode.count(), 1);

        // Re-reading a file with the same mode still re-tints, but the mode
        // itself has not moved.
        theme.setColorsPath(light);
        QCOMPARE(colors.count(), 2);
        QCOMPARE(mode.count(), 1);
    }

    void settersOnlySignalOnChange() {
        OmarchyTheme theme;
        theme.setColorsPath(writeColors(QStringLiteral("setters.toml"),
                                        QStringLiteral("mode = \"dark\"\n")));

        QSignalSpy mode(&theme, &OmarchyTheme::darkModeChanged);
        QSignalSpy scale(&theme, &OmarchyTheme::textScaleChanged);

        theme.setDarkMode(true);
        QCOMPARE(mode.count(), 0);
        theme.setDarkMode(false);
        QCOMPARE(mode.count(), 1);
        // Light defaults follow the mode without another file read.
        QCOMPARE(theme.background(), LightBackground);

        theme.setTextScale(1.0);
        QCOMPARE(scale.count(), 0);
        theme.setTextScale(1.5);
        QCOMPARE(scale.count(), 1);
        QCOMPARE(theme.textScale(), 1.5);
    }

    void colorFallsBackToTheForeground() {
        const QString path = writeColors(QStringLiteral("raw-keys.toml"), QStringLiteral(
            "mode = \"dark\"\n"
            "foreground = \"#abcdef\"\n"
            "terminal_bright_puce = \"#123456\"\n"));
        QVERIFY(!path.isEmpty());

        OmarchyTheme theme;
        theme.setColorsPath(path);

        // Any raw key from the file is readable...
        QCOMPARE(theme.color(QStringLiteral("terminal_bright_puce")), QColor("#123456"));
        // ...and a key that is neither in the file nor a default reads as the
        // foreground, so a missing color is visible rather than transparent.
        QCOMPARE(theme.color(QStringLiteral("no_such_key")), QColor("#abcdef"));
    }

    void mixBlendsAndClamps() {
        const QColor black("#000000");
        const QColor white("#ffffff");

        QCOMPARE(OmarchyTheme::mix(black, white, 0.0), black);
        QCOMPARE(OmarchyTheme::mix(black, white, 1.0), white);
        QCOMPARE(OmarchyTheme::mix(black, white, 0.5), QColor::fromRgbF(0.5, 0.5, 0.5));
        // Out-of-range amounts clamp instead of overshooting into garbage.
        QCOMPARE(OmarchyTheme::mix(black, white, 2.5), white);
        QCOMPARE(OmarchyTheme::mix(black, white, -1.0), black);

        // Channels blend independently, and the result is always opaque.
        const QColor blended = OmarchyTheme::mix(QColor("#ff0000"), QColor("#0000ff"), 0.25);
        QCOMPARE(blended, QColor::fromRgbF(0.75, 0.0, 0.25));
        QVERIFY(nearly(OmarchyTheme::mix(OmarchyTheme::alpha(black, 0.0), white, 0.0).alphaF(), 1.0));
    }

    void alphaSetsAndClamps() {
        const QColor red("#ff0000");

        QVERIFY(nearly(OmarchyTheme::alpha(red, 0.5).alphaF(), 0.5));
        QVERIFY(nearly(OmarchyTheme::alpha(red, 2.0).alphaF(), 1.0));
        QVERIFY(nearly(OmarchyTheme::alpha(red, -0.5).alphaF(), 0.0));

        // Only the alpha moves.
        const QColor faded = OmarchyTheme::alpha(red, 0.25);
        QCOMPARE(faded.red(), 255);
        QCOMPARE(faded.green(), 0);
        QCOMPARE(faded.blue(), 0);
    }
};

QTEST_GUILESS_MAIN(OmarchyThemeTest)
#include "tst_common.moc"
