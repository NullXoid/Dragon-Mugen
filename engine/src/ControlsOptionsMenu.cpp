#include "ControlsOptionsMenu.h"

#include "FrontendMenu.h"

#include <algorithm>
#include <cstddef>
#include <sstream>

namespace dragon {
namespace {

int wrapSelection(int selected, int count, int delta) {
    if (count <= 0) {
        return selected;
    }
    const int current = std::clamp(selected, 0, count - 1);
    return (current + delta + count) % count;
}

std::string compact(const std::string& value, size_t maxChars) {
    return compactSettingText(value, maxChars);
}

std::string missingSummary(const ControlProfileBinding* profile) {
    if (!profile) {
        return "DEFAULT";
    }
    const auto missing = missingRequiredControlActions(*profile);
    if (missing.empty()) {
        return "OK";
    }
    return std::to_string(missing.size()) + " MISSING";
}

std::string conflictSummary(const ControlProfileBinding* profile) {
    if (!profile) {
        return "OK";
    }
    const auto conflicts = controlBindingConflicts(*profile);
    if (conflicts.empty()) {
        return "OK";
    }
    return std::to_string(conflicts.size()) + " WARN";
}

const ControlProfileBinding* playerProfile(const ControlsOptionsContext& context, int playerIndex) {
    if (!context.controls) {
        return nullptr;
    }
    return findControlProfile(*context.controls, context.playerProfileIds[static_cast<size_t>(playerIndex)]);
}

OptionsMenuRowView row(
    std::string label,
    std::string value,
    bool selected,
    bool adjustable = false,
    bool disabled = false) {
    return OptionsMenuRowView{ std::move(label), std::move(value), selected, adjustable, disabled };
}

std::vector<OptionsMenuRowView> buildRootRows(const ControlsOptionsContext& context, int selected) {
    return {
        row("GAMEPLAY", "MATCH/PROFILES", selected == 0),
        row("VIDEO", "CANVAS/FPS", selected == 1),
        row("CONTROLS", "PLAYERS/DEVICES", selected == 2),
        row("BACK", "", selected == 3),
    };
}

std::vector<OptionsMenuRowView> buildGameplayRows(const ControlsOptionsContext& context, int selected) {
    return {
        row("MATCH TIMER", matchTimerSettingText(context.settings), selected == 0, true),
        row("P1 PROFILE", compact(context.playerProfileNames[0], 18), selected == 1, true),
        row("NEW P1 PROFILE", "CREATE", selected == 2),
        row("P2 PROFILE", compact(context.playerProfileNames[1], 18), selected == 3, true),
        row("NEW P2 PROFILE", "CREATE", selected == 4),
        row("FALL FALLBACKS", context.settings.fallFallbacksEnabled ? "ON" : "OFF", selected == 5, true),
        row("BACK", "", selected == 6),
    };
}

std::vector<OptionsMenuRowView> buildVideoRows(const ControlsOptionsContext& context, int selected) {
    return {
        row("CANVAS SIZE", canvasSizeSettingText(context.settings), selected == 0, true),
        row("UI SCALE", uiScaleSettingText(context.settings), selected == 1, true),
        row("FPS CAP", context.settings.fpsCapEnabled ? "60" : "OFF", selected == 2, true),
        row("PERFORMANCE HUD", performanceHudModeText(context.settings.performanceHudMode), selected == 3, true),
        row("BACK", "", selected == 4),
    };
}

std::vector<OptionsMenuRowView> buildControlsRows(const ControlsOptionsContext& context, int selected) {
    return {
        row("P1 CONTROLS", compact(context.playerProfileNames[0], 16), selected == 0),
        row("P2 CONTROLS", compact(context.playerProfileNames[1], 16), selected == 1),
        row("P3 CONTROLS", compact(context.playerProfileNames[2], 16), selected == 2),
        row("P4 CONTROLS", compact(context.playerProfileNames[3], 16), selected == 3),
        row("KEYBOARD SETUP", "KEYS", selected == 4),
        row("PAD SETUP", compact(context.padSummary, 18), selected == 5),
        row("INPUT TEST", "LIVE", selected == 6),
        row("RESTORE DEFAULTS", "ALL", selected == 7),
        row("BACK", "", selected == 8),
    };
}

std::vector<OptionsMenuRowView> buildPlayerRows(const ControlsOptionsContext& context, int selected) {
    const int player = std::clamp(context.settings.selectedControlPlayer, 0, kControlPlayerCount - 1);
    const ControlProfileBinding* profile = playerProfile(context, player);
    std::vector<OptionsMenuRowView> rows;
    rows.push_back(row("PROFILE", compact(context.playerProfileNames[static_cast<size_t>(player)], 18), selected == 0));
    rows.push_back(row("DEVICE", context.gamepadAssignmentText[static_cast<size_t>(player)], selected == 1, true));
    rows.push_back(row("PRESET", profile ? compact(profile->presetName, 18) : "DEFAULT", selected == 2, true));
    rows.push_back(row("ACTION SET", profile ? std::string(inputActionSetLabel(profile->actionSet)) : "FIGHTING", selected == 3, true));

    const auto actions = controlsPlayerActionRows(context.settings);
    for (size_t i = 0; i < actions.size(); ++i) {
        const int rowIndex = kControlPlayerStaticRows + static_cast<int>(i);
        rows.push_back(row(
            std::string(inputActionLabel(actions[i])),
            profile ? actionBindingLabel(*profile, actions[i], context.promptStyle) : "-",
            selected == rowIndex,
            true));
    }

    const int restoreRow = kControlPlayerStaticRows + static_cast<int>(actions.size());
    rows.push_back(row("RESTORE DEFAULTS", missingSummary(profile), selected == restoreRow));
    rows.push_back(row("GUIDED SETUP", conflictSummary(profile), selected == restoreRow + 1));
    rows.push_back(row("BACK", "", selected == restoreRow + 2));
    return rows;
}

std::vector<OptionsMenuRowView> buildKeyboardRows(const ControlsOptionsContext& context, int selected) {
    return {
        row("P1 KEYBOARD", "ARROWS A/S/D Z/X/C", selected == 0),
        row("P2 KEYBOARD", "IJKL U/O/P N/M/,", selected == 1),
        row("P3 KEYBOARD", "TFGH Y/7/8 B/V/6", selected == 2),
        row("P4 KEYBOARD", "NUMPAD", selected == 3),
        row("RESTORE KEYS", "ALL PLAYERS", selected == 4),
        row("BACK", "", selected == 5),
    };
}

std::vector<OptionsMenuRowView> buildControllerRows(const ControlsOptionsContext& context, int selected) {
    return {
        row("PAD LABELS", gamepadPromptStyleText(context.settings.gamepadPromptStyle), selected == 0, true),
        row("P1 GAMEPAD", context.gamepadAssignmentText[0], selected == 1, true),
        row("P2 GAMEPAD", context.gamepadAssignmentText[1], selected == 2, true),
        row("P3 GAMEPAD", context.gamepadAssignmentText[2], selected == 3, true),
        row("P4 GAMEPAD", context.gamepadAssignmentText[3], selected == 4, true),
        row("DEADZONE", "10000", selected == 5, true),
        row("TRIGGER", "10000", selected == 6, true),
        row("BACK", "", selected == 7),
    };
}

std::vector<OptionsMenuRowView> buildInputTestRows(const ControlsOptionsContext& context, int selected) {
    return {
        row("PRESS INPUT", context.settings.controlStatusMessage.empty() ? "WAITING" : compact(context.settings.controlStatusMessage, 22), selected == 0, false),
        row("BACK", "", selected == 1),
    };
}

std::vector<OptionsMenuRowView> buildRestoreRows(const ControlsOptionsContext& context, int selected) {
    return {
        row("RESTORE ALL CONTROLS", "DEFAULTS", selected == 0),
        row("RESTORE PLAYER CONTROLS", "CURRENT PLAYER", selected == 1),
        row("BACK", "", selected == 2),
    };
}

} // namespace

int optionsScreenRowCount(const MainSettings& settings) {
    switch (settings.optionsScreen) {
    case OptionsMenuScreen::Gameplay:
        return kOptionsGameplayCount;
    case OptionsMenuScreen::Video:
        return kOptionsVideoCount;
    case OptionsMenuScreen::Controls:
        return kOptionsControlsCount;
    case OptionsMenuScreen::PlayerControls:
        return kControlPlayerStaticRows + static_cast<int>(controlsPlayerActionRows(settings).size()) + 3;
    case OptionsMenuScreen::KeyboardSetup:
        return kOptionsKeyboardSetupCount;
    case OptionsMenuScreen::ControllerSetup:
        return kOptionsControllerSetupCount;
    case OptionsMenuScreen::InputTest:
        return kOptionsInputTestCount;
    case OptionsMenuScreen::RestoreDefaults:
        return kOptionsRestoreDefaultsCount;
    case OptionsMenuScreen::Root:
    default:
        return kOptionsRootCount;
    }
}

int currentOptionsSelection(const MainSettings& settings) {
    switch (settings.optionsScreen) {
    case OptionsMenuScreen::Gameplay:
        return settings.selectedGameplayOption;
    case OptionsMenuScreen::Video:
        return settings.selectedVideoOption;
    case OptionsMenuScreen::Controls:
        return settings.selectedControlsOption;
    case OptionsMenuScreen::PlayerControls:
        return settings.selectedPlayerControlsOption;
    case OptionsMenuScreen::KeyboardSetup:
        return settings.selectedKeyboardSetupOption;
    case OptionsMenuScreen::ControllerSetup:
        return settings.selectedControllerSetupOption;
    case OptionsMenuScreen::InputTest:
        return settings.selectedInputTestOption;
    case OptionsMenuScreen::RestoreDefaults:
        return settings.selectedRestoreDefaultsOption;
    case OptionsMenuScreen::Root:
    default:
        return settings.selectedRootOption;
    }
}

void setCurrentOptionsSelection(MainSettings& settings, int selected) {
    const int clamped = std::clamp(selected, 0, std::max(0, optionsScreenRowCount(settings) - 1));
    settings.selectedOption = clamped;
    switch (settings.optionsScreen) {
    case OptionsMenuScreen::Gameplay:
        settings.selectedGameplayOption = clamped;
        break;
    case OptionsMenuScreen::Video:
        settings.selectedVideoOption = clamped;
        break;
    case OptionsMenuScreen::Controls:
        settings.selectedControlsOption = clamped;
        break;
    case OptionsMenuScreen::PlayerControls:
        settings.selectedPlayerControlsOption = clamped;
        break;
    case OptionsMenuScreen::KeyboardSetup:
        settings.selectedKeyboardSetupOption = clamped;
        break;
    case OptionsMenuScreen::ControllerSetup:
        settings.selectedControllerSetupOption = clamped;
        break;
    case OptionsMenuScreen::InputTest:
        settings.selectedInputTestOption = clamped;
        break;
    case OptionsMenuScreen::RestoreDefaults:
        settings.selectedRestoreDefaultsOption = clamped;
        break;
    case OptionsMenuScreen::Root:
    default:
        settings.selectedRootOption = clamped;
        break;
    }
}

int moveCurrentOptionsSelection(const MainSettings& settings, FrontendKey key) {
    if (key == FrontendKey::Up) {
        return wrapSelection(currentOptionsSelection(settings), optionsScreenRowCount(settings), -1);
    }
    if (key == FrontendKey::Down) {
        return wrapSelection(currentOptionsSelection(settings), optionsScreenRowCount(settings), 1);
    }
    return currentOptionsSelection(settings);
}

std::vector<InputAction> controlsPlayerActionRows(const MainSettings& settings) {
    if (settings.optionsScreen != OptionsMenuScreen::PlayerControls) {
        return fightingInputActions();
    }
    return fightingInputActions();
}

bool controlsPlayerRowToAction(const MainSettings& settings, int row, InputAction* action) {
    const auto actions = controlsPlayerActionRows(settings);
    const int index = row - kControlPlayerStaticRows;
    if (index < 0 || index >= static_cast<int>(actions.size())) {
        return false;
    }
    if (action) {
        *action = actions[static_cast<size_t>(index)];
    }
    return true;
}

int controlsPlayerActionToRow(const MainSettings& settings, InputAction action) {
    const auto actions = controlsPlayerActionRows(settings);
    const auto it = std::find(actions.begin(), actions.end(), action);
    if (it == actions.end()) {
        return -1;
    }
    return kControlPlayerStaticRows + static_cast<int>(std::distance(actions.begin(), it));
}

std::string optionsScreenTitle(OptionsMenuScreen screen) {
    switch (screen) {
    case OptionsMenuScreen::Gameplay:
        return "GAMEPLAY OPTIONS";
    case OptionsMenuScreen::Video:
        return "VIDEO OPTIONS";
    case OptionsMenuScreen::Controls:
        return "CONTROLS";
    case OptionsMenuScreen::PlayerControls:
        return "PLAYER CONTROLS";
    case OptionsMenuScreen::KeyboardSetup:
        return "KEYBOARD SETUP";
    case OptionsMenuScreen::ControllerSetup:
        return "CONTROLLER SETUP";
    case OptionsMenuScreen::InputTest:
        return "INPUT TEST";
    case OptionsMenuScreen::RestoreDefaults:
        return "RESTORE CONTROLS";
    case OptionsMenuScreen::Root:
    default:
        return "OPTIONS";
    }
}

std::string optionsFooterText(const MainSettings& settings) {
    if (settings.awaitingControlBinding) {
        return "PRESS A KEY/BUTTON  ESC CANCEL";
    }
    if (settings.optionsScreen == OptionsMenuScreen::InputTest) {
        return "PRESS ANY INPUT  ESC BACK";
    }
    if (settings.optionsScreen == OptionsMenuScreen::PlayerControls) {
        return "UP/DOWN  ENTER BIND  L/R CHANGE  ESC";
    }
    return "UP/DOWN  L/R CHANGE  ENTER  ESC";
}

std::vector<OptionsMenuRowView> buildControlsOptionsRows(const ControlsOptionsContext& context) {
    const int selected = currentOptionsSelection(context.settings);
    switch (context.settings.optionsScreen) {
    case OptionsMenuScreen::Gameplay:
        return buildGameplayRows(context, selected);
    case OptionsMenuScreen::Video:
        return buildVideoRows(context, selected);
    case OptionsMenuScreen::Controls:
        return buildControlsRows(context, selected);
    case OptionsMenuScreen::PlayerControls:
        return buildPlayerRows(context, selected);
    case OptionsMenuScreen::KeyboardSetup:
        return buildKeyboardRows(context, selected);
    case OptionsMenuScreen::ControllerSetup:
        return buildControllerRows(context, selected);
    case OptionsMenuScreen::InputTest:
        return buildInputTestRows(context, selected);
    case OptionsMenuScreen::RestoreDefaults:
        return buildRestoreRows(context, selected);
    case OptionsMenuScreen::Root:
    default:
        return buildRootRows(context, selected);
    }
}

OptionsMenuView buildControlsOptionsView(
    const ControlsOptionsContext& context,
    std::vector<OptionsMenuRowView>& rows) {
    rows = buildControlsOptionsRows(context);
    OptionsMenuView view;
    view.rows = rows;
    view.title = optionsScreenTitle(context.settings.optionsScreen);
    view.pageLabel = context.settings.optionsScreen == OptionsMenuScreen::PlayerControls
        ? "P" + std::to_string(std::clamp(context.settings.selectedControlPlayer, 0, kControlPlayerCount - 1) + 1)
        : "";
    view.padSummary = context.settings.awaitingControlBinding
        ? "BINDING " + std::to_string(context.settings.controlBindingActionIndex + 1)
        : context.padSummary;
    view.footer = optionsFooterText(context.settings);
    return view;
}

} // namespace dragon
