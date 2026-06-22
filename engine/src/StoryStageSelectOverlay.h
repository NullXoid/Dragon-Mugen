#pragma once

#include "UiRenderContext.h"

#include <span>
#include <string>

namespace dragon {

struct StoryStageCardView {
    std::string name;
    std::string id;
    std::string author;
    bool selected = false;
    bool scrolling = false;
};

struct StoryStageSelectView {
    std::span<const StoryStageCardView> stages;
    std::string fighterLabel;
    std::string selectedStageName;
    std::string selectedStageAuthor;
    std::string difficultyLabel;
    int selectedIndex = 0;
    int waveCount = 3;
    int frame = 0;
};

void drawStoryStageSelectOverlay(const UiRenderContext& ui, const StoryStageSelectView& view);

} // namespace dragon
