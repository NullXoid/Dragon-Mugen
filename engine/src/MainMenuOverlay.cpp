#include "MainMenuOverlay.h"

#include "UiRenderPrimitives.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <string>
#include <string_view>

namespace dragon {
namespace {

constexpr std::array<std::string_view, 5> kModeLabels{ {
    "TRAINING",
    "SINGLE PLAYER",
    "VS MODE",
    "OPTIONS",
    "EXIT",
} };

constexpr std::array<std::string_view, kModeLabels.size()> kModeDescriptions{ {
    "TRAINING SANDBOX",
    "ARCADE STYLE MATCH",
    "LOCAL COUCH MATCH",
    "OPTIONS",
    "EXIT",
} };

float menuCenterX(const UiRenderContext& ui) {
    return static_cast<float>(ui.logicalWidth) * 0.5f;
}

} // namespace

void drawMainMenuTitleText(const UiRenderContext& ui) {
    SDL_Renderer* renderer = ui.renderer;
    const float centerX = menuCenterX(ui);

    setColor(renderer, 6, 8, 12, 150);
    fillRect(renderer, centerX - 112.0f, 12.0f, 224.0f, 15.0f);
    setColor(renderer, 158, 64, 58);
    fillRect(renderer, centerX - 86.0f, 28.0f, 172.0f, 1.0f);
    setColor(renderer, 230, 220, 172);
    debugTextCentered(renderer, centerX, 16, "DRAGON MUGEN CORE");
}

void drawMainMenuOverlay(const UiRenderContext& ui, const MainMenuView& view) {
    SDL_Renderer* renderer = ui.renderer;
    const float centerX = menuCenterX(ui);
    const float menuX = centerX - 76.0f;
    const int selectedMode = std::clamp(view.selectedMode, 0, static_cast<int>(kModeLabels.size()) - 1);
    const float pulse = 0.5f + 0.5f * std::sin(static_cast<float>(view.frame) * 0.16f);
    const int pulseAlpha = 96 + static_cast<int>(pulse * 70.0f);

    setColor(renderer, 6, 8, 12, 224);
    fillRect(renderer, menuX, 130, 152, 84);
    setColor(renderer, 24, 32, 46, 238);
    fillRect(renderer, menuX + 2.0f, 132.0f, 148.0f, 14.0f);
    setColor(renderer, 92, 108, 138);
    drawRect(renderer, menuX, 130, 152, 84);
    setColor(renderer, 158, 64, 58);
    fillRect(renderer, menuX + 2.0f, 146.0f, 148.0f, 2.0f);

    for (int i = 0; i < static_cast<int>(kModeLabels.size()); ++i) {
        const float y = 156.0f + static_cast<float>(i * 13);
        const std::string label(kModeLabels[static_cast<std::size_t>(i)]);
        const float textX = centerX - static_cast<float>(label.size()) * 4.0f;

        if (i == selectedMode) {
            setColor(renderer, 230, 190, 105, static_cast<Uint8>(pulseAlpha));
            fillRect(renderer, menuX + 12.0f, y - 8.0f, 128.0f, 12.0f);
            setColor(renderer, 8, 12, 16, 230);
            fillRect(renderer, menuX + 15.0f, y - 6.0f, 122.0f, 8.0f);
            setColor(renderer, 230, 220, 172);
            fillRect(renderer, menuX + 17.0f, y + 4.0f, 118.0f, 1.0f);
            setColor(renderer, 255, 238, 120);
            debugText(renderer, textX, y - 6, label);
        } else {
            setColor(renderer, 190, 202, 218);
            debugText(renderer, textX, y - 6, label);
        }
    }

    setColor(renderer, 18, 24, 34, 232);
    fillRect(renderer, centerX - 118.0f, 216.0f, 236.0f, 20.0f);
    setColor(renderer, view.exitConfirmOpen ? 158 : 74, view.exitConfirmOpen ? 64 : 170, view.exitConfirmOpen ? 58 : 134);
    drawRect(renderer, centerX - 118.0f, 216.0f, 236.0f, 20.0f);
    if (view.exitConfirmOpen) {
        setColor(renderer, 255, 222, 130);
        debugTextCentered(renderer, centerX, 219, "ARE YOU SURE?");
        setColor(renderer, 150, 156, 166);
        debugTextCentered(renderer, centerX, 230, "ENTER YES  ESC NO");
    } else {
        setColor(renderer, 230, 220, 172);
        debugTextCentered(renderer, centerX, 219, std::string(kModeDescriptions[static_cast<std::size_t>(selectedMode)]));

        setColor(renderer, 150, 156, 166);
        debugTextCentered(renderer, centerX, 230, "UP/DOWN  ENTER  ESC");
    }
}

} // namespace dragon
