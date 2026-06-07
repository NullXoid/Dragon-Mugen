#pragma once

#include "UiRenderContext.h"

#include <array>
#include <string>

namespace dragon {

struct ArenaSetupView {
    std::string title;
    std::string description;
    std::string fighterName;
    int cpuCount = 1;
    std::string modeLabel = "Free-for-all";
    std::string stageName;
    int selectedOption = 0;
    int frame = 0;
};

void drawArenaSetupOverlay(const UiRenderContext& ui, const ArenaSetupView& view);

} // namespace dragon
