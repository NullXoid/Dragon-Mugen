#include "StageSelectOverlay.h"

#include "UiRenderPrimitives.h"

#include <SDL3/SDL_render.h>

#include <algorithm>
#include <cstddef>

namespace dragon {

void drawStageSelectOverlay(const UiRenderContext& ui, const StageSelectView& view) {
    SDL_Renderer* renderer = ui.renderer;
    const float widthF = static_cast<float>(ui.logicalWidth);
    const float heightF = static_cast<float>(ui.logicalHeight);

    if (!view.hasStagePreview) {
        setColor(renderer, 10, 12, 16);
        SDL_RenderClear(renderer);

        setColor(renderer, 36, 34, 30);
        fillRect(renderer, 0, 0, widthF, heightF);
        setColor(renderer, 24, 30, 38);
        fillRect(renderer, 0, 176, widthF, 64);
        setColor(renderer, 94, 78, 54);
        fillRect(renderer, 0, 174, widthF, 1);
    } else {
        setColor(renderer, 4, 6, 10, 88);
        fillRect(renderer, 0, 0, widthF, heightF);
        setColor(renderer, 6, 8, 12, 190);
        fillRect(renderer, 0, 0, widthF, 48);
        setColor(renderer, 6, 8, 12, 178);
        fillRect(renderer, 0, 174, widthF, heightF - 174.0f);
        setColor(renderer, 214, 174, 76, 210);
        fillRect(renderer, 0, 174, widthF, 1);
    }

    setColor(renderer, 210, 224, 238);
    debugText(renderer, 18, 16, "DRAGON MUGEN CORE");
    setColor(renderer, 220, 178, 112);
    debugText(renderer, 20, 30, "STAGE SELECT");
    setColor(renderer, 155, 164, 174);
    debugText(renderer, 210, 30, fitDebugText(view.modeLabel, 15));

    const float panelX = 18.0f;
    const float panelY = 52.0f;
    const float panelW = std::max(284.0f, widthF - 36.0f);
    const float panelH = 108.0f;
    const float detailX = panelX + (panelW > 320.0f ? panelW * 0.50f : 164.0f);
    const int rowChars = std::clamp(static_cast<int>((detailX - panelX - 24.0f) / 8.0f), 10, 24);
    const int detailChars = std::clamp(static_cast<int>((panelX + panelW - detailX - 10.0f) / 8.0f), 12, 30);
    drawPanel(renderer, panelX, panelY, panelW, panelH);

    if (view.rows.empty()) {
        setColor(renderer, 230, 130, 120);
        debugText(renderer, 32, 72, "No stages found in game/stages");
    } else {
        for (int i = 0; i < static_cast<int>(view.rows.size()); ++i) {
            const float y = 66.0f + static_cast<float>(i * 20);
            const auto& row = view.rows[static_cast<std::size_t>(i)];
            if (row.selected) {
                const int pulse = 150 + ((view.frame / 8) % 55);
                setColor(renderer, static_cast<Uint8>(pulse), 124, 58);
                fillRect(renderer, 28, y - 3, detailX - 44.0f, 15);
                setColor(renderer, 8, 12, 16);
                debugText(renderer, 36, y, fitDebugText(row.label, rowChars));
            } else {
                setColor(renderer, 184, 178, 168);
                debugText(renderer, 36, y, fitDebugText(row.label, rowChars));
            }
        }

        setColor(renderer, 222, 226, 232);
        debugText(renderer, detailX, 68, fitDebugText(view.selectedStageName, detailChars));
        setColor(renderer, 155, 164, 174);
        debugText(renderer, detailX, 84, fitDebugText("id: " + view.selectedStageId, detailChars));
        debugText(renderer, detailX, 96, fitDebugText("author: " + view.selectedStageAuthor, detailChars));
        debugText(renderer, detailX, 116, fitDebugText("fighter: " + view.fighterLabel, detailChars));
        debugText(renderer, detailX, 132, fitDebugText("opponent: " + view.opponentLabel, detailChars));
    }

    setColor(renderer, 118, 126, 138);
    debugText(renderer, 20, 204, "UP/DOWN choose  ENTER start  ESC fighter select");
}

} // namespace dragon
