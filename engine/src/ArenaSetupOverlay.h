#pragma once

#include "UiRenderContext.h"

#include <array>
#include <string>
#include <vector>

namespace dragon {

struct ArenaSetupView {
    std::string title;
    std::string description;
    std::string fighterName;
    int cpuCount = 1;
    std::array<std::string, 3> cpuNames{ "Random", "Random", "Random" };
    std::string modeLabel = "Free-for-all";
    std::string stageName;
    std::string timerLabel = "INF";
    bool zAxisEnabled = true;
    bool cameraRotationEnabled = false;
    int selectedOption = 0;
    int frame = 0;
};

void drawArenaSetupOverlay(const UiRenderContext& ui, const ArenaSetupView& view);

} // namespace dragon
