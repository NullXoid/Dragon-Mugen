#include "StageSelectOverlay.h"

#include "UiRenderPrimitives.h"

#include <SDL3/SDL_render.h>

#include <algorithm>
#include <cstddef>

namespace dragon {
namespace {

void shadowText(SDL_Renderer* renderer, float x, float y, const std::string& text, Uint8 r, Uint8 g, Uint8 b) {
    setColor(renderer, 4, 6, 10, 210);
    debugText(renderer, x + 1.0f, y + 1.0f, text);
    setColor(renderer, r, g, b);
    debugText(renderer, x, y, text);
}

void shadowTextCentered(SDL_Renderer* renderer, float centerX, float y, const std::string& text, Uint8 r, Uint8 g, Uint8 b) {
    const float x = centerX - static_cast<float>(text.size() * 8) * 0.5f;
    shadowText(renderer, x, y, text, r, g, b);
}

} // namespace

void drawStageSelectOverlay(const UiRenderContext& ui, const StageSelectView& view) {
    SDL_Renderer* renderer = ui.renderer;
    const float widthF = static_cast<float>(ui.logicalWidth);
    const float heightF = static_cast<float>(ui.logicalHeight);
    const float centerX = widthF * 0.5f;

    if (!view.hasStagePreview) {
        setColor(renderer, 10, 12, 16);
        SDL_RenderClear(renderer);

        setColor(renderer, 36, 34, 30);
        fillRect(renderer, 0, 0, widthF, heightF);
        setColor(renderer, 24, 30, 38);
        fillRect(renderer, 0, 176, widthF, 64);
        setColor(renderer, 94, 78, 54);
        fillRect(renderer, 0, 174, widthF, 1);
    }

    if (view.rows.empty()) {
        shadowTextCentered(renderer, centerX, 96.0f, "NO STAGES FOUND", 230, 130, 120);
        shadowTextCentered(renderer, centerX, 112.0f, "CHECK game/stages", 210, 218, 230);
        return;
    }

    shadowText(renderer, 18.0f, 16.0f, "DRAGON MUGEN CORE", 220, 232, 242);
    shadowTextCentered(renderer, centerX, 28.0f, fitDebugText(view.modeLabel, 20), 156, 194, 246);
    shadowTextCentered(renderer, centerX, 44.0f, "STAGE SELECT", 246, 214, 92);

    const std::string stageName = fitDebugText(view.selectedStageName, std::clamp(static_cast<int>(widthF / 8.0f) - 6, 12, 42));
    const float stageY = heightF - 38.0f;
    const bool showArrows = view.rows.size() > 1;
    shadowTextCentered(renderer, centerX, stageY, showArrows ? "< " + stageName + " >" : stageName, 246, 214, 92);
    shadowTextCentered(renderer, centerX, heightF - 18.0f, "LEFT/RIGHT stage   ENTER start   ESC fighter select", 210, 218, 230);
}

} // namespace dragon
