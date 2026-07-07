#pragma once

#include "UiRenderContext.h"
#include "UiSpriteView.h"

#include <SDL3/SDL_rect.h>

#include <array>
#include <cstddef>
#include <string_view>

namespace dragon {

inline constexpr std::size_t kMainMenuOptionCount = 8;

struct MainMenuTitleBarView {
    std::string_view leftText = "DRAGON MUGEN CORE";
    std::string_view centerText = "MAIN MENU";
    bool visible = true;
};

struct MainMenuLogoView {
    UiSpriteView sprite;
    bool visible = false;
    float x = 0.0f;
    float y = 0.0f;
    float scale = 1.0f;
    int alpha = 255;
};

struct MainMenuPanelStyle {
    float x = -1.0f;
    float y = 181.0f;
    float w = 176.0f;
    float h = 100.0f;
    float headerH = 19.0f;
    float rowH = 9.5f;
    float selectedInsetX = 12.0f;
    float selectedInsetY = 2.0f;
    int panelFillAlpha = 228;
    int panelBorderAlpha = 160;
    int headerFillAlpha = 238;
    int selectionBorderAlpha = 166;
    int selectionFillAlpha = 238;
    int selectionUnderlineAlpha = 255;
    int shadowAlpha = 0;
    float shadowOffsetX = 0.0f;
    float shadowOffsetY = 0.0f;
};

struct MainMenuView {
    int selectedMode = 0;
    int frame = 0;
    bool exitConfirmOpen = false;
    std::string_view panelLeftText = "M.U.G.E.N";
    std::string_view panelRightText = "CORE";
    std::array<std::string_view, kMainMenuOptionCount> labels{};
    MainMenuPanelStyle panel;
    MainMenuLogoView logo;
};

struct MainMenuBackdropView {
    UiSpriteView background;
    bool fallbackGrid = true;
    float panX = 0.5f;
    int dimAlpha = 0;
};

void drawMainMenuTitleText(const UiRenderContext& ui, const MainMenuTitleBarView& title);
void drawDragonMainMenuBackdrop(const UiRenderContext& ui, const MainMenuBackdropView& backdrop);
SDL_FRect mainMenuPanelRect(const UiRenderContext& ui);
SDL_FRect mainMenuPanelRect(const UiRenderContext& ui, const MainMenuPanelStyle& style);
void drawMainMenuOverlay(const UiRenderContext& ui, const MainMenuView& view);

} // namespace dragon
