#pragma once

#include "AppTypes.h"

#include <string>

namespace dragon {

struct TrainingCommandDemoState {
    bool active = false;
    int selectedMoveListEntry = -1;
    int stepIndex = 0;
    int stepTicks = 0;
    int neutralTicks = 0;
    int elapsedTicks = 0;
    int flashTicks = 0;
};

struct TrainingCommandPracticeState {
    int completedMoveListEntry = -1;
    int completedTargetState = -1;
    int flashTicks = 0;
    int cooldownTicks = 0;
    std::string notification;
};

struct TrainingState {
    TrainingOptions options;
    TrainingCommandDemoState commandDemo;
    TrainingCommandPracticeState commandPractice;
};

} // namespace dragon
