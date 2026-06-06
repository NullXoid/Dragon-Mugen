#include "FrontendMenu.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <iterator>
#include <vector>

namespace dragon {
namespace {

constexpr int kMainMenuOptionCount = 5;

int wrapSelection(int selected, int count, int delta) {
    if (count <= 0) {
        return selected;
    }
    const int current = std::clamp(selected, 0, count - 1);
    return (current + delta + count) % count;
}

GamepadPromptStyle cyclePromptStyle(GamepadPromptStyle style, int direction) {
    static constexpr std::array<GamepadPromptStyle, 3> values{
        GamepadPromptStyle::Auto,
        GamepadPromptStyle::Xbox,
        GamepadPromptStyle::Playstation,
    };
    auto current = std::find(values.begin(), values.end(), style);
    int index = current == values.end() ? 0 : static_cast<int>(std::distance(values.begin(), current));
    index = (index + direction + static_cast<int>(values.size())) % static_cast<int>(values.size());
    return values[static_cast<size_t>(index)];
}

int cycleGamepadAssignmentValue(int assignment, int deviceCount, int direction) {
    std::vector<int> values;
    values.push_back(0);
    values.push_back(-1);
    for (int i = 1; i <= std::max(0, deviceCount); ++i) {
        values.push_back(i);
    }

    auto current = std::find(values.begin(), values.end(), assignment);
    int index = current == values.end() ? 0 : static_cast<int>(std::distance(values.begin(), current));
    index = (index + direction + static_cast<int>(values.size())) % static_cast<int>(values.size());
    return values[static_cast<size_t>(index)];
}

int cycleValue(int value, int direction, const int* begin, const int* end, int fallbackIndex) {
    const auto current = std::find(begin, end, value);
    int index = current == end ? fallbackIndex : static_cast<int>(std::distance(begin, current));
    const int count = static_cast<int>(std::distance(begin, end));
    index = (index + direction + count) % count;
    return *(begin + index);
}

} // namespace

OpponentType defaultOpponentTypeForMode(PendingMode mode) {
    switch (mode) {
    case PendingMode::Training:
        return OpponentType::Dummy;
    case PendingMode::SinglePlayer:
        return OpponentType::Cpu;
    case PendingMode::SingleFight:
    default:
        return OpponentType::LocalP2;
    }
}

bool isMatchMode(PendingMode mode) {
    return mode == PendingMode::SinglePlayer || mode == PendingMode::SingleFight;
}

std::string_view pendingModeTitle(PendingMode mode) {
    switch (mode) {
    case PendingMode::Training:
        return "TRAINING";
    case PendingMode::SinglePlayer:
        return "SINGLE PLAYER";
    case PendingMode::SingleFight:
    default:
        return "VS MODE";
    }
}

std::string_view opponentTypeLabel(OpponentType type) {
    switch (type) {
    case OpponentType::Dummy:
        return "DUMMY";
    case OpponentType::Cpu:
        return "CPU";
    case OpponentType::LocalP2:
    default:
        return "P2";
    }
}

int moveMainMenuSelection(int selected, FrontendKey key) {
    if (key == FrontendKey::Up) {
        return wrapSelection(selected, kMainMenuOptionCount, -1);
    }
    if (key == FrontendKey::Down) {
        return wrapSelection(selected, kMainMenuOptionCount, 1);
    }
    return selected;
}

FrontendAction decideMainMenuAction(int selected) {
    switch (std::clamp(selected, 0, kMainMenuOptionCount - 1)) {
    case 0:
        return { FrontendActionKind::OpenMode, PendingMode::Training };
    case 1:
        return { FrontendActionKind::OpenMode, PendingMode::SinglePlayer };
    case 2:
        return { FrontendActionKind::OpenMode, PendingMode::SingleFight };
    case 3:
        return { FrontendActionKind::OpenOptions };
    case 4:
        return { FrontendActionKind::ExitApp };
    default:
        return {};
    }
}

int moveOptionsSelection(int selected, FrontendKey key) {
    if (key == FrontendKey::Up) {
        return wrapSelection(selected, kMainSettingsCount, -1);
    }
    if (key == FrontendKey::Down) {
        return wrapSelection(selected, kMainSettingsCount, 1);
    }
    return selected;
}

MainSettings cycleMainSetting(MainSettings settings, int row, int direction, int gamepadDeviceCount) {
    if (direction == 0) {
        return settings;
    }

    switch (row) {
    case 0: {
        static constexpr std::array<int, 6> values{ 30, 60, 90, 99, 120, 180 };
        settings.matchTimerSeconds = cycleValue(
            settings.matchTimerSeconds,
            direction,
            values.data(),
            values.data() + values.size(),
            3);
        break;
    }
    case 1: {
        static constexpr std::array<int, 3> values{
            kClassicLogicalWidth,
            kDefaultLogicalWidth,
            kExtraWideLogicalWidth,
        };
        settings.canvasWidth = cycleValue(
            settings.canvasWidth,
            direction,
            values.data(),
            values.data() + values.size(),
            1);
        break;
    }
    case 2: {
        static constexpr std::array<int, 5> values{ 60, 70, 80, 90, 100 };
        settings.uiScalePercent = cycleValue(
            settings.uiScalePercent,
            direction,
            values.data(),
            values.data() + values.size(),
            2);
        break;
    }
    case 3:
        settings.gamepadPromptStyle = cyclePromptStyle(settings.gamepadPromptStyle, direction);
        break;
    case 4:
        settings.p1GamepadAssignment =
            cycleGamepadAssignmentValue(settings.p1GamepadAssignment, gamepadDeviceCount, direction);
        break;
    case 5:
        settings.p2GamepadAssignment =
            cycleGamepadAssignmentValue(settings.p2GamepadAssignment, gamepadDeviceCount, direction);
        break;
    default:
        break;
    }
    return settings;
}

FrontendAction decideOptionsAction(const MainSettings& settings, FrontendKey key) {
    if (key == FrontendKey::Escape) {
        return { FrontendActionKind::BackToMain };
    }
    if (key == FrontendKey::Accept && settings.selectedOption == kMainSettingsCount - 1) {
        return { FrontendActionKind::BackToMain };
    }
    return {};
}

std::string_view mainSettingLabel(int option) {
    static constexpr std::array<std::string_view, kMainSettingsCount> labels{
        "MATCH TIMER",
        "CANVAS SIZE",
        "UI SCALE",
        "PAD LABELS",
        "P1 GAMEPAD",
        "P2 GAMEPAD",
        "BACK",
    };
    return labels[static_cast<size_t>(std::clamp(option, 0, kMainSettingsCount - 1))];
}

std::string matchTimerSettingText(const MainSettings& settings) {
    if (settings.matchTimerSeconds <= 0) {
        return "OFF";
    }
    return std::to_string(settings.matchTimerSeconds);
}

std::string canvasSizeSettingText(const MainSettings& settings) {
    switch (settings.canvasWidth) {
    case kClassicLogicalWidth:
        return "320x240 CLASSIC";
    case kExtraWideLogicalWidth:
        return "480x240 EXTRA";
    case kDefaultLogicalWidth:
    default:
        return "426x240 WIDE";
    }
}

std::string uiScaleSettingText(const MainSettings& settings) {
    return std::to_string(settings.uiScalePercent) + "%";
}

std::string gamepadPromptStyleText(GamepadPromptStyle style) {
    switch (style) {
    case GamepadPromptStyle::Xbox:
        return "XBOX";
    case GamepadPromptStyle::Playstation:
        return "PLAYSTATION";
    case GamepadPromptStyle::Auto:
    default:
        return "AUTO";
    }
}

int matchTimerTicksFromSettings(const MainSettings& settings) {
    return std::max(1, settings.matchTimerSeconds) * 60;
}

std::string compactSettingText(const std::string& value, size_t maxChars) {
    if (value.size() <= maxChars) {
        return value;
    }
    if (maxChars <= 1) {
        return value.substr(0, maxChars);
    }
    return value.substr(0, maxChars - 1) + "~";
}

int moveCharacterCursor(int selected, int characterCount, FrontendKey key) {
    if (characterCount <= 0) {
        return selected;
    }

    const int current = std::clamp(selected, 0, characterCount - 1);
    const int pageFirst = (current / kCharacterSelectPageSize) * kCharacterSelectPageSize;
    const int local = current - pageFirst;
    const int column = local % kCharacterSelectColumns;
    const int row = local / kCharacterSelectColumns;

    int deltaColumn = 0;
    int deltaRow = 0;
    if (key == FrontendKey::Left) {
        deltaColumn = -1;
    } else if (key == FrontendKey::Right) {
        deltaColumn = 1;
    } else if (key == FrontendKey::Up) {
        deltaRow = -1;
    } else if (key == FrontendKey::Down) {
        deltaRow = 1;
    } else {
        return selected;
    }

    const int targetColumn = column + deltaColumn;
    const int targetRow = row + deltaRow;
    if (targetColumn < 0
        || targetColumn >= kCharacterSelectColumns
        || targetRow < 0
        || targetRow >= kCharacterSelectRows) {
        return selected;
    }

    const int target = pageFirst + targetRow * kCharacterSelectColumns + targetColumn;
    return target >= 0 && target < characterCount ? target : selected;
}

FrontendAction decideCharacterSelectAction(int selected, int characterCount, FrontendKey key) {
    if (key == FrontendKey::Escape) {
        return { FrontendActionKind::BackToMain };
    }
    if (key == FrontendKey::Accept && characterCount > 0) {
        return {
            FrontendActionKind::CharacterChosen,
            PendingMode::Training,
            std::clamp(selected, 0, characterCount - 1),
        };
    }
    return {};
}

int moveStageCursor(int selected, int stageCount, FrontendKey key) {
    if (key == FrontendKey::Up) {
        return wrapSelection(selected, stageCount, -1);
    }
    if (key == FrontendKey::Down) {
        return wrapSelection(selected, stageCount, 1);
    }
    return selected;
}

FrontendAction decideStageSelectAction(int selected, int stageCount, FrontendKey key) {
    if (key == FrontendKey::Escape) {
        return { FrontendActionKind::BackToCharacterSelect };
    }
    if (key == FrontendKey::Accept && stageCount > 0) {
        return {
            FrontendActionKind::StageChosen,
            PendingMode::Training,
            std::clamp(selected, 0, stageCount - 1),
        };
    }
    return {};
}

FrontendAction decideVsScreenAction(FrontendKey key) {
    if (key == FrontendKey::Escape) {
        return { FrontendActionKind::BackToStageSelect };
    }
    if (key == FrontendKey::Accept) {
        return { FrontendActionKind::StartFightRequested };
    }
    return {};
}

} // namespace dragon
