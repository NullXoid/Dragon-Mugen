#pragma once

#include "UiRenderContext.h"

#include <span>
#include <string>

namespace dragon {

struct StoryStageCardView {
    std::string name;
    std::string id;
    std::string author;
    std::string kindLabel;
    bool selected = false;
    bool scrolling = false;
    bool shop = false;
    bool boss = false;
};

struct StoryStageSelectView {
    std::span<const StoryStageCardView> stages;
    std::string fighterLabel;
    std::string routeTitle;
    std::string selectedStageName;
    std::string selectedStageAuthor;
    std::string selectedNodeKind;
    std::string selectedNodeTarget;
    std::string difficultyLabel;
    int selectedIndex = 0;
    int waveCount = 3;
    int frame = 0;
};

void drawStoryStageSelectOverlay(const UiRenderContext& ui, const StoryStageSelectView& view);

} // namespace dragon
