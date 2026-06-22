#pragma once

#include "dragon/DragonProgression.h"

#include <filesystem>
#include <string>

namespace dragon {

struct ProgressionState {
    DragonProgressionData data;
    DragonProgressionSave save;
    std::filesystem::path savePath;
    bool loaded = false;
    bool matchAwardApplied = false;
    std::string lastAwardText;
};

} // namespace dragon
