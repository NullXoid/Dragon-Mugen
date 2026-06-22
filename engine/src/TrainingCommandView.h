#pragma once

#include "TrainingCommandInputRenderer.h"
#include "UiSpriteView.h"

#include <array>
#include <span>
#include <string>

namespace dragon {

enum class TrainingCommandStepStatus {
    Pending,
    Current,
    Matched,
};

struct TrainingCommandStepView {
    std::string label;
    TrainingCommandStepStatus status = TrainingCommandStepStatus::Pending;
};

struct TrainingCommandRowView {
    std::string label;
    std::string input;
    bool active = false;
    bool selected = false;
};

struct TrainingInputHudView {
    bool visible = false;
    std::string currentInput;
    std::string recentInputs;
    std::string expectedInput;
};

struct TrainingCommandButtonGuideButtonView {
    std::string label;
    bool pressed = false;
    bool required = false;
    bool matched = false;
};

struct TrainingCommandButtonGuideView {
    bool visible = false;
    std::array<TrainingCommandButtonGuideButtonView, 4> buttons;
    TrainingCommandButtonGuideButtonView systemButton;
    bool systemButtonVisible = false;
};

struct TrainingCommandDirectionGuideButtonView {
    std::string label;
    bool pressed = false;
    bool required = false;
    bool matched = false;
};

struct TrainingCommandDirectionGuideView {
    bool visible = false;
    std::array<TrainingCommandDirectionGuideButtonView, 4> directions;
};

struct TrainingCommandHudView {
    TrainingInputHudView input;
    TrainingCommandButtonGuideView buttonGuide;
    TrainingCommandDirectionGuideView directionGuide;
    std::span<const TrainingCommandRowView> commandRows;
    std::span<const TrainingCommandStepView> practiceSteps;
    CommandInputIconAtlasView commandIcons;
    UiSpriteView completionCheck;
    std::string currentMoveName;
    std::string currentMoveInput;
    std::string activeCommandLabel;
    std::string categoryLabel;
    std::string pageLabel;
    std::string showMeLabel;
    std::string nextMoveLabel;
    std::string completionLabel;
    bool completeFlash = false;
    bool completionVisible = false;
    int completionTicks = 0;
    bool demoActive = false;
    bool commandsVisible = false;
    bool physicalDirections = false;
    bool paused = false;
    bool hasGuideAnchor = false;
    int facing = 1;
    float guideAnchorX = 0.0f;
};

} // namespace dragon
