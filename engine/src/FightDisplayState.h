#pragma once

#include <array>

#include "AppTypes.h"

namespace dragon {

struct FightDisplayState {
    int envShakeTicks = 0;
    int envShakeTotalTicks = 0;
    int envShakeFrequency = 60;
    float envShakeAmplitude = 0.0f;
    int envShakePhase = 0;
    float envShakeOffsetY = 0.0f;
    std::array<ComboCounterState, 2> comboCounters;
};

} // namespace dragon
