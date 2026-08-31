#pragma once

#include <QObject>
#include <QString>

// What a digit does once it has been read: the click mode a keypad press
// obeys, the fixed contract the number row keeps, and the highlight
// (games/omadoku/src/sudokuinput.h).
class InputTests : public QObject {
    Q_OBJECT
private slots:
    void initTestCase();
    void init();

    void theClickModeWalksTheThreeActions();
    void theNumberRowIgnoresTheClickMode();
    void aPlainDigitOverSeveralCellsMeansANote();
    void theHighlightTakesOneDigitAtATime();

    void clickModeDecidesWhatAKeypadClickDoes();
    void keyboardMappingIgnoresTheClickMode();
    void aPlainDigitNotesTheWholeSelection();
    void clickModeCyclesAndPersists();
    void highlightTogglesAndSwitchesDigits();
    void highlightWearsAFixedHighlighterYellow();

private:
    QString m_settingsDir;
};
