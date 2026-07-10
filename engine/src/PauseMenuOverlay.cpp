#include "PauseMenuOverlay.h"

#include "DragonUi.h"
#include "UiRenderPrimitives.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <string>

namespace dragon {

void drawSingleFightPauseMenu(const UiRenderContext& ui, const PauseMenuView& view) {
    SDL_Renderer* renderer = ui.renderer;
    const DragonUiMetrics metrics = dragonUiMetricsForContext(ui);
    const SDL_FRect safe = dragonPixelUiSafeArea(CanvasDimensions{ ui.logicalWidth, ui.logicalHeight });
    const auto& tokens = dragonUiTokens();
    const float s = metrics.pixelScale;
    const float panelW = std::min(184.0f * s, safe.w - 16.0f * s);
    const float panelH = 156.0f * s;
    const float panelX = safe.x + std::floor((safe.w - panelW) * 0.5f);
    const float panelY = safe.y + std::floor((safe.h - panelH) * 0.5f);

    setColor(renderer, tokens.panelBase, 238);
    fillRect(renderer, panelX, panelY, panelW, panelH);
    setColor(renderer, tokens.primaryTeal, 190);
    drawRect(renderer, panelX, panelY, panelW, panelH);
    setColor(renderer, tokens.secondaryPanel, 236);
    fillRect(renderer, panelX + 2.0f * s, panelY + 2.0f * s, panelW - 4.0f * s, 28.0f * s);
    setColor(renderer, tokens.separatorRed);
    fillRect(renderer, panelX + 2.0f * s, panelY + 30.0f * s, panelW - 4.0f * s, std::max(1.0f, s));

    setColor(renderer, tokens.mutedGold);
    scaledDebugText(renderer, s, panelX + 16.0f * s, panelY + 10.0f * s, std::string(view.modeLabel));
    setColor(renderer, tokens.mutedText);
    scaledDebugText(renderer, s, panelX + panelW - 68.0f * s, panelY + 10.0f * s, "PAUSE");

    const int selectedOption =
        std::clamp(view.selectedOption, 0, static_cast<int>(view.optionLabels.size()) - 1);
    for (int i = 0; i < static_cast<int>(view.optionLabels.size()); ++i) {
        const float y = panelY + (46.0f + static_cast<float>(i * 18)) * s;
        if (i == selectedOption) {
            setColor(renderer, tokens.primaryTeal, 210);
            fillRect(renderer, panelX + 16.0f * s, y - 3.0f * s, panelW - 32.0f * s, 14.0f * s);
            setColor(renderer, 8, 12, 16);
        } else {
            setColor(renderer, tokens.primaryText);
        }
        scaledDebugText(
            renderer,
            s,
            panelX + 26.0f * s,
            y,
            std::string(view.optionLabels[static_cast<std::size_t>(i)]));
    }

    setColor(renderer, tokens.mutedText);
    scaledDebugText(renderer, s, panelX + 18.0f * s, panelY + panelH - 20.0f * s, "ENTER SELECT  ESC RESUME");
}

} // namespace dragon
