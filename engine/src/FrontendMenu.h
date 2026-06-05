#pragma once

#include "AppTypes.h"

namespace dragon {

enum class FrontendKey {
    Other,
    Escape,
    Up,
    Down,
    Left,
    Right,
    Accept,
};

enum class FrontendActionKind {
    None,
    ExitApp,
    OpenMode,
    OpenOptions,
    BackToMain,
    BackToCharacterSelect,
    BackToStageSelect,
    CharacterChosen,
    StageChosen,
    StartFightRequested,
};

struct FrontendAction {
    FrontendActionKind kind = FrontendActionKind::None;
    PendingMode mode = PendingMode::Training;
    int index = -1;
};

int moveMainMenuSelection(int selected, FrontendKey key);
FrontendAction decideMainMenuAction(int selected);

int moveOptionsSelection(int selected, FrontendKey key);
MainSettings cycleMainSetting(MainSettings settings, int row, int direction, int gamepadDeviceCount);
FrontendAction decideOptionsAction(const MainSettings& settings, FrontendKey key);

int moveCharacterCursor(int selected, int characterCount, FrontendKey key);
FrontendAction decideCharacterSelectAction(int selected, int characterCount, FrontendKey key);

int moveStageCursor(int selected, int stageCount, FrontendKey key);
FrontendAction decideStageSelectAction(int selected, int stageCount, FrontendKey key);

FrontendAction decideVsScreenAction(FrontendKey key);

} // namespace dragon
