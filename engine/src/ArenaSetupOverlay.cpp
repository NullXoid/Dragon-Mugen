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
        return "CPU 1  " + view.cpuNames[0];
    case 2:
        return view.cpuCount >= 2 ? "CPU 2  " + view.cpuNames[1] : "CPU 2  -";
    case 3:
        return view.cpuCount >= 3 ? "CPU 3  " + view.cpuNames[2] : "CPU 3  -";
    case 4:
        return "STAGE  " + view.stageName;
    case 5:
        return "TIMER  " + view.timerLabel;
    case 6:
        return std::string("Z AXIS  ") + (view.zAxisEnabled ? "ON" : "OFF");
    case 7:
        return std::string("CAMERA ROTATE  ") + (view.cameraRotationEnabled ? "ON" : "OFF");
    case 8:
        return "START MATCH";
    case 9:
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

    setColor(renderer, 6, 8, 12, 130);
    fillRect(renderer, 0, 0, widthF, static_cast<float>(ui.logicalHeight));
    setColor(renderer, 24, 32, 46, 220);
    fillRect(renderer, 0, 0, widthF, 48);
    setColor(renderer, 158, 64, 58);
    fillRect(renderer, 0, 48, widthF, 2);

    setColor(renderer, 230, 220, 172);
    debugTextCentered(renderer, centerX, 14, view.title);
    setColor(renderer, 150, 162, 178);
    debugTextCentered(renderer, centerX, 30, view.description);

    setColor(renderer, 6, 8, 12, 214);
    fillRect(renderer, centerX - 142.0f, 58, 284, 68);
    setColor(renderer, 78, 90, 112);
    drawRect(renderer, centerX - 142.0f, 58, 284, 68);
    setColor(renderer, 230, 190, 105);
    fillRect(renderer, centerX - 138.0f, 61, 276, 2);

    setColor(renderer, 222, 226, 232);
    debugText(renderer, centerX - 130.0f, 74, "FIGHTER  " + view.fighterName);
    debugText(renderer, centerX - 130.0f, 90, "MODE     " + view.modeLabel);
    debugText(renderer, centerX - 130.0f, 106, "DEPTH    " + std::string(view.zAxisEnabled ? "SHIFT+UP/DOWN" : "OFF"));

    constexpr int rowCount = 10;
    const int selected = std::clamp(view.selectedOption, 0, rowCount - 1);
    for (int i = 0; i < rowCount; ++i) {
        const float y = 132.0f + static_cast<float>(i * 10);
        if (i == selected) {
            setColor(renderer, 74, 170, 134, static_cast<Uint8>(188 + pulse * 52.0f));
            fillRect(renderer, centerX - 142.0f, y - 3.0f, 284, 10);
            setColor(renderer, 8, 12, 16);
        } else {
            const bool inactiveCpu = (i == 2 && view.cpuCount < 2) || (i == 3 && view.cpuCount < 3);
            if (inactiveCpu) {
                setColor(renderer, 108, 116, 130);
            } else {
                setColor(renderer, 210, 220, 232);
            }
        }
        debugTextCentered(renderer, centerX, y, fitDebugText(setupLabel(i, view), 34));
    }

    setColor(renderer, 156, 166, 180);
    debugTextCentered(renderer, centerX, 230, "UP/DOWN row  LEFT/RIGHT change  ENTER select  ESC back");
}

} // namespace dragon
