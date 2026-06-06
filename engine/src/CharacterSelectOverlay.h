#pragma once

#include "UiRenderContext.h"
#include "UiSpriteView.h"

#include <span>
#include <string>

namespace dragon {

struct CharacterCellView {
    UiSpriteView icon;
    bool occupied = false;
};

struct CharacterSelectView {
    std::span<const CharacterCellView> cells;
    std::string modeTitle;
    std::string activePlayerLabel = "P1";
    std::string selectedName;
    std::string opponentName;
    std::string preferredStageLabel;
    UiSpriteView selectedPortrait;
    UiSpriteView opponentPortrait;
    UiSpriteView cellSprite;
    UiSpriteView cursorSprite;
    int selectedCell = 0;
    int columns = 5;
    int frame = 0;
};

void drawCharacterSelectOverlay(const UiRenderContext& ui, const CharacterSelectView& view);

} // namespace dragon
