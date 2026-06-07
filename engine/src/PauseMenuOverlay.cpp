#include "PauseMenuOverlay.h"

#include "UiRenderPrimitives.h"

#include <algorithm>
#include <string>

namespace dragon {

void drawSingleFightPauseMenu(const UiRenderContext& ui, const PauseMenuView& view) {
    SDL_Renderer* renderer = ui.renderer;
    ScopedUiScale scaledUi(ui, 320.0f, 240.0f);

    setColor(renderer, 6, 8, 14, 238);
    fillRect(renderer, 68, 42, 184, 156);
    setColor(renderer, 72, 82, 104);
    drawRect(renderer, 68, 42, 184, 156);
    setColor(renderer, 30, 38, 54);
    fillRect(renderer, 70, 44, 180, 28);
    setColor(renderer, 158, 64, 58);
    fillRect(renderer, 70, 72, 180, 2);

    setColor(renderer, 230, 220, 172);
    debugText(renderer, 84, 52, std::string(view.modeLabel));
    setColor(renderer, 128, 171, 225);
    debugText(renderer, 204, 52, "PAUSE");

    const int selectedOption =
        std::clamp(view.selectedOption, 0, static_cast<int>(view.optionLabels.size()) - 1);
    for (int i = 0; i < static_cast<int>(view.optionLabels.size()); ++i) {
        const float y = 88.0f + static_cast<float>(i * 18);
        if (i == selectedOption) {
            setColor(renderer, 74, 170, 134);
            fillRect(renderer, 84, y - 3.0f, 132, 14);
            setColor(renderer, 8, 12, 16);
        } else {
            setColor(renderer, 174, 184, 196);
        }
        debugText(renderer, 94, y, std::string(view.optionLabels[static_cast<size_t>(i)]));
    }

    setColor(renderer, 130, 142, 156);
    debugText(renderer, 86, 178, "ENTER select  ESC resume");
}

} // namespace dragon
