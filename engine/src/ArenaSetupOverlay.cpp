#include "ArenaSetupOverlay.h"

#include "UiRenderPrimitives.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <string>

namespace dragon {
namespace {

std::string setupLabel(int row, const ArenaSetupView& view) {
    switch (row) {
    case 0:
        return "CPU COUNT  " + std::to_string(view.cpuCount);
    case 1:
        return "START MATCH";
    case 2:
    default:
        return "BACK";
    }
}

} // namespace

void drawArenaSetupOverlay(const UiRenderContext& ui, const ArenaSetupView& view) {
    SDL_Renderer* renderer = ui.renderer;
    const float widthF = static_cast<float>(ui.logicalWidth);
    const float centerX = widthF * 0.5f;
    const float pulse = 0.5f + 0.5f * std::sin(static_cast<float>(view.frame) * 0.14f);

    setColor(renderer, 12, 16, 22);
    fillRect(renderer, 0, 0, widthF, static_cast<float>(ui.logicalHeight));
    setColor(renderer, 24, 32, 46);
    fillRect(renderer, 0, 0, widthF, 48);
    setColor(renderer, 158, 64, 58);
    fillRect(renderer, 0, 48, widthF, 2);

    setColor(renderer, 230, 220, 172);
    debugTextCentered(renderer, centerX, 14, view.title);
    setColor(renderer, 150, 162, 178);
    debugTextCentered(renderer, centerX, 30, view.description);

    setColor(renderer, 6, 8, 12, 228);
    fillRect(renderer, centerX - 128.0f, 64, 256, 62);
    setColor(renderer, 78, 90, 112);
    drawRect(renderer, centerX - 128.0f, 64, 256, 62);
    setColor(renderer, 230, 190, 105);
    fillRect(renderer, centerX - 124.0f, 67, 248, 2);

    setColor(renderer, 222, 226, 232);
    debugText(renderer, centerX - 116.0f, 78, "FIGHTER  " + view.fighterName);
    debugText(renderer, centerX - 116.0f, 94, "MODE     " + view.modeLabel);
    debugText(renderer, centerX - 116.0f, 110, "STAGE    " + view.stageName);

    constexpr int rowCount = 3;
    const int selected = std::clamp(view.selectedOption, 0, rowCount - 1);
    for (int i = 0; i < rowCount; ++i) {
        const float y = 146.0f + static_cast<float>(i * 18);
        if (i == selected) {
            setColor(renderer, 74, 170, 134, static_cast<Uint8>(188 + pulse * 52.0f));
            fillRect(renderer, centerX - 72.0f, y - 4.0f, 144, 14);
            setColor(renderer, 8, 12, 16);
        } else {
            setColor(renderer, 190, 202, 218);
        }
        debugTextCentered(renderer, centerX, y, setupLabel(i, view));
    }

    setColor(renderer, 156, 166, 180);
    debugTextCentered(renderer, centerX, 226, "UP/DOWN select  LEFT/RIGHT CPU  ENTER confirm  ESC back");
}

} // namespace dragon
