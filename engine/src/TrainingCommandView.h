#pragma once

#include <span>
#include <string>

namespace dragon {

struct TrainingCommandRowView {
    std::string label;
    std::string input;
    bool active = false;
};

struct TrainingInputHudView {
    bool visible = false;
    std::string currentInput;
    std::string recentInputs;
};

struct TrainingCommandHudView {
    TrainingInputHudView input;
    std::span<const TrainingCommandRowView> commandRows;
    std::string activeCommandLabel;
    bool commandsVisible = false;
};

} // namespace dragon
