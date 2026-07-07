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

void drawComicCorners(SDL_Renderer* renderer, float x, float y, float w, float h, SDL_Color color) {
    setColor(renderer, color.r, color.g, color.b, color.a);
    fillRect(renderer, x, y, 14.0f, 2.0f);
    fillRect(renderer, x, y, 2.0f, 10.0f);
    fillRect(renderer, x + w - 14.0f, y + h - 2.0f, 14.0f, 2.0f);
    fillRect(renderer, x + w - 2.0f, y + h - 10.0f, 2.0f, 10.0f);
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

} // namespace

void drawStoryStageSelectOverlay(const UiRenderContext& ui, const StoryStageSelectView& view) {
    SDL_Renderer* renderer = ui.renderer;
    const float virtualWidth = ui.logicalWidth <= 340 ? 320.0f : 426.0f;
    ScopedVirtualCanvas virtualCanvas(ui, virtualWidth, 240.0f);
    const float widthF = virtualWidth;
    const float heightF = 240.0f;
    const float centerX = widthF * 0.5f;

    setColor(renderer, 4, 6, 10, 112);
    fillRect(renderer, 0, 0, widthF, heightF);
    setColor(renderer, 8, 12, 20, 180);
    fillRect(renderer, 0, 0, widthF, 27.0f);
    setColor(renderer, 224, 64, 86, 210);
    fillRect(renderer, 0, 27.0f, widthF, 2.0f);

    storyText(renderer, 14.0f, 8.0f, fitDebugText(view.routeTitle.empty() ? "STORY MAP" : view.routeTitle, widthF < 360.0f ? 11 : 15), 246, 226, 112);
    storyTextCentered(renderer, centerX, 8.0f, "SELECT A BOARD", 150, 210, 252);
    const float fighterRight = widthF - 66.0f;
    const int fighterChars = std::clamp(static_cast<int>((fighterRight - centerX - 62.0f) / 8.0f), 0, 8);
    if (fighterChars > 0) {
        storyTextRight(renderer, fighterRight, 8.0f, fitDebugText(view.fighterLabel, fighterChars), 220, 232, 242);
    }

    if (view.stages.empty()) {
        storyTextCentered(renderer, centerX, 96.0f, "NO STORY BOARDS", 246, 126, 116);
        storyTextCentered(renderer, centerX, 112.0f, "CHECK story_boards.def", 220, 232, 242);
        return;
    }

    const int stageCount = static_cast<int>(view.stages.size());
    const int visibleCount = std::min(stageCount, widthF < 360.0f ? 3 : 5);
    const int first = firstVisibleStage(std::clamp(view.selectedIndex, 0, stageCount - 1), stageCount, visibleCount);
    const int last = first + visibleCount;
    const float margin = widthF < 360.0f ? 18.0f : 26.0f;
    const float cardGap = widthF < 360.0f ? 12.0f : 16.0f;
    const float maxCardW = visibleCount <= 3 ? 118.0f : 92.0f;
    const float cardW = std::clamp((widthF - margin * 2.0f - cardGap * static_cast<float>(visibleCount - 1))
            / static_cast<float>(std::max(1, visibleCount)),
        64.0f,
        maxCardW);
    const float cardH = widthF < 360.0f ? 53.0f : 58.0f;
    const float rowW = cardW * static_cast<float>(visibleCount) + cardGap * static_cast<float>(visibleCount - 1);
    const float rowX = centerX - rowW * 0.5f;
    const float rowY = 50.0f;
    const float roadY = rowY + cardH + 17.0f;

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

    const float panelX = std::max(18.0f, centerX - 184.0f);
    const float panelW = std::min(widthF - panelX * 2.0f, 368.0f);
    const float panelY = heightF - 78.0f;
    const float panelH = 53.0f;
    setColor(renderer, 5, 8, 15, 200);
    fillRect(renderer, panelX, panelY, panelW, panelH);
    setColor(renderer, 26, 42, 60, 210);
    fillRect(renderer, panelX, panelY, panelW, 12.0f);
    setColor(renderer, 54, 188, 202, 230);
    fillRect(renderer, panelX, panelY, panelW, 1.0f);
    setColor(renderer, 248, 210, 80, 210);
    fillRect(renderer, panelX + 10.0f, panelY + 24.0f, panelW - 20.0f, 1.0f);
    drawComicCorners(renderer, panelX, panelY, panelW, panelH, SDL_Color{ 56, 188, 202, 210 });

    const int selectedOneBased = std::clamp(view.selectedIndex + 1, 1, stageCount);
    storyText(renderer, panelX + 12.0f, panelY + 4.0f, "EPISODE " + std::to_string(selectedOneBased) + "/" + std::to_string(stageCount), 246, 226, 112);
    storyText(renderer, panelX + 102.0f, panelY + 4.0f, fitDebugText(view.selectedStageName, 24), 150, 226, 252);
    const bool compactDetails = widthF < 360.0f;
    storyText(
        renderer,
        panelX + 12.0f,
        panelY + 30.0f,
        fitDebugText(view.selectedNodeKind.empty() ? "NODE" : view.selectedNodeKind, compactDetails ? 9 : 11),
        120,
        226,
        218);
    storyText(renderer, panelX + (compactDetails ? 100.0f : 112.0f), panelY + 30.0f, "WAVES " + std::to_string(std::max(1, view.waveCount)), 220, 232, 242);
    storyText(renderer, panelX + (compactDetails ? 170.0f : 190.0f), panelY + 30.0f, "DIFF " + fitDebugText(view.difficultyLabel, 4), 246, 226, 112);
    if (!view.selectedNodeTarget.empty()) {
        storyTextRight(renderer, panelX + panelW - 12.0f, panelY + 30.0f, fitDebugText(view.selectedNodeTarget, 12), 196, 206, 220);
    } else {
        storyTextRight(renderer, panelX + panelW - 12.0f, panelY + 30.0f, fitDebugText(view.selectedStageAuthor, 12), 196, 206, 220);
    }

    setColor(renderer, 6, 8, 12, 190);
    fillRect(renderer, 0, heightF - 15.0f, widthF, 15.0f);
    storyTextCentered(renderer, centerX, heightF - 12.0f, "L/R BOARD  UP/DN DIFF  ENT START  ESC BACK", 220, 232, 242);
}

} // namespace dragon
