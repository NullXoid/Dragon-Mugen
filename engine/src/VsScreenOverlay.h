#pragma once

#include "UiRenderContext.h"

#include <SDL3/SDL_render.h>

#include <string>

namespace dragon {

enum class VsScreenLoadStatus {
    Loading,
    Ready,
    Failed,
};

struct VsPortraitView {
    SDL_Texture* texture = nullptr;
    int width = 0;
    int height = 0;
};

struct VsScreenView {
    std::string modeTitle;
    std::string p1Name;
    std::string opponentName;
    std::string opponentSlotLabel;
    std::string stageName;
    VsScreenLoadStatus loadStatus = VsScreenLoadStatus::Loading;
    VsPortraitView p1Portrait;
};

void drawVersusScreenOverlay(const UiRenderContext& ui, const VsScreenView& view);

} // namespace dragon
