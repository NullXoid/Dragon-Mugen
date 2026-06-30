#pragma once

#include "UiRenderContext.h"
#include "UiSpriteView.h"

#include <SDL3/SDL_rect.h>

namespace dragon {

struct MainMenuView {
    int selectedMode = 0;
    int frame = 0;
    bool exitConfirmOpen = false;
};

struct MainMenuBackdropView {
    UiSpriteView background;
    bool fallbackGrid = true;
    float panX = 0.5f;
    int dimAlpha = 0;
};

void drawMainMenuTitleText(const UiRenderContext& ui);
void drawDragonMainMenuBackdrop(const UiRenderContext& ui, const MainMenuBackdropView& backdrop);
SDL_FRect mainMenuPanelRect(const UiRenderContext& ui);
void drawMainMenuOverlay(const UiRenderContext& ui, const MainMenuView& view);

} // namespace dragon
