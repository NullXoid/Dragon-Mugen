#pragma once

#include "AppTypes.h"
#include "ControlMapping.h"
#include "FrontendMenu.h"
#include "OptionsMenuOverlay.h"

#include <array>
#include <string>
#include <vector>

namespace dragon {

struct ControlsOptionsContext {
    MainSettings settings;
    const ControlsSettings* controls = nullptr;
    std::array<std::string, kControlPlayerCount> playerProfileIds;
    std::array<std::string, kControlPlayerCount> playerProfileNames;
    std::array<std::string, kControlPlayerCount> gamepadAssignmentText;
    std::string padSummary;
    GamepadPromptStyle promptStyle = GamepadPromptStyle::Auto;
};

int optionsScreenRowCount(const MainSettings& settings);
int currentOptionsSelection(const MainSettings& settings);
void setCurrentOptionsSelection(MainSettings& settings, int selected);
int moveCurrentOptionsSelection(const MainSettings& settings, FrontendKey key);
std::vector<InputAction> controlsPlayerActionRows(const MainSettings& settings);
bool controlsPlayerRowToAction(const MainSettings& settings, int row, InputAction* action);
int controlsPlayerActionToRow(const MainSettings& settings, InputAction action);

std::string optionsScreenTitle(OptionsMenuScreen screen);
std::string optionsFooterText(const MainSettings& settings);
std::vector<OptionsMenuRowView> buildControlsOptionsRows(const ControlsOptionsContext& context);
OptionsMenuView buildControlsOptionsView(
    const ControlsOptionsContext& context,
    std::vector<OptionsMenuRowView>& rows);

} // namespace dragon
