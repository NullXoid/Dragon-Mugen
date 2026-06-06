#pragma once

#include "AppTypes.h"

#include <string>
#include <string_view>

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

OpponentType defaultOpponentTypeForMode(PendingMode mode);
bool isMatchMode(PendingMode mode);
std::string_view pendingModeTitle(PendingMode mode);
std::string_view opponentTypeLabel(OpponentType type);

int moveMainMenuSelection(int selected, FrontendKey key);
FrontendAction decideMainMenuAction(int selected);

int moveOptionsSelection(int selected, FrontendKey key);
MainSettings cycleMainSetting(MainSettings settings, int row, int direction, int gamepadDeviceCount);
FrontendAction decideOptionsAction(const MainSettings& settings, FrontendKey key);
std::string_view mainSettingLabel(int option);
std::string matchTimerSettingText(const MainSettings& settings);
std::string canvasSizeSettingText(const MainSettings& settings);
std::string uiScaleSettingText(const MainSettings& settings);
std::string gamepadPromptStyleText(GamepadPromptStyle style);
int matchTimerTicksFromSettings(const MainSettings& settings);
std::string compactSettingText(const std::string& value, size_t maxChars);

int moveCharacterCursor(int selected, int characterCount, FrontendKey key);
FrontendAction decideCharacterSelectAction(int selected, int characterCount, FrontendKey key);

int moveStageCursor(int selected, int stageCount, FrontendKey key);
FrontendAction decideStageSelectAction(int selected, int stageCount, FrontendKey key);

FrontendAction decideVsScreenAction(FrontendKey key);

} // namespace dragon
