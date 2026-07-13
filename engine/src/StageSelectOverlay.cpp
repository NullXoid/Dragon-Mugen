#include "StageSelectOverlay.h"

#include "UiRenderPrimitives.h"

#include <SDL3/SDL_render.h>

#include <algorithm>
#include <cstddef>
#include <string>

namespace dragon {
namespace {

constexpr float kStageSelectWidth = 640.0f;
constexpr float kStageSelectHeight = 360.0f;

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

float debugTextWidth(const std::string& text) {
    return static_cast<float>(text.size() * 8);
}

void shadowTextRight(SDL_Renderer* renderer, float rightX, float y, const std::string& text, Uint8 r, Uint8 g, Uint8 b) {
    shadowText(renderer, rightX - debugTextWidth(text), y, text, r, g, b);
}

void drawTopBar(SDL_Renderer* renderer, const StageSelectView& view) {
    constexpr float centerX = kStageSelectWidth * 0.5f;
    constexpr float barH = 38.0f;
    constexpr float titleY = 14.0f;

    setColor(renderer, 7, 16, 25, 220);
    fillRect(renderer, 0.0f, 0.0f, kStageSelectWidth, barH);
    setColor(renderer, 198, 79, 85, 230);
    fillRect(renderer, 0.0f, barH, kStageSelectWidth, 2.0f);

    shadowText(renderer, 20.0f, titleY, "DRAGON MUGEN CORE", 246, 226, 112);
    shadowTextCentered(renderer, centerX, titleY, fitDebugText(view.modeLabel, 26), 81, 210, 198);
    if (!view.fighterLabel.empty()) {
        shadowTextRight(renderer, kStageSelectWidth - 20.0f, titleY, fitDebugText(view.fighterLabel, 18), 220, 232, 242);
    }
}

void drawStageRows(SDL_Renderer* renderer, const StageSelectView& view) {
    if (view.rows.size() <= 1) {
        return;
    }

    const int rowCount = static_cast<int>(std::min<std::size_t>(view.rows.size(), 5));
    int selected = 0;
    for (int i = 0; i < static_cast<int>(view.rows.size()); ++i) {
        if (view.rows[static_cast<std::size_t>(i)].selected) {
            selected = i;
            break;
        }
    }
    const int first = std::clamp(selected - rowCount / 2, 0, std::max(0, static_cast<int>(view.rows.size()) - rowCount));

    constexpr float centerX = kStageSelectWidth * 0.5f;
    constexpr float rowW = 92.0f;
    constexpr float rowH = 26.0f;
    constexpr float gap = 10.0f;
    const float totalW = rowW * static_cast<float>(rowCount) + gap * static_cast<float>(std::max(0, rowCount - 1));
    const float startX = centerX - totalW * 0.5f;
    constexpr float y = 62.0f;

    for (int visible = 0; visible < rowCount; ++visible) {
        const int index = first + visible;
        const auto& row = view.rows[static_cast<std::size_t>(index)];
        const float x = startX + static_cast<float>(visible) * (rowW + gap);
        setColor(renderer, row.selected ? 81 : 13, row.selected ? 210 : 23, row.selected ? 198 : 35, row.selected ? 224 : 198);
        fillRect(renderer, x, y, rowW, rowH);
        setColor(renderer, row.selected ? 231 : 81, row.selected ? 195 : 120, row.selected ? 90 : 132, row.selected ? 230 : 190);
        drawRect(renderer, x, y, rowW, rowH);
        const std::string label = fitDebugText(row.label, std::max(6, static_cast<int>((rowW - 10.0f) / 8.0f)));
        if (row.selected) {
            shadowTextCentered(renderer, x + rowW * 0.5f, y + 7.0f, label, 7, 16, 25);
        } else {
            shadowTextCentered(renderer, x + rowW * 0.5f, y + 7.0f, label, 220, 232, 242);
        }
    }
}

void drawStagePanel(SDL_Renderer* renderer, const StageSelectView& view) {
    constexpr float centerX = kStageSelectWidth * 0.5f;
    constexpr float panelW = 492.0f;
    constexpr float panelH = 88.0f;
    const float panelX = centerX - panelW * 0.5f;
    constexpr float panelY = kStageSelectHeight - panelH - 40.0f;
    const bool showArrows = view.rows.size() > 1;
    const int stageChars = std::max(10, static_cast<int>((panelW - 36.0f) / 8.0f));
    const std::string stageName = fitDebugText(view.selectedStageName, static_cast<std::size_t>(stageChars));

    setColor(renderer, 5, 8, 15, 220);
    fillRect(renderer, panelX, panelY, panelW, panelH);
    setColor(renderer, 15, 28, 42, 218);
    fillRect(renderer, panelX, panelY, panelW, 24.0f);
    setColor(renderer, 81, 210, 198, 230);
    drawRect(renderer, panelX, panelY, panelW, panelH);
    setColor(renderer, 198, 79, 85, 230);
    fillRect(renderer, panelX + 6.0f, panelY + 24.0f, panelW - 12.0f, 2.0f);

    shadowText(renderer, panelX + 12.0f, panelY + 8.0f, "STAGE SELECT", 246, 226, 112);
    if (!view.opponentLabel.empty()) {
        shadowTextRight(renderer, panelX + panelW - 12.0f, panelY + 8.0f, fitDebugText(view.opponentLabel, 16), 196, 206, 220);
    }

    shadowTextCentered(
        renderer,
        centerX,
        panelY + 38.0f,
        showArrows ? "< " + stageName + " >" : stageName,
        81,
        210,
        198);

    const std::string meta = "ID " + fitDebugText(view.selectedStageId.empty() ? "stage" : view.selectedStageId, 18)
        + "   AUTHOR " + fitDebugText(view.selectedStageAuthor.empty() ? "unknown" : view.selectedStageAuthor, 18);
    shadowTextCentered(renderer, centerX, panelY + 57.0f, fitDebugText(meta, 54), 196, 206, 220);
    shadowTextCentered(renderer, centerX, panelY + 72.0f, "LEFT/RIGHT STAGE   ENTER START   ESC FIGHTER SELECT", 220, 232, 242);
}

} // namespace

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
    }

    ScopedVirtualCanvas virtualCanvas(ui, kStageSelectWidth, kStageSelectHeight);
    constexpr float centerX = kStageSelectWidth * 0.5f;

    if (view.rows.empty()) {
        drawTopBar(renderer, view);
        shadowTextCentered(renderer, centerX, kStageSelectHeight * 0.44f, "NO STAGES FOUND", 230, 130, 120);
        shadowTextCentered(renderer, centerX, kStageSelectHeight * 0.44f + 18.0f, "CHECK game/stages", 210, 218, 230);
        return;
    }

    setColor(renderer, 4, 6, 10, 58);
    fillRect(renderer, 0.0f, 0.0f, kStageSelectWidth, kStageSelectHeight);
    drawTopBar(renderer, view);
    drawStageRows(renderer, view);
    drawStagePanel(renderer, view);
}

} // namespace dragon
