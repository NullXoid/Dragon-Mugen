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
};

struct OptionsMenuView {
    std::span<const OptionsMenuRowView> rows;
    std::string padSummary;
};

void drawOptionsMenuOverlay(const UiRenderContext& ui, const OptionsMenuView& view);

} // namespace dragon
