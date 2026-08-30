#pragma once

#include <QObject>
#include <QString>
#include <QTemporaryDir>

// colors.toml parsing, the per-mode defaults and the color helpers
// (common/src/omarchytheme.h).
class OmarchyThemeTest : public QObject {
    Q_OBJECT

    QTemporaryDir m_dir;

    // Every case gets its own file: the theme watches the file it reads, and
    // a shared one would let one case's colors leak into the next.
    QString writeColors(const QString &name, const QString &contents);

private slots:
    void initTestCase();
    void parsesQuotedUnquotedCommentsAndBlankLines();
    void ignoresUnknownKeysAndInvalidColors();
    void fallsBackToDefaultsPerMode();
    void readsModeFromTheFile();
    void guessesModeFromBackgroundLuminance();
    void keepsModeWhenTheFileSaysNothing();
    void missingFileLeavesEveryColorAtItsDefault();
    void setColorsPathReloadsAndSignals();
    void settersOnlySignalOnChange();
    void colorFallsBackToTheForeground();
    void mixBlendsAndClamps();
    void alphaSetsAndClamps();
};
