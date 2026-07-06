#pragma once

#include "UiRenderContext.h"
#include "UiSpriteView.h"

#include <SDL3/SDL_rect.h>

#include <array>
#include <cstddef>
#include <string_view>

namespace dragon {

inline constexpr std::size_t kMainMenuOptionCount = 8;

struct MainMenuView {
    int selectedMode = 0;
    int frame = 0;
    bool exitConfirmOpen = false;
    std::array<std::string_view, kMainMenuOptionCount> labels{};
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
