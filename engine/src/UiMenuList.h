#pragma once

#include "UiRenderContext.h"

#include <span>
#include <string>

namespace dragon {

struct UiMenuListRowView {
    std::string label;
    std::string value;
    bool selected = false;
    bool adjustable = false;
    bool disabled = false;
};

struct UiMenuListView {
    std::span<const UiMenuListRowView> rows;
    std::string title;
    std::string pageLabel;
    std::string labelHeader = "SETTING";
    std::string valueHeader = "VALUE";
    std::string statusLine;
    std::string footer;
};

struct UiMenuListStyle {
    float panelY = 48.0f;
    float minPanelW = 320.0f;
    float maxPanelW = 430.0f;
    float rowH = 12.0f;
    bool redSelection = true;
};

struct UiMenuListGeometryReport {
    bool ok = false;
    std::string detail;
};

void drawUiMenuList(const UiRenderContext& ui, const UiMenuListView& view, const UiMenuListStyle& style = {});
UiMenuListGeometryReport verifyUiMenuListGeometry(
    const UiMenuListView& view,
    float frameW,
    const UiMenuListStyle& style = {});

} // namespace dragon
