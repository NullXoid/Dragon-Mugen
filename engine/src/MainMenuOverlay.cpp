#include "MainMenuOverlay.h"

#include "UiRenderPrimitives.h"

#include <algorithm>
#include <array>
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

    setColor(renderer, 230, 230, 238);
    debugTextCentered(renderer, centerX, 16, "DRAGON MUGEN CORE");
}

void drawMainMenuOverlay(const UiRenderContext& ui, const MainMenuView& view) {
    SDL_Renderer* renderer = ui.renderer;
    const float centerX = menuCenterX(ui);
    const float menuX = centerX - 64.0f;
    const int selectedMode = std::clamp(view.selectedMode, 0, static_cast<int>(kModeLabels.size()) - 1);

    setColor(renderer, 10, 10, 12);
    fillRect(renderer, menuX, 138, 128, 70);
    setColor(renderer, 96, 106, 122);
    drawRect(renderer, menuX, 138, 128, 70);

    for (int i = 0; i < static_cast<int>(kModeLabels.size()); ++i) {
        const float y = 150.0f + static_cast<float>(i * 13);
        const std::string label(kModeLabels[static_cast<std::size_t>(i)]);
        const float textX = centerX - static_cast<float>(label.size()) * 4.0f;

        if (i == selectedMode) {
            setColor(renderer, 226, 210, 78);
            drawRect(renderer, menuX + 8.0f, y - 9, 112, 13);
            setColor(renderer, 255, 238, 96);
            debugText(renderer, textX, y - 6, label);
        } else {
            setColor(renderer, 220, 224, 232);
            debugText(renderer, textX, y - 6, label);
        }
    }

    if (view.exitConfirmOpen) {
        setColor(renderer, 255, 238, 96);
        debugTextCentered(renderer, centerX, 214, "ARE YOU SURE?");
        setColor(renderer, 150, 156, 166);
        debugTextCentered(renderer, centerX, 226, "ENTER YES  ESC NO");
    } else {
        setColor(renderer, 220, 224, 232);
        debugTextCentered(renderer, centerX, 214, std::string(kModeDescriptions[static_cast<std::size_t>(selectedMode)]));

        setColor(renderer, 150, 156, 166);
        debugTextCentered(renderer, centerX, 226, "UP/DOWN  ENTER  ESC");
    }
}

} // namespace dragon
