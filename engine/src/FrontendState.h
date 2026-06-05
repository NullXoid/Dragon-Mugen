#pragma once

#include "AppTypes.h"

namespace dragon {

struct FrontendState {
    Screen screen = Screen::ModeSelect;
    PendingMode pendingMode = PendingMode::Training;
    int selectedMode = 0;
    bool menuRailOnLeft = true;
    bool singleFightPauseOpen = false;
    int selectedSingleFightPauseOption = 0;
    int selectedMatchResultOption = 0;
    int screenFrame = 0;
};

} // namespace dragon
