#pragma once

#include "UiRenderContext.h"

#include <span>
#include <string>

namespace dragon {

struct OptionsMenuRowView {
    std::string label;
    std::string value;
    bool selected = false;
    bool adjustable = false;
    bool disabled = false;
};

struct OptionsMenuView {
    std::span<const OptionsMenuRowView> rows;
    std::string title = "OPTIONS";
    std::string pageLabel;
    std::string labelHeader = "SETTING";
    std::string valueHeader = "VALUE";
    std::string padSummary;
    std::string footer = "UP/DOWN SEL  LEFT/RIGHT CHANGE  ENTER NEW/BACK  ESC";
};

void drawOptionsMenuOverlay(const UiRenderContext& ui, const OptionsMenuView& view);

} // namespace dragon
