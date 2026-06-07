#pragma once

#include "UiRenderContext.h"

#include <span>
#include <string>

namespace dragon {

struct StageSelectRowView {
    std::string label;
    bool selected = false;
};

struct StageSelectView {
    std::span<const StageSelectRowView> rows;
    std::string modeLabel;
    std::string selectedStageName;
    std::string selectedStageId;
    std::string selectedStageAuthor;
    std::string fighterLabel;
    std::string opponentLabel;
    int frame = 0;
};

void drawStageSelectOverlay(const UiRenderContext& ui, const StageSelectView& view);

} // namespace dragon
