#include "OptionsMenuOverlay.h"

#include "UiRenderPrimitives.h"

#include <SDL3/SDL_render.h>

#include <cstddef>

namespace dragon {

void drawOptionsMenuOverlay(const UiRenderContext& ui, const OptionsMenuView& view) {
    SDL_Renderer* renderer = ui.renderer;
    ScopedUiScale scaledUi(ui, 320.0f, 240.0f);

    constexpr float centerX = 160.0f;
    constexpr float menuTop = 76.0f;
    constexpr float rowSpacing = 13.0f;
    constexpr float labelX = 62.0f;
    constexpr float valueX = 204.0f;

    setColor(renderer, 238, 238, 244);
    debugTextCentered(renderer, centerX, 28, "DRAGON MUGEN CORE");
    setColor(renderer, 246, 214, 92);
    debugTextCentered(renderer, centerX, 46, "OPTIONS");

    setColor(renderer, 0, 0, 0, 178);
    fillRect(renderer, 36, menuTop - 14.0f, 248, 112);
    setColor(renderer, 78, 88, 108);
    drawRect(renderer, 36, menuTop - 14.0f, 248, 112);

    for (int i = 0; i < static_cast<int>(view.rows.size()); ++i) {
        const auto& row = view.rows[static_cast<std::size_t>(i)];
        const float y = menuTop + static_cast<float>(i) * rowSpacing;
        if (row.selected) {
            setColor(renderer, 238, 222, 92);
            drawRect(renderer, 48, y - 9.0f, 224, 13);
            setColor(renderer, 246, 214, 92);
        } else {
            setColor(renderer, 222, 226, 232);
        }

        debugText(renderer, labelX, y - 6.0f, row.label);
        if (row.adjustable) {
            setColor(renderer, row.selected ? 246 : 172, row.selected ? 214 : 182, row.selected ? 92 : 194);
            debugText(renderer, valueX - 12.0f, y - 6.0f, "<");
            debugText(renderer, valueX, y - 6.0f, row.value);
            debugText(renderer, valueX + 8.0f + static_cast<float>(row.value.size()) * 8.0f, y - 6.0f, ">");
        }
    }

    setColor(renderer, 0, 0, 0, 170);
    fillRect(renderer, 0, 204, 320, 24);

    setColor(renderer, 172, 182, 194);
    debugTextCentered(renderer, centerX, 208, view.padSummary);
    debugTextCentered(renderer, centerX, 220, "UP/DOWN   LEFT/RIGHT   ENTER   ESC");
}

} // namespace dragon
