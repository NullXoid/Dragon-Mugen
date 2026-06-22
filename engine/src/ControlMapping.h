#pragma once

#include "AppTypes.h"

#include <SDL3/SDL.h>

#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace dragon {

inline constexpr int kControlPlayerCount = 4;

enum class InputAction {
    MoveLeft,
    MoveRight,
    MoveUp,
    MoveDown,
    LP,
    MP,
    SP,
    LK,
    MK,
    SK,
    Taunt,
    Pause,
    DepthModifier,
    TrainingShow,
    TrainingPrevious,
    TrainingNext,
    Jump,
    LightAttack,
    HeavyAttack,
    Special,
    Grab,
    Guard,
    Dash,
    SuperAssist,
};

enum class InputActionSet {
    Fighting,
    Arena,
    BeatEmUp,
};

enum class ControlPreset {
    ArcadeFighter,
    BeatEmUpModern,
    OpenBorClassic,
    KeyboardClassic,
};

enum class PhysicalInputKind {
    None,
    KeyboardScancode,
    GamepadButton,
    GamepadAxisPositive,
    GamepadAxisNegative,
};

struct PhysicalInputBinding {
    PhysicalInputKind kind = PhysicalInputKind::None;
    int code = 0;
};

struct ControlActionBinding {
    InputAction action = InputAction::MoveLeft;
    std::vector<PhysicalInputBinding> bindings;
};

struct ControlProfileBinding {
    std::string profileId;
    std::string presetName;
    InputActionSet actionSet = InputActionSet::Fighting;
    int deadzone = 10000;
    int triggerThreshold = 10000;
    std::vector<ControlActionBinding> actionBindings;
};

struct ControlsSettings {
    int schemaVersion = 1;
    std::array<int, kControlPlayerCount> gamepadAssignments{ 0, 0, 0, 0 };
    std::vector<ControlProfileBinding> profiles;
};

std::string_view inputActionId(InputAction action);
std::string_view inputActionLabel(InputAction action);
std::vector<InputAction> fightingInputActions();
std::vector<InputAction> requiredFightingInputActions();
std::vector<InputAction> beatEmUpInputActions();
bool isRequiredControlAction(InputAction action);
bool isGameplayCriticalAction(InputAction action);

std::string_view inputActionSetLabel(InputActionSet actionSet);
InputActionSet cycleInputActionSet(InputActionSet actionSet, int direction);
std::string_view controlPresetName(ControlPreset preset);
std::vector<ControlPreset> controlPresets();
std::optional<ControlPreset> controlPresetFromName(std::string_view name);
ControlPreset cycleControlPreset(std::string_view currentPresetName, int direction);

ControlProfileBinding makeDefaultControlProfile(
    std::string_view profileId,
    int playerIndex,
    ControlPreset preset = ControlPreset::ArcadeFighter);
void applyControlPreset(
    ControlProfileBinding& profile,
    ControlPreset preset,
    int playerIndex);
const ControlActionBinding* findActionBinding(
    const ControlProfileBinding& profile,
    InputAction action);
ControlActionBinding& ensureActionBinding(
    ControlProfileBinding& profile,
    InputAction action);
void setPrimaryActionBinding(
    ControlProfileBinding& profile,
    InputAction action,
    PhysicalInputBinding binding);

const ControlProfileBinding* findControlProfile(
    const ControlsSettings& controls,
    std::string_view profileId);
ControlProfileBinding& ensureControlProfile(
    ControlsSettings& controls,
    std::string_view profileId,
    int playerIndex);
void syncDefaultControlProfilesForPlayers(
    ControlsSettings& controls,
    const std::array<std::string, kControlPlayerCount>& profileIds);

std::string physicalInputToken(PhysicalInputBinding binding);
std::optional<PhysicalInputBinding> parsePhysicalInputToken(std::string_view token);
std::string physicalInputLabel(
    PhysicalInputBinding binding,
    GamepadPromptStyle promptStyle = GamepadPromptStyle::Auto);
std::string actionBindingLabel(
    const ControlProfileBinding& profile,
    InputAction action,
    GamepadPromptStyle promptStyle = GamepadPromptStyle::Auto);
std::vector<std::string> missingRequiredControlActions(const ControlProfileBinding& profile);
std::vector<std::string> controlBindingConflicts(const ControlProfileBinding& profile);

bool samePhysicalInput(PhysicalInputBinding lhs, PhysicalInputBinding rhs);
PhysicalInputBinding keyBinding(SDL_Scancode scancode);
PhysicalInputBinding gamepadButtonBinding(SDL_GamepadButton button);
PhysicalInputBinding gamepadAxisPositiveBinding(SDL_GamepadAxis axis);
PhysicalInputBinding gamepadAxisNegativeBinding(SDL_GamepadAxis axis);

} // namespace dragon
