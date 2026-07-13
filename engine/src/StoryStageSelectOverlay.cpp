#include "StoryStageSelectOverlay.h"

#include "UiRenderPrimitives.h"

#include <SDL3/SDL_render.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <string>
#include <utility>

namespace dragon {
namespace {

constexpr float kStoryStageWidth = 640.0f;
constexpr float kStoryStageHeight = 360.0f;

void storyText(SDL_Renderer* renderer, float x, float y, const std::string& text, Uint8 r, Uint8 g, Uint8 b) {
    setColor(renderer, 3, 5, 8, 230);
    debugText(renderer, x + 1.0f, y + 1.0f, text);
    setColor(renderer, r, g, b);
    debugText(renderer, x, y, text);
}

void storyTextCentered(SDL_Renderer* renderer, float centerX, float y, const std::string& text, Uint8 r, Uint8 g, Uint8 b) {
    storyText(renderer, centerX - static_cast<float>(text.size() * 8) * 0.5f, y, text, r, g, b);
}

float debugTextWidth(const std::string& text) {
    return static_cast<float>(text.size() * 8);
}

void storyTextRight(SDL_Renderer* renderer, float rightX, float y, const std::string& text, Uint8 r, Uint8 g, Uint8 b) {
    storyText(renderer, rightX - debugTextWidth(text), y, text, r, g, b);
}

std::pair<std::string, std::string> wrapStageName(const std::string& name, int maxChars) {
    if (static_cast<int>(name.size()) <= maxChars) {
        return { name, "" };
    }
    const int breakLimit = std::min(static_cast<int>(name.size()), maxChars);
    int split = breakLimit;
    for (int i = breakLimit; i >= 1; --i) {
        if (name[static_cast<std::size_t>(i - 1)] == ' ') {
            split = i - 1;
            break;
        }
    }
    if (split < 4) {
        split = breakLimit;
    }

    std::string first = name.substr(0, static_cast<std::size_t>(split));
    std::string second = name.substr(static_cast<std::size_t>(split));
    while (!second.empty() && second.front() == ' ') {
        second.erase(second.begin());
    }
    return { fitDebugText(first, maxChars), fitDebugText(second, maxChars) };
}

void drawDashedRoad(SDL_Renderer* renderer, float x1, float x2, float y) {
    if (x2 < x1) {
        std::swap(x1, x2);
    }
    setColor(renderer, 248, 210, 80, 220);
    for (float x = x1; x < x2; x += 11.0f) {
        fillRect(renderer, x, y, std::min(6.0f, x2 - x), 2.0f);
    }
    setColor(renderer, 18, 24, 34, 180);
    fillRect(renderer, x1, y + 3.0f, x2 - x1, 1.0f);
}

void drawStageCard(
    SDL_Renderer* renderer,
    const StoryStageCardView& stage,
    int stageNumber,
    float x,
    float y,
    float w,
    float h) {
    const bool selected = stage.selected;
    const float lift = selected ? -7.0f : 0.0f;
    const float expand = selected ? 8.0f : 0.0f;
    const float sx = x - expand * 0.5f;
    const float sy = y + lift;
    const float sw = w + expand;
    const float sh = selected ? h + 10.0f : h;

    setColor(renderer, 4, 7, 13, selected ? 214 : 180);
    fillRect(renderer, sx + 5.0f, sy + 6.0f, sw, sh);
    setColor(renderer, selected ? 38 : 24, selected ? 48 : 32, selected ? 64 : 44, selected ? 230 : 190);
    fillRect(renderer, sx, sy, sw, sh);
    setColor(renderer, selected ? 252 : 136, selected ? 216 : 154, selected ? 88 : 184, selected ? 255 : 210);
    drawRect(renderer, sx, sy, sw, sh);
    if (selected) {
        drawRect(renderer, sx + 2.0f, sy + 2.0f, sw - 4.0f, sh - 4.0f);
    }

    if (stage.shop) {
        setColor(renderer, 122, 77, 216, 220);
    } else if (stage.boss) {
        setColor(renderer, 231, 195, 90, 220);
    } else {
        setColor(renderer, stage.scrolling ? 40 : 142, stage.scrolling ? 192 : 96, stage.scrolling ? 174 : 172, 220);
    }
    fillRect(renderer, sx + 4.0f, sy + 4.0f, sw - 8.0f, 7.0f);
    const int nameChars = std::max(7, static_cast<int>((sw - 14.0f) / 8.0f));
    const auto [nameLine1, nameLine2] = wrapStageName(stage.name, nameChars);
    storyText(renderer, sx + 7.0f, sy + 15.0f, "BOARD " + std::to_string(stageNumber), 246, 226, 112);
    storyText(renderer, sx + 7.0f, sy + 27.0f, nameLine1, 220, 232, 242);
    if (!nameLine2.empty()) {
        storyText(renderer, sx + 7.0f, sy + 39.0f, nameLine2, 220, 232, 242);
    }
    if (selected) {
        storyText(renderer, sx + 7.0f, sy + 51.0f, fitDebugText(stage.kindLabel.empty() ? "STORY NODE" : stage.kindLabel, nameChars), 120, 226, 218);
    }
}

int firstVisibleStage(int selectedIndex, int stageCount, int visibleCount) {
    if (stageCount <= visibleCount) {
        return 0;
    }
    return std::clamp(selectedIndex - visibleCount / 2, 0, stageCount - visibleCount);
}

void drawStoryStageSelect(const UiRenderContext& ui, const StoryStageSelectView& view) {
    SDL_Renderer* renderer = ui.renderer;
    ScopedVirtualCanvas virtualCanvas(ui, kStoryStageWidth, kStoryStageHeight);
    constexpr float widthF = kStoryStageWidth;
    constexpr float heightF = kStoryStageHeight;
    const float centerX = widthF * 0.5f;

    setColor(renderer, 4, 6, 10, 96);
    fillRect(renderer, 0.0f, 0.0f, widthF, heightF);
    setColor(renderer, 7, 16, 25, 224);
    fillRect(renderer, 0.0f, 0.0f, widthF, 38.0f);
    setColor(renderer, 198, 79, 85, 230);
    fillRect(renderer, 0.0f, 38.0f, widthF, 2.0f);

    storyText(renderer, 20.0f, 14.0f, fitDebugText(view.routeTitle.empty() ? "STORY MAP" : view.routeTitle, 23), 246, 226, 112);
    storyTextCentered(renderer, centerX, 14.0f, "STORY BOARD", 81, 210, 198);
    if (!view.fighterLabel.empty()) {
        storyTextRight(renderer, widthF - 20.0f, 14.0f, fitDebugText(view.fighterLabel, 18), 220, 232, 242);
    }

    if (view.stages.empty()) {
        setColor(renderer, 7, 16, 25, 236);
        fillRect(renderer, centerX - 142.0f, 130.0f, 284.0f, 84.0f);
        setColor(renderer, 81, 210, 198, 230);
        drawRect(renderer, centerX - 142.0f, 130.0f, 284.0f, 84.0f);
        storyTextCentered(renderer, centerX, 158.0f, "NO STORY BOARDS", 246, 126, 116);
        storyTextCentered(renderer, centerX, 176.0f, "CHECK story_boards.def", 220, 232, 242);
        return;
    }

    const int stageCount = static_cast<int>(view.stages.size());
    const int visibleCount = std::min(stageCount, 5);
    const int first = firstVisibleStage(std::clamp(view.selectedIndex, 0, stageCount - 1), stageCount, visibleCount);
    const int last = first + visibleCount;
    const float cardW = visibleCount >= 5 ? 104.0f : 112.0f;
    const float cardH = 80.0f;
    const float cardGap = 12.0f;
    const float rowW = cardW * static_cast<float>(visibleCount) + cardGap * static_cast<float>(visibleCount - 1);
    const float rowX = centerX - rowW * 0.5f;
    const float rowY = 72.0f;
    const float roadY = rowY + cardH + 18.0f;

    storyTextCentered(renderer, centerX, 51.0f, "CHOOSE A ROUTE NODE", 150, 210, 252);

    for (int visible = 0; visible + 1 < visibleCount; ++visible) {
        const float x1 = rowX + static_cast<float>(visible) * (cardW + cardGap) + cardW + 2.0f;
        const float x2 = rowX + static_cast<float>(visible + 1) * (cardW + cardGap) - 2.0f;
        drawDashedRoad(renderer, x1, x2, roadY);
    }

    for (int i = first; i < last; ++i) {
        const int visible = i - first;
        const float x = rowX + static_cast<float>(visible) * (cardW + cardGap);
        drawStageCard(renderer, view.stages[static_cast<std::size_t>(i)], i + 1, x, rowY, cardW, cardH);
    }

    const float panelX = 54.0f;
    const float panelY = 197.0f;
    const float panelW = widthF - panelX * 2.0f;
    const float panelH = 102.0f;
    setColor(renderer, 5, 8, 15, 220);
    fillRect(renderer, panelX, panelY, panelW, panelH);
    setColor(renderer, 15, 28, 42, 218);
    fillRect(renderer, panelX, panelY, panelW, 24.0f);
    setColor(renderer, 81, 210, 198, 230);
    drawRect(renderer, panelX, panelY, panelW, panelH);
    setColor(renderer, 198, 79, 85, 230);
    fillRect(renderer, panelX + 6.0f, panelY + 24.0f, panelW - 12.0f, 2.0f);
    setColor(renderer, 231, 195, 90, 220);
    fillRect(renderer, panelX + 10.0f, panelY + 70.0f, panelW - 20.0f, 1.0f);

    const int selectedOneBased = std::clamp(view.selectedIndex + 1, 1, stageCount);
    storyText(renderer, panelX + 14.0f, panelY + 8.0f, "EPISODE " + std::to_string(selectedOneBased) + "/" + std::to_string(stageCount), 231, 195, 90);
    storyTextRight(renderer, panelX + panelW - 14.0f, panelY + 8.0f, "DIFFICULTY " + fitDebugText(view.difficultyLabel, 8), 231, 195, 90);

    storyText(renderer, panelX + 14.0f, panelY + 36.0f, fitDebugText(view.selectedStageName, 36), 81, 210, 198);
    storyTextRight(renderer, panelX + panelW - 14.0f, panelY + 36.0f, "WAVES " + std::to_string(std::max(1, view.waveCount)), 220, 232, 242);
    storyText(renderer, panelX + 14.0f, panelY + 54.0f, "TYPE " + fitDebugText(view.selectedNodeKind.empty() ? "NODE" : view.selectedNodeKind, 18), 196, 206, 220);
    const std::string target = !view.selectedNodeTarget.empty() ? view.selectedNodeTarget : view.selectedStageAuthor;
    storyText(renderer, panelX + 230.0f, panelY + 54.0f, "TARGET " + fitDebugText(target, 26), 196, 206, 220);
    storyTextCentered(renderer, centerX, panelY + 79.0f, "ENTER START BOARD", 120, 226, 218);

    setColor(renderer, 6, 8, 12, 214);
    fillRect(renderer, 18.0f, heightF - 29.0f, widthF - 36.0f, 20.0f);
    setColor(renderer, 81, 210, 198, 210);
    drawRect(renderer, 18.0f, heightF - 29.0f, widthF - 36.0f, 20.0f);
    storyText(renderer, 28.0f, heightF - 24.0f, "L/R BOARD", 220, 232, 242);
    storyTextCentered(renderer, centerX, heightF - 24.0f, "UP/DOWN DIFFICULTY", 220, 232, 242);
    storyTextRight(renderer, widthF - 28.0f, heightF - 24.0f, "ESC BACK", 220, 232, 242);
}

} // namespace

void drawStoryStageSelectOverlay(const UiRenderContext& ui, const StoryStageSelectView& view) {
    drawStoryStageSelect(ui, view);
}

} // namespace dragon
