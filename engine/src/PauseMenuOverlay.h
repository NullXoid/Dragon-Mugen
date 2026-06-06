#pragma once

#include "UiRenderContext.h"

#include <string_view>

namespace dragon {

struct PauseMenuView {
    std::string_view modeLabel = "TRAINING";
    int selectedOption = 0;
};

void drawSingleFightPauseMenu(const UiRenderContext& ui, const PauseMenuView& view);

} // namespace dragon
