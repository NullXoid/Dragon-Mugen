#include "OptionsMenuOverlay.h"

#include "DragonUi.h"
#include "UiMenuList.h"
#include "UiRenderPrimitives.h"

#include <SDL3/SDL_render.h>

#include <cstddef>
#include <vector>

namespace dragon {
namespace {

std::vector<UiMenuListRowView> uiMenuRows(const OptionsMenuView& view) {
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
    return rows;
}

UiMenuListView uiMenuView(const OptionsMenuView& view, const std::vector<UiMenuListRowView>& rows) {
    return UiMenuListView{
        rows,
        view.title.empty() ? "OPTIONS" : view.title,
        view.pageLabel,
        view.labelHeader.empty() ? "SETTING" : view.labelHeader,
        view.valueHeader,
        view.padSummary,
        view.footer,
    };
}

UiMenuListStyle optionsMenuStyle(const UiRenderContext& ui) {
    const DragonUiMetrics metrics = dragonUiMetricsForContext(ui);
    const SDL_FRect safe = dragonPixelUiSafeArea(CanvasDimensions{ ui.logicalWidth, ui.logicalHeight });
    const float s = metrics.pixelScale;
    return UiMenuListStyle{
        metrics.topBarH + 16.0f * s,
        300.0f * s,
        std::min(safe.w - 20.0f * s, 430.0f * s),
        metrics.rowH,
        true,
        s,
        safe.x,
        safe.y,
        safe.w,
        safe.h,
    };
}

} // namespace

void drawOptionsMenuOverlay(const UiRenderContext& ui, const OptionsMenuView& view) {
    SDL_Renderer* renderer = ui.renderer;
    const DragonUiMetrics metrics = dragonUiMetricsForContext(ui);
    const SDL_FRect safe = dragonPixelUiSafeArea(CanvasDimensions{ ui.logicalWidth, ui.logicalHeight });
    const float centerX = safe.x + safe.w * 0.5f;
    const auto& tokens = dragonUiTokens();
    const float s = metrics.pixelScale;

    setColor(renderer, tokens.panelBase, 226);
    fillRect(renderer, safe.x, safe.y, safe.w, metrics.topBarH);
    setColor(renderer, tokens.separatorRed);
    fillRect(renderer, safe.x, safe.y + metrics.topBarH - metrics.border, safe.w, metrics.border);
    setColor(renderer, tokens.mutedGold);
    scaledDebugText(renderer, s, safe.x + 10.0f * s, safe.y + 8.0f * s, "DRAGON MUGEN CORE");
    setColor(renderer, tokens.primaryTeal);
    const std::string title = view.title.empty() ? "OPTIONS" : view.title;
    scaledDebugText(renderer, s, centerX - static_cast<float>(title.size()) * 4.0f * s, safe.y + 8.0f * s, title);

    const std::vector<UiMenuListRowView> rows = uiMenuRows(view);

    drawUiMenuList(
        ui,
        uiMenuView(view, rows),
        optionsMenuStyle(ui));
}

UiMenuListGeometrySnapshot optionsMenuGeometrySnapshot(const UiRenderContext& ui, const OptionsMenuView& view) {
    const std::vector<UiMenuListRowView> rows = uiMenuRows(view);
    return uiMenuListGeometrySnapshot(
        uiMenuView(view, rows),
        static_cast<float>(ui.logicalWidth),
        static_cast<float>(ui.logicalHeight),
        optionsMenuStyle(ui));
}

} // namespace dragon
