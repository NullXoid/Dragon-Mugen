#pragma once

#include "AppTypes.h"
#include "TrainingCommandInputRenderer.h"
#include "UiRenderContext.h"

#include <span>
#include <string>

namespace dragon {

struct TrainingOptionRowView {
    std::string label;
    std::string status;
    bool selected = false;
};

struct TrainingMoveRowView {
    std::string number;
    std::string label;
    std::string input;
    bool selected = false;
    std::string category;
    bool sectionStart = false;
    bool sectionHeader = false;
};

struct TrainingMoveDetailView {
    std::string name;
    std::string state;
    std::string input;
    std::string type;
    std::string powerStatus;
    std::string performInput;
    bool visible = false;
};

struct TrainingMoveListView {
    std::span<const TrainingMoveRowView> rows;
    TrainingMoveDetailView detail;
    CommandInputIconAtlasView commandIcons;
    std::string selectedCharacterLabel;
    std::string categoryLabel;
    std::string pageLabel;
    TrainingMoveListTab activeTab = TrainingMoveListTab::All;
    int selectedIndex = 0;
    int firstVisibleIndex = 0;
    int totalCount = 0;
    int visibleCapacity = 0;
    bool empty = false;
    bool physicalDirections = false;
    int facing = 1;
};

struct TrainingOptionsMenuView {
    std::span<const TrainingOptionRowView> rows;
    std::string pageLabel;
};

struct TrainingOptionsMenuGeometryReport {
    bool ok = false;
    std::string detail;
};

void drawTrainingOptionsMenu(const UiRenderContext& ui, const TrainingOptionsMenuView& view);
void drawTrainingMoveListPage(const UiRenderContext& ui, const TrainingMoveListView& view);
TrainingOptionsMenuGeometryReport verifyTrainingOptionsMenuGeometry(const TrainingOptionsMenuView& view);
TrainingOptionsMenuGeometryReport verifyTrainingMoveListGeometry(const TrainingMoveListView& view);

} // namespace dragon
