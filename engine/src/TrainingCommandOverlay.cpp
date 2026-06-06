#include "TrainingCommandOverlay.h"

#include "UiRenderPrimitives.h"

#include <algorithm>
#include <cstddef>
#include <string>

namespace dragon {

void drawTrainingCommandOverlay(const UiRenderContext& ui, const TrainingCommandHudView& view) {
    if (!view.input.visible && !view.commandsVisible) {
        return;
    }

    SDL_Renderer* renderer = ui.renderer;
    const float scale = ui.scale;
    const float widthF = static_cast<float>(ui.logicalWidth);
    const float panelW = std::min(194.0f, widthF - 16.0f);
    const float panelX = widthF - panelW - 8.0f;
    const float panelY = 42.0f;
    const float panelH = view.input.visible && view.commandsVisible ? 112.0f : 62.0f;

    setColor(renderer, 6, 8, 14, 216);
    fillScaledRect(renderer, scale, panelX, panelY, panelW, panelH);
    setColor(renderer, 54, 70, 98);
    drawScaledRect(renderer, scale, panelX, panelY, panelW, panelH);

    float y = panelY + 8.0f;
    if (view.input.visible) {
        setColor(renderer, 230, 220, 172);
        scaledDebugText(renderer, scale, panelX + 8.0f, y, "INPUT");
        setColor(renderer, 222, 226, 232);
        scaledDebugText(renderer, scale, panelX + 58.0f, y, view.input.currentInput);
        y += 14.0f;

        setColor(renderer, 130, 142, 156);
        scaledDebugText(renderer, scale, panelX + 8.0f, y, view.input.recentInputs);
        y += 20.0f;
    }

    if (view.commandsVisible) {
        setColor(renderer, 230, 220, 172);
        scaledDebugText(renderer, scale, panelX + 8.0f, y, "COMMANDS");
        y += 13.0f;

        int drawn = 0;
        for (const auto& row : view.commandRows) {
            if (drawn >= 5) {
                break;
            }
            if (row.active) {
                setColor(renderer, 74, 170, 134);
                fillScaledRect(renderer, scale, panelX + 6.0f, y - 2.0f, panelW - 12.0f, 11.0f);
                setColor(renderer, 8, 12, 16);
            } else {
                setColor(renderer, 174, 184, 196);
            }
            scaledDebugText(renderer, scale, panelX + 10.0f, y, row.label);
            scaledDebugText(renderer, scale, panelX + 116.0f, y, row.input);
            ++drawn;
            y += 12.0f;
        }

        if (!view.activeCommandLabel.empty()) {
            setColor(renderer, 230, 190, 105);
            scaledDebugText(renderer, scale, panelX + 8.0f, panelY + panelH - 14.0f, "MATCH " + view.activeCommandLabel);
        }
    }
}

} // namespace dragon
