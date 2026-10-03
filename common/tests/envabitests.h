#pragma once

#include <QObject>

// The C ABI (common/env/omagames_env.h), driven the way a trainer drives it
// through ctypes, over the toy WalkEnv.
class EnvAbiTests : public QObject {
    Q_OBJECT
private slots:
    void gameSpecAddsTheAbiAndMaxSteps();
    void createRefusesABadConfig();
    void envSpecFollowsTheConfig();
    void nothingIsSteppedBeforeAReset();
    void theObservationIsWhereTheSpecSays();
    void maskedAndOutOfRangeActionsAreRefused();
    void anEpisodeEndsWhenTheRulesSaySo();
    void maxStepsTruncates();
    void aCloneContinuesIdentically();
    void aReseededCloneRedrawsWhatIsHidden();
    void theReplayRecordsTheEpisode();
    void aBatchStepsAndResetsInPlace();
    void aBatchChecksEveryActionFirst();
};
