#pragma once

#include <QObject>

// The pieces under the env ABI: the observation layout, config resolution
// and replay/v1 (common/env/).
class EnvPartsTests : public QObject {
    Q_OBJECT
private slots:
    void layoutAlignsEveryTensor();
    void layoutWritesWhereItSays();
    void layoutNamesTheColumnsItIsGiven();

    void configFillsInDefaults();
    void configRejectsAnUnknownKey();
    void configRejectsABadChoice();
    void configRejectsOutOfRangeAndFractions();
    void configRejectsTheWrongType();

    void replayMergesIdleTicks();
    void replayRoundTrips();
    void replayRefusesWhatItCannotPlay();
    void replayKnowsWhoCanPlayIt();
    void replayFilesAreRead();
};
