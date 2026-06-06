#include "TrainingOptionsOverlay.h"

#include "UiRenderPrimitives.h"

#include <cstddef>

namespace dragon {

void drawTrainingMoveListPage(const UiRenderContext& ui, const TrainingMoveListView& view) {
    SDL_Renderer* renderer = ui.renderer;
    ScopedUiScale scaledUi(ui, 320.0f, 240.0f);

    setColor(renderer, 6, 8, 14, 244);
    fillRect(renderer, 10, 16, 300, 216);
    setColor(renderer, 54, 70, 98);
    drawRect(renderer, 10, 16, 300, 216);

    setColor(renderer, 28, 42, 74);
    fillRect(renderer, 12, 18, 296, 24);
    setColor(renderer, 158, 64, 58);
    fillRect(renderer, 12, 42, 296, 3);
    setColor(renderer, 230, 190, 105);
    fillRect(renderer, 18, 48, 282, 1);

    setColor(renderer, 230, 220, 172);
    debugText(renderer, 20, 24, "COMMAND TRAINING");
    setColor(renderer, 128, 171, 225);
    debugText(renderer, 184, 24, view.selectedCharacterLabel);
    setColor(renderer, 174, 184, 196);
    debugText(renderer, 20, 38, view.categoryLabel);

    setColor(renderer, 16, 20, 28, 232);
    fillRect(renderer, 18, 54, 150, 116);
    setColor(renderer, 78, 94, 124);
    drawRect(renderer, 18, 54, 150, 116);
    setColor(renderer, 208, 168, 92);
    debugText(renderer, 26, 62, "TECHNIQUE");

    if (view.empty) {
        setColor(renderer, 184, 178, 168);
        debugText(renderer, 26, 84, "No loaded moves");
    } else {
        for (int row = 0; row < static_cast<int>(view.rows.size()); ++row) {
            const auto& entry = view.rows[static_cast<std::size_t>(row)];
            const float y = 78.0f + static_cast<float>(row * 13);
            if (entry.selected) {
                setColor(renderer, 74, 170, 134);
                fillRect(renderer, 24, y - 3.0f, 136, 12);
                setColor(renderer, 8, 12, 16);
            } else {
                setColor(renderer, row % 2 == 0 ? 174 : 138, row % 2 == 0 ? 184 : 150, row % 2 == 0 ? 196 : 164);
            }
            debugText(renderer, 28, y, entry.number);
            debugText(renderer, 48, y, entry.label);
        }
    }

    setColor(renderer, 16, 20, 28, 232);
    fillRect(renderer, 176, 54, 124, 116);
    setColor(renderer, 78, 94, 124);
    drawRect(renderer, 176, 54, 124, 116);
    setColor(renderer, 208, 168, 92);
    debugText(renderer, 184, 62, "DETAIL");

    if (view.detail.visible) {
        setColor(renderer, 222, 226, 232);
        debugText(renderer, 184, 82, view.detail.name);
        setColor(renderer, 128, 171, 225);
        debugText(renderer, 184, 100, "STATE");
        setColor(renderer, 230, 190, 105);
        debugText(renderer, 236, 100, view.detail.state);
        setColor(renderer, 128, 171, 225);
        debugText(renderer, 184, 116, "INPUT");
        setColor(renderer, 222, 226, 232);
        debugText(renderer, 184, 130, view.detail.input);
        setColor(renderer, 128, 171, 225);
        debugText(renderer, 184, 146, "TYPE");
        setColor(renderer, 222, 226, 232);
        debugText(renderer, 236, 146, view.detail.type);
        setColor(renderer, 116, 190, 154);
        debugText(renderer, 184, 158, view.detail.powerStatus);
    }

    setColor(renderer, 34, 26, 30, 238);
    fillRect(renderer, 18, 176, 282, 32);
    setColor(renderer, 158, 64, 58);
    drawRect(renderer, 18, 176, 282, 32);
    setColor(renderer, 230, 220, 172);
    if (view.detail.visible) {
        debugText(renderer, 28, 184, "PERFORM");
        setColor(renderer, 222, 226, 232);
        debugText(renderer, 90, 184, view.detail.performInput);
    } else {
        debugText(renderer, 28, 184, "No executable commands loaded");
    }

    setColor(renderer, 130, 142, 156);
    debugText(renderer, 20, 216, "UP/DOWN technique  ESC back");
    debugText(renderer, 266, 216, view.pageLabel);
}

void drawTrainingOptionsMenu(const UiRenderContext& ui, const TrainingOptionsMenuView& view) {
    SDL_Renderer* renderer = ui.renderer;
    ScopedUiScale scaledUi(ui, 320.0f, 240.0f);

    setColor(renderer, 8, 10, 12, 236);
    fillRect(renderer, 34, 24, 286, 202);
    setColor(renderer, 92, 104, 124);
    drawRect(renderer, 34, 24, 286, 202);

    setColor(renderer, 210, 224, 238);
    debugText(renderer, 46, 34, "TRAINING OPTIONS");

    constexpr int rowsPerColumn = 10;
    for (int i = 0; i < static_cast<int>(view.rows.size()); ++i) {
        const int column = i / rowsPerColumn;
        const int row = i % rowsPerColumn;
        const float x = column == 0 ? 48.0f : 176.0f;
        const float y = 52.0f + static_cast<float>(row * 14);
        const auto& option = view.rows[static_cast<std::size_t>(i)];
        if (option.selected) {
            setColor(renderer, 74, 170, 134);
            fillRect(renderer, x - 6.0f, y - 3.0f, 116.0f, 12.0f);
            setColor(renderer, 8, 12, 16);
        } else {
            setColor(renderer, 172, 182, 194);
        }
        debugText(renderer, x, y, option.label);
        debugText(renderer, x + 86.0f, y, option.status);
    }

    setColor(renderer, 130, 142, 156);
    debugText(renderer, 48, 204, "ENTER toggle");
    debugText(renderer, 176, 204, "RESET POS runs now");
    debugText(renderer, 48, 216, "F2/ESC close");
}

} // namespace dragon
