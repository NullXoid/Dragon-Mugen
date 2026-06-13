#pragma once

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
    bool selected = false;
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
    std::string selectedCharacterLabel;
    std::string categoryLabel;
    std::string pageLabel;
    bool empty = false;
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

} // namespace dragon
