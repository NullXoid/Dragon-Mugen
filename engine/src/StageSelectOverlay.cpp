#include "StageSelectOverlay.h"

#include "UiRenderPrimitives.h"

#include <SDL3/SDL_render.h>

#include <cstddef>

namespace dragon {

void drawStageSelectOverlay(const UiRenderContext& ui, const StageSelectView& view) {
    SDL_Renderer* renderer = ui.renderer;
    const float widthF = static_cast<float>(ui.logicalWidth);
    const float heightF = static_cast<float>(ui.logicalHeight);

    setColor(renderer, 10, 12, 16);
    SDL_RenderClear(renderer);

    setColor(renderer, 36, 34, 30);
    fillRect(renderer, 0, 0, widthF, heightF);
    setColor(renderer, 24, 30, 38);
    fillRect(renderer, 0, 176, widthF, 64);
    setColor(renderer, 94, 78, 54);
    fillRect(renderer, 0, 174, widthF, 1);

    setColor(renderer, 210, 224, 238);
    debugText(renderer, 18, 16, "DRAGON MUGEN CORE");
    setColor(renderer, 220, 178, 112);
    debugText(renderer, 20, 30, "STAGE SELECT");
    setColor(renderer, 155, 164, 174);
    debugText(renderer, 210, 30, fitDebugText(view.modeLabel, 15));

    drawPanel(renderer, 18, 52, 284, 108);

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
                fillRect(renderer, 28, y - 3, 140, 15);
                setColor(renderer, 8, 12, 16);
                debugText(renderer, 36, y, fitDebugText(row.label, 15));
            } else {
                setColor(renderer, 184, 178, 168);
                debugText(renderer, 36, y, fitDebugText(row.label, 15));
            }
        }

        setColor(renderer, 222, 226, 232);
        debugText(renderer, 182, 68, fitDebugText(view.selectedStageName, 15));
        setColor(renderer, 155, 164, 174);
        debugText(renderer, 182, 84, fitDebugText("id: " + view.selectedStageId, 16));
        debugText(renderer, 182, 96, fitDebugText("author: " + view.selectedStageAuthor, 16));
        debugText(renderer, 182, 116, fitDebugText("fighter: " + view.fighterLabel, 16));
        debugText(renderer, 182, 132, fitDebugText("opponent: " + view.opponentLabel, 16));
    }

    setColor(renderer, 118, 126, 138);
    debugText(renderer, 20, 204, "UP/DOWN choose  ENTER start  ESC fighter select");
}

} // namespace dragon
