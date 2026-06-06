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
    VsScreenLoadStatus loadStatus = VsScreenLoadStatus::Loading;
    UiSpriteView p1Portrait;
};

void drawVersusScreenOverlay(const UiRenderContext& ui, const VsScreenView& view);

} // namespace dragon
