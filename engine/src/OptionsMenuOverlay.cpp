#include "OptionsMenuOverlay.h"

#include "UiMenuList.h"
#include "UiRenderPrimitives.h"

#include <SDL3/SDL_render.h>

#include <cstddef>
#include <vector>

namespace dragon {

void drawOptionsMenuOverlay(const UiRenderContext& ui, const OptionsMenuView& view) {
    SDL_Renderer* renderer = ui.renderer;
    const float centerX = static_cast<float>(ui.logicalWidth) * 0.5f;

    setColor(renderer, 238, 238, 244);
    debugTextCentered(renderer, centerX, 28, "DRAGON MUGEN CORE");
    setColor(renderer, 246, 214, 92);
    debugTextCentered(renderer, centerX, 46, "OPTIONS");

    std::vector<UiMenuListRowView> rows;
    rows.reserve(view.rows.size());
    for (const auto& row : view.rows) {
        rows.push_back(UiMenuListRowView{
            row.label,
            row.value,
            row.selected,
            row.adjustable,
            false,
        });
    }

    drawUiMenuList(
        ui,
        UiMenuListView{
            rows,
            "OPTIONS",
            "",
            "SETTING",
            "VALUE",
            view.padSummary,
            "UP/DOWN SEL  LEFT/RIGHT CHANGE  ENTER  ESC",
        },
        UiMenuListStyle{
            64.0f,
            320.0f,
            430.0f,
            12.0f,
            true,
        });
}

} // namespace dragon
