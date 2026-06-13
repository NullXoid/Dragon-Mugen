#pragma once

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
};

struct TrainingCommandHudView {
    TrainingInputHudView input;
    std::span<const TrainingCommandRowView> commandRows;
    std::span<const TrainingCommandStepView> practiceSteps;
    std::string currentMoveName;
    std::string currentMoveInput;
    std::string activeCommandLabel;
    std::string categoryLabel;
    std::string pageLabel;
    std::string showMeLabel;
    std::string completionLabel;
    bool completeFlash = false;
    bool completionVisible = false;
    bool demoActive = false;
    bool commandsVisible = false;
};

} // namespace dragon
