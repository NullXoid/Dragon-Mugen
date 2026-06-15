#pragma once

#include "AppTypes.h"

namespace dragon {

struct FrontendState {
    Screen screen = Screen::ModeSelect;
    PendingMode pendingMode = PendingMode::Training;
    int selectedMode = 0;
    bool exitConfirmOpen = false;
    bool menuRailOnLeft = true;
    bool fightPauseOpen = false;
    bool screenshotFreeze = false;
    int screenshotFreezeNoticeTicks = 0;
    bool singleFightPauseOpen = false;
    int selectedSingleFightPauseOption = 0;
    int selectedMatchResultOption = 0;
    int selectedArenaSetupOption = 0;
    int screenFrame = 0;
};

} // namespace dragon
