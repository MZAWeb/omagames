#pragma once

#include <QString>
#include <array>

// The three classic Minesweeper sizes. `key` is the stable id used for
// settings and saved games; `label` is what the UI shows.
enum class Preset { Beginner, Intermediate, Expert };
constexpr int kPresetCount = 3;

struct PresetSpec {
    Preset id;
    const char *key;
    const char *label;
    int width;
    int height;
    int mines;
};

namespace Presets {

constexpr std::array<PresetSpec, kPresetCount> kAll = {{
    {Preset::Beginner, "beginner", "Beginner", 9, 9, 10},
    {Preset::Intermediate, "intermediate", "Intermediate", 16, 16, 40},
    {Preset::Expert, "expert", "Expert", 30, 16, 99},
}};

constexpr const PresetSpec &spec(Preset preset) { return kAll[size_t(preset)]; }

// A preset's key is the id everything outside the engine uses: the setting,
// the QML model and the best-times table.
QString id(Preset preset);
QString label(Preset preset);
// False, leaving `preset` untouched, when the id is not one of ours.
bool fromId(const QString &id, Preset *preset);

}  // namespace Presets
