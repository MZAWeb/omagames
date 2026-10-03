#pragma once

// What makes a run of Oma2048 play out the way it does, for an agent's
// replays and trained models (docs/AGENT-ENV.md).
namespace Rules {

// Goes up with any change that makes the same moves from the same seed play
// out differently (the slide, the merges, the spawn odds), so a replay or a
// model from before is refused rather than misread.
constexpr int kVersion = 1;

}  // namespace Rules
