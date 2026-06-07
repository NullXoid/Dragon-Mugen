#pragma once

#include "UiRenderContext.h"

#include <array>
#include <string_view>

namespace dragon {

struct PauseMenuView {
    std::string_view modeLabel = "TRAINING";
    int selectedOption = 0;
    std::array<std::string_view, 5> optionLabels{
        "RESUME",
        "RESTART MATCH",
        "FIGHTER SELECT",
        "STAGE SELECT",
        "MODE SELECT",
    };
};

void drawSingleFightPauseMenu(const UiRenderContext& ui, const PauseMenuView& view);

} // namespace dragon
