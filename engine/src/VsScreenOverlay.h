#pragma once

#include "UiRenderContext.h"
#include "UiSpriteView.h"

#include <string>

namespace dragon {

enum class VsScreenLoadStatus {
    Loading,
    Ready,
    Failed,
};

struct VsScreenView {
    std::string modeTitle;
    std::string p1Name;
    std::string opponentName;
    std::string opponentSlotLabel;
    std::string stageName;
    std::string loadPhaseText;
    std::string loadProgressText;
    VsScreenLoadStatus loadStatus = VsScreenLoadStatus::Loading;
    float loadProgress = 0.0f;
    UiSpriteView p1Portrait;
    UiSpriteView opponentPortrait;
};

void drawVersusScreenOverlay(const UiRenderContext& ui, const VsScreenView& view);

} // namespace dragon
