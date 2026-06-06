#pragma once

#include "UiRenderContext.h"

namespace dragon {

struct MainMenuView {
    int selectedMode = 0;
};

void drawMainMenuTitleText(const UiRenderContext& ui);
void drawMainMenuOverlay(const UiRenderContext& ui, const MainMenuView& view);

} // namespace dragon
