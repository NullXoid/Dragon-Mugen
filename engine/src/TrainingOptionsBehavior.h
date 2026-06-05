#pragma once

#include "AppTypes.h"

#include <string>
#include <string_view>

namespace dragon {

std::string_view dummyGuardModeStatus(DummyGuardMode mode);
void cycleDummyGuardMode(TrainingOptions& settings);

std::string_view trainingPowerModeStatus(TrainingPowerMode mode);
void cycleTrainingPowerMode(TrainingOptions& settings);

std::string_view trainingMoveCategoryStatus(TrainingMoveCategory category);
void cycleTrainingMoveCategory(TrainingOptions& settings);

std::string_view trainingOptionLabel(int option);
std::string trainingOptionStatus(const TrainingOptions& settings, int option);
void toggleTrainingOption(TrainingOptions& settings, int option);

} // namespace dragon
