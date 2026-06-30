#include "OptionsMenuOverlay.h"

#include "DragonUi.h"
#include "UiMenuList.h"
#include "UiRenderPrimitives.h"

#include <SDL3/SDL_render.h>

#include <cstddef>
#include <vector>

namespace dragon {

void drawOptionsMenuOverlay(const UiRenderContext& ui, const OptionsMenuView& view) {
    SDL_Renderer* renderer = ui.renderer;
    const float centerX = static_cast<float>(ui.logicalWidth) * 0.5f;
    const DragonUiMetrics metrics = dragonUiMetricsForContext(ui);
    const auto& tokens = dragonUiTokens();
    const float s = metrics.pixelScale;

    setColor(renderer, tokens.panelBase, 226);
    fillRect(renderer, 0.0f, 0.0f, static_cast<float>(ui.logicalWidth), metrics.topBarH);
    setColor(renderer, tokens.separatorRed);
    fillRect(renderer, 0.0f, metrics.topBarH - metrics.border, static_cast<float>(ui.logicalWidth), metrics.border);
    setColor(renderer, tokens.mutedGold);
    scaledDebugText(renderer, s, 10.0f * s, 8.0f * s, "DRAGON MUGEN CORE");
    setColor(renderer, tokens.primaryTeal);
    const std::string title = view.title.empty() ? "OPTIONS" : view.title;
    scaledDebugText(renderer, s, centerX - static_cast<float>(title.size()) * 4.0f * s, 8.0f * s, title);

    std::vector<UiMenuListRowView> rows;
    rows.reserve(view.rows.size());
    for (const auto& row : view.rows) {
        rows.push_back(UiMenuListRowView{
            row.label,
            row.value,
            row.selected,
            row.adjustable,
            row.disabled,
        });
    }

    drawUiMenuList(
        ui,
        UiMenuListView{
            rows,
            view.title.empty() ? "OPTIONS" : view.title,
            view.pageLabel,
            view.labelHeader.empty() ? "SETTING" : view.labelHeader,
            view.valueHeader,
            view.padSummary,
            view.footer,
        },
        UiMenuListStyle{
            metrics.topBarH + 16.0f * s,
            300.0f * s,
            std::min(static_cast<float>(ui.logicalWidth) - 20.0f * s, 430.0f * s),
            metrics.rowH,
            true,
            s,
        });
}

} // namespace dragon
