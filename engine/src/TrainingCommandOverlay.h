#pragma once

#include "TrainingCommandView.h"
#include "UiRenderContext.h"

#include <string>

namespace dragon {

struct TrainingCommandHudGeometryReport {
    bool ok = false;
    bool objectiveVisible = false;
    bool inputVisible = false;
    bool controllerVisible = false;
    bool bottomLegendVisible = false;
    bool commandIconsVisible = false;
    std::string detail;
};

struct TrainingPauseHelpView {
    bool visible = false;
};

struct TrainingPauseHelpGeometryReport {
    bool ok = false;
    bool visible = false;
    bool legendVisible = false;
    std::string detail;
};

void drawTrainingCommandOverlay(const UiRenderContext& ui, const TrainingCommandHudView& view);
void drawTrainingPauseHelpOverlay(const UiRenderContext& ui, const TrainingPauseHelpView& view);
TrainingCommandHudGeometryReport verifyTrainingCommandHudGeometry(
    const TrainingCommandHudView& view,
    int logicalWidth,
    int logicalHeight = 240);
TrainingPauseHelpGeometryReport verifyTrainingPauseHelpGeometry(
    const TrainingPauseHelpView& view,
    int logicalWidth,
    int logicalHeight = 240);

} // namespace dragon
