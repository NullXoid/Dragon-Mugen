#include "ControlMapping.h"

#include "dragon/MugenText.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <iterator>
#include <map>
#include <sstream>

namespace dragon {
namespace {

std::string lowercaseAscii(std::string_view value) {
    std::string out(value);
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return out;
}

bool equalsNoCase(std::string_view lhs, std::string_view rhs) {
    return lowercaseAscii(lhs) == lowercaseAscii(rhs);
}

bool isContextualControlAction(InputAction action) {
    return action == InputAction::TrainingShow
        || action == InputAction::TrainingPrevious
        || action == InputAction::TrainingNext;
}

std::string normalizeProfileId(std::string_view profileId, int playerIndex) {
    std::string out = trim(profileId);
    if (out.empty()) {
        out = "player" + std::to_string(std::clamp(playerIndex, 0, kControlPlayerCount - 1) + 1);
    }
    return lowercaseAscii(out);
}

ControlActionBinding binding(InputAction action, std::initializer_list<PhysicalInputBinding> inputs) {
    ControlActionBinding out;
    out.action = action;
    out.bindings.assign(inputs.begin(), inputs.end());
    return out;
}

bool samePhysicalInputValue(PhysicalInputBinding lhs, PhysicalInputBinding rhs) {
    return lhs.kind == rhs.kind && lhs.code == rhs.code;
}

void addOrMergeActionBinding(std::vector<ControlActionBinding>& bindings, ControlActionBinding incoming) {
    auto it = std::find_if(bindings.begin(), bindings.end(), [&](const auto& existing) {
        return existing.action == incoming.action;
    });
    if (it == bindings.end()) {
        bindings.push_back(std::move(incoming));
        return;
    }
    for (const auto& input : incoming.bindings) {
        if (std::none_of(it->bindings.begin(), it->bindings.end(), [&](const auto& existing) {
                return samePhysicalInputValue(existing, input);
            })) {
            it->bindings.push_back(input);
        }
    }
}

void addCommonGamepadBindings(std::vector<ControlActionBinding>& bindings) {
    addOrMergeActionBinding(bindings, binding(InputAction::MoveLeft, {
        gamepadButtonBinding(SDL_GAMEPAD_BUTTON_DPAD_LEFT),
        gamepadAxisNegativeBinding(SDL_GAMEPAD_AXIS_LEFTX),
    }));
    addOrMergeActionBinding(bindings, binding(InputAction::MoveRight, {
        gamepadButtonBinding(SDL_GAMEPAD_BUTTON_DPAD_RIGHT),
        gamepadAxisPositiveBinding(SDL_GAMEPAD_AXIS_LEFTX),
    }));
    addOrMergeActionBinding(bindings, binding(InputAction::MoveUp, {
        gamepadButtonBinding(SDL_GAMEPAD_BUTTON_DPAD_UP),
        gamepadAxisNegativeBinding(SDL_GAMEPAD_AXIS_LEFTY),
    }));
    addOrMergeActionBinding(bindings, binding(InputAction::MoveDown, {
        gamepadButtonBinding(SDL_GAMEPAD_BUTTON_DPAD_DOWN),
        gamepadAxisPositiveBinding(SDL_GAMEPAD_AXIS_LEFTY),
    }));
    addOrMergeActionBinding(bindings, binding(InputAction::LP, { gamepadButtonBinding(SDL_GAMEPAD_BUTTON_WEST) }));
    addOrMergeActionBinding(bindings, binding(InputAction::MP, { gamepadButtonBinding(SDL_GAMEPAD_BUTTON_NORTH) }));
    addOrMergeActionBinding(bindings, binding(InputAction::SP, { gamepadButtonBinding(SDL_GAMEPAD_BUTTON_LEFT_SHOULDER) }));
    addOrMergeActionBinding(bindings, binding(InputAction::LK, { gamepadButtonBinding(SDL_GAMEPAD_BUTTON_SOUTH) }));
    addOrMergeActionBinding(bindings, binding(InputAction::MK, { gamepadButtonBinding(SDL_GAMEPAD_BUTTON_EAST) }));
    addOrMergeActionBinding(bindings, binding(InputAction::SK, { gamepadButtonBinding(SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER) }));
    addOrMergeActionBinding(bindings, binding(InputAction::Taunt, { gamepadButtonBinding(SDL_GAMEPAD_BUTTON_TOUCHPAD) }));
    addOrMergeActionBinding(bindings, binding(InputAction::Pause, { gamepadButtonBinding(SDL_GAMEPAD_BUTTON_START) }));
    addOrMergeActionBinding(bindings, binding(InputAction::DepthModifier, { gamepadAxisPositiveBinding(SDL_GAMEPAD_AXIS_LEFT_TRIGGER) }));
    addOrMergeActionBinding(bindings, binding(InputAction::TrainingShow, {
        gamepadButtonBinding(SDL_GAMEPAD_BUTTON_LEFT_STICK),
        gamepadButtonBinding(SDL_GAMEPAD_BUTTON_RIGHT_STICK),
    }));
    addOrMergeActionBinding(bindings, binding(InputAction::TrainingPrevious, { gamepadButtonBinding(SDL_GAMEPAD_BUTTON_LEFT_SHOULDER) }));
    addOrMergeActionBinding(bindings, binding(InputAction::TrainingNext, { gamepadButtonBinding(SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER) }));
}

std::vector<ControlActionBinding> keyboardBindingsForPlayer(int playerIndex) {
    if (playerIndex == 1) {
        return {
            binding(InputAction::MoveLeft, { keyBinding(SDL_SCANCODE_J) }),
            binding(InputAction::MoveRight, { keyBinding(SDL_SCANCODE_L) }),
            binding(InputAction::MoveUp, { keyBinding(SDL_SCANCODE_I) }),
            binding(InputAction::MoveDown, { keyBinding(SDL_SCANCODE_K) }),
            binding(InputAction::LP, { keyBinding(SDL_SCANCODE_U) }),
            binding(InputAction::MP, { keyBinding(SDL_SCANCODE_O) }),
            binding(InputAction::SP, { keyBinding(SDL_SCANCODE_P) }),
            binding(InputAction::LK, { keyBinding(SDL_SCANCODE_N) }),
            binding(InputAction::MK, { keyBinding(SDL_SCANCODE_M) }),
            binding(InputAction::SK, { keyBinding(SDL_SCANCODE_COMMA) }),
            binding(InputAction::Taunt, { keyBinding(SDL_SCANCODE_SEMICOLON) }),
            binding(InputAction::Pause, { keyBinding(SDL_SCANCODE_BACKSPACE) }),
            binding(InputAction::DepthModifier, { keyBinding(SDL_SCANCODE_RSHIFT) }),
        };
    }

    if (playerIndex == 2) {
        return {
            binding(InputAction::MoveLeft, { keyBinding(SDL_SCANCODE_F) }),
            binding(InputAction::MoveRight, { keyBinding(SDL_SCANCODE_H) }),
            binding(InputAction::MoveUp, { keyBinding(SDL_SCANCODE_T) }),
            binding(InputAction::MoveDown, { keyBinding(SDL_SCANCODE_G) }),
            binding(InputAction::LP, { keyBinding(SDL_SCANCODE_Y) }),
            binding(InputAction::MP, { keyBinding(SDL_SCANCODE_7) }),
            binding(InputAction::SP, { keyBinding(SDL_SCANCODE_8) }),
            binding(InputAction::LK, { keyBinding(SDL_SCANCODE_B) }),
            binding(InputAction::MK, { keyBinding(SDL_SCANCODE_V) }),
            binding(InputAction::SK, { keyBinding(SDL_SCANCODE_6) }),
            binding(InputAction::Taunt, { keyBinding(SDL_SCANCODE_5) }),
            binding(InputAction::Pause, { keyBinding(SDL_SCANCODE_F9) }),
            binding(InputAction::DepthModifier, { keyBinding(SDL_SCANCODE_LALT) }),
        };
    }

    if (playerIndex == 3) {
        return {
            binding(InputAction::MoveLeft, { keyBinding(SDL_SCANCODE_KP_4) }),
            binding(InputAction::MoveRight, { keyBinding(SDL_SCANCODE_KP_6) }),
            binding(InputAction::MoveUp, { keyBinding(SDL_SCANCODE_KP_8) }),
            binding(InputAction::MoveDown, { keyBinding(SDL_SCANCODE_KP_5) }),
            binding(InputAction::LP, { keyBinding(SDL_SCANCODE_KP_7) }),
            binding(InputAction::MP, { keyBinding(SDL_SCANCODE_KP_9) }),
            binding(InputAction::SP, { keyBinding(SDL_SCANCODE_KP_PLUS) }),
            binding(InputAction::LK, { keyBinding(SDL_SCANCODE_KP_1) }),
            binding(InputAction::MK, { keyBinding(SDL_SCANCODE_KP_2) }),
            binding(InputAction::SK, { keyBinding(SDL_SCANCODE_KP_3) }),
            binding(InputAction::Taunt, { keyBinding(SDL_SCANCODE_KP_0) }),
            binding(InputAction::Pause, { keyBinding(SDL_SCANCODE_F10) }),
            binding(InputAction::DepthModifier, { keyBinding(SDL_SCANCODE_RALT) }),
        };
    }

    return {
        binding(InputAction::MoveLeft, { keyBinding(SDL_SCANCODE_LEFT) }),
        binding(InputAction::MoveRight, { keyBinding(SDL_SCANCODE_RIGHT) }),
        binding(InputAction::MoveUp, { keyBinding(SDL_SCANCODE_UP) }),
        binding(InputAction::MoveDown, { keyBinding(SDL_SCANCODE_DOWN) }),
        binding(InputAction::LP, { keyBinding(SDL_SCANCODE_A) }),
        binding(InputAction::MP, { keyBinding(SDL_SCANCODE_S) }),
        binding(InputAction::SP, { keyBinding(SDL_SCANCODE_D) }),
        binding(InputAction::LK, { keyBinding(SDL_SCANCODE_Z) }),
        binding(InputAction::MK, { keyBinding(SDL_SCANCODE_X) }),
        binding(InputAction::SK, { keyBinding(SDL_SCANCODE_C) }),
        binding(InputAction::Taunt, { keyBinding(SDL_SCANCODE_SPACE) }),
        binding(InputAction::Pause, { keyBinding(SDL_SCANCODE_RETURN) }),
        binding(InputAction::DepthModifier, { keyBinding(SDL_SCANCODE_LSHIFT) }),
        binding(InputAction::TrainingShow, { keyBinding(SDL_SCANCODE_H) }),
        binding(InputAction::TrainingPrevious, { keyBinding(SDL_SCANCODE_PAGEUP) }),
        binding(InputAction::TrainingNext, { keyBinding(SDL_SCANCODE_PAGEDOWN) }),
    };
}

void mergeBinding(ControlProfileBinding& profile, ControlActionBinding incoming) {
    auto it = std::find_if(profile.actionBindings.begin(), profile.actionBindings.end(), [&](const auto& existing) {
        return existing.action == incoming.action;
    });
    if (it == profile.actionBindings.end()) {
        profile.actionBindings.push_back(std::move(incoming));
        return;
    }
    for (const auto& binding : incoming.bindings) {
        if (std::none_of(it->bindings.begin(), it->bindings.end(), [&](const auto& existing) {
                return samePhysicalInput(existing, binding);
            })) {
            it->bindings.push_back(binding);
        }
    }
}

std::string scancodeName(SDL_Scancode scancode) {
    const char* name = SDL_GetScancodeName(scancode);
    return name && *name ? name : ("KEY" + std::to_string(static_cast<int>(scancode)));
}

std::string gamepadButtonName(SDL_GamepadButton button, GamepadPromptStyle promptStyle) {
    switch (button) {
    case SDL_GAMEPAD_BUTTON_SOUTH:
        return promptStyle == GamepadPromptStyle::Playstation ? "X" : "A";
    case SDL_GAMEPAD_BUTTON_EAST:
        return promptStyle == GamepadPromptStyle::Playstation ? "O" : "B";
    case SDL_GAMEPAD_BUTTON_WEST:
        return promptStyle == GamepadPromptStyle::Playstation ? "SQ" : "X";
    case SDL_GAMEPAD_BUTTON_NORTH:
        return promptStyle == GamepadPromptStyle::Playstation ? "TRI" : "Y";
    case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER:
        return promptStyle == GamepadPromptStyle::Playstation ? "L1" : "LB";
    case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER:
        return promptStyle == GamepadPromptStyle::Playstation ? "R1" : "RB";
    case SDL_GAMEPAD_BUTTON_BACK:
        return promptStyle == GamepadPromptStyle::Playstation ? "SEL" : "BACK";
    case SDL_GAMEPAD_BUTTON_START:
        return promptStyle == GamepadPromptStyle::Playstation ? "OPT" : "START";
    case SDL_GAMEPAD_BUTTON_LEFT_STICK:
        return "L3";
    case SDL_GAMEPAD_BUTTON_RIGHT_STICK:
        return "R3";
    case SDL_GAMEPAD_BUTTON_DPAD_UP:
        return "UP";
    case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
        return "DOWN";
    case SDL_GAMEPAD_BUTTON_DPAD_LEFT:
        return "LEFT";
    case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:
        return "RIGHT";
    case SDL_GAMEPAD_BUTTON_TOUCHPAD:
        return "TP";
    default:
        return "BTN" + std::to_string(static_cast<int>(button));
    }
}

std::string axisName(SDL_GamepadAxis axis, bool positive) {
    switch (axis) {
    case SDL_GAMEPAD_AXIS_LEFTX:
        return positive ? "LS RIGHT" : "LS LEFT";
    case SDL_GAMEPAD_AXIS_LEFTY:
        return positive ? "LS DOWN" : "LS UP";
    case SDL_GAMEPAD_AXIS_RIGHTX:
        return positive ? "RS RIGHT" : "RS LEFT";
    case SDL_GAMEPAD_AXIS_RIGHTY:
        return positive ? "RS DOWN" : "RS UP";
    case SDL_GAMEPAD_AXIS_LEFT_TRIGGER:
        return "LT";
    case SDL_GAMEPAD_AXIS_RIGHT_TRIGGER:
        return "RT";
    default:
        return (positive ? "AXIS+" : "AXIS-") + std::to_string(static_cast<int>(axis));
    }
}

} // namespace

std::string_view inputActionId(InputAction action) {
    switch (action) {
    case InputAction::MoveLeft: return "MoveLeft";
    case InputAction::MoveRight: return "MoveRight";
    case InputAction::MoveUp: return "MoveUp";
    case InputAction::MoveDown: return "MoveDown";
    case InputAction::LP: return "LP";
    case InputAction::MP: return "MP";
    case InputAction::SP: return "SP";
    case InputAction::LK: return "LK";
    case InputAction::MK: return "MK";
    case InputAction::SK: return "SK";
    case InputAction::Taunt: return "Taunt";
    case InputAction::Pause: return "Pause";
    case InputAction::DepthModifier: return "DepthModifier";
    case InputAction::TrainingShow: return "TrainingShow";
    case InputAction::TrainingPrevious: return "TrainingPrevious";
    case InputAction::TrainingNext: return "TrainingNext";
    case InputAction::Jump: return "Jump";
    case InputAction::LightAttack: return "LightAttack";
    case InputAction::HeavyAttack: return "HeavyAttack";
    case InputAction::Special: return "Special";
    case InputAction::Grab: return "Grab";
    case InputAction::Guard: return "Guard";
    case InputAction::Dash: return "Dash";
    case InputAction::SuperAssist: return "SuperAssist";
    default: return "Unknown";
    }
}

std::string_view inputActionLabel(InputAction action) {
    switch (action) {
    case InputAction::MoveLeft: return "LEFT";
    case InputAction::MoveRight: return "RIGHT";
    case InputAction::MoveUp: return "UP";
    case InputAction::MoveDown: return "DOWN";
    case InputAction::LP: return "LIGHT PUNCH";
    case InputAction::MP: return "MED PUNCH";
    case InputAction::SP: return "STRONG PUNCH";
    case InputAction::LK: return "LIGHT KICK";
    case InputAction::MK: return "MED KICK";
    case InputAction::SK: return "STRONG KICK";
    case InputAction::Taunt: return "ST / TAUNT";
    case InputAction::Pause: return "PAUSE";
    case InputAction::DepthModifier: return "DEPTH MOD";
    case InputAction::TrainingShow: return "TRAINING SHOW";
    case InputAction::TrainingPrevious: return "TRAINING PREV";
    case InputAction::TrainingNext: return "TRAINING NEXT";
    case InputAction::Jump: return "JUMP";
    case InputAction::LightAttack: return "LIGHT ATTACK";
    case InputAction::HeavyAttack: return "HEAVY ATTACK";
    case InputAction::Special: return "SPECIAL";
    case InputAction::Grab: return "GRAB";
    case InputAction::Guard: return "GUARD";
    case InputAction::Dash: return "DASH / RUN";
    case InputAction::SuperAssist: return "SUPER / ASSIST";
    default: return "UNKNOWN";
    }
}

std::vector<InputAction> fightingInputActions() {
    return {
        InputAction::MoveLeft,
        InputAction::MoveRight,
        InputAction::MoveUp,
        InputAction::MoveDown,
        InputAction::LP,
        InputAction::MP,
        InputAction::SP,
        InputAction::LK,
        InputAction::MK,
        InputAction::SK,
        InputAction::Taunt,
        InputAction::Pause,
        InputAction::DepthModifier,
        InputAction::TrainingShow,
        InputAction::TrainingPrevious,
        InputAction::TrainingNext,
    };
}

std::vector<InputAction> requiredFightingInputActions() {
    return {
        InputAction::MoveLeft,
        InputAction::MoveRight,
        InputAction::MoveUp,
        InputAction::MoveDown,
        InputAction::LP,
        InputAction::MP,
        InputAction::SP,
        InputAction::LK,
        InputAction::MK,
        InputAction::SK,
        InputAction::Pause,
    };
}

std::vector<InputAction> beatEmUpInputActions() {
    return {
        InputAction::MoveLeft,
        InputAction::MoveRight,
        InputAction::MoveUp,
        InputAction::MoveDown,
        InputAction::Jump,
        InputAction::LightAttack,
        InputAction::HeavyAttack,
        InputAction::Special,
        InputAction::Grab,
        InputAction::Guard,
        InputAction::Dash,
        InputAction::SuperAssist,
        InputAction::Taunt,
        InputAction::Pause,
    };
}

bool isRequiredControlAction(InputAction action) {
    const auto required = requiredFightingInputActions();
    return std::find(required.begin(), required.end(), action) != required.end();
}

bool isGameplayCriticalAction(InputAction action) {
    return isRequiredControlAction(action)
        || action == InputAction::Taunt
        || action == InputAction::DepthModifier;
}

std::string_view inputActionSetLabel(InputActionSet actionSet) {
    switch (actionSet) {
    case InputActionSet::Arena:
        return "ARENA / FLYING DRAGON";
    case InputActionSet::BeatEmUp:
        return "BEAT 'EM UP";
    case InputActionSet::Fighting:
    default:
        return "FIGHTING";
    }
}

InputActionSet cycleInputActionSet(InputActionSet actionSet, int direction) {
    static constexpr std::array<InputActionSet, 3> values{
        InputActionSet::Fighting,
        InputActionSet::Arena,
        InputActionSet::BeatEmUp,
    };
    auto current = std::find(values.begin(), values.end(), actionSet);
    int index = current == values.end() ? 0 : static_cast<int>(std::distance(values.begin(), current));
    index = (index + direction + static_cast<int>(values.size())) % static_cast<int>(values.size());
    return values[static_cast<size_t>(index)];
}

std::string_view controlPresetName(ControlPreset preset) {
    switch (preset) {
    case ControlPreset::BeatEmUpModern:
        return "Beat 'em Up Modern";
    case ControlPreset::OpenBorClassic:
        return "OpenBOR Classic";
    case ControlPreset::KeyboardClassic:
        return "Keyboard Classic";
    case ControlPreset::ArcadeFighter:
    default:
        return "Arcade Fighter";
    }
}

std::vector<ControlPreset> controlPresets() {
    return {
        ControlPreset::ArcadeFighter,
        ControlPreset::BeatEmUpModern,
        ControlPreset::OpenBorClassic,
        ControlPreset::KeyboardClassic,
    };
}

std::optional<ControlPreset> controlPresetFromName(std::string_view name) {
    for (ControlPreset preset : controlPresets()) {
        if (equalsNoCase(controlPresetName(preset), name)) {
            return preset;
        }
    }
    return std::nullopt;
}

ControlPreset cycleControlPreset(std::string_view currentPresetName, int direction) {
    const auto values = controlPresets();
    const auto currentPreset = controlPresetFromName(currentPresetName).value_or(ControlPreset::ArcadeFighter);
    auto current = std::find(values.begin(), values.end(), currentPreset);
    int index = current == values.end() ? 0 : static_cast<int>(std::distance(values.begin(), current));
    index = (index + direction + static_cast<int>(values.size())) % static_cast<int>(values.size());
    return values[static_cast<size_t>(index)];
}

ControlProfileBinding makeDefaultControlProfile(
    std::string_view profileId,
    int playerIndex,
    ControlPreset preset) {
    ControlProfileBinding profile;
    profile.profileId = normalizeProfileId(profileId, playerIndex);
    applyControlPreset(profile, preset, playerIndex);
    return profile;
}

void applyControlPreset(
    ControlProfileBinding& profile,
    ControlPreset preset,
    int playerIndex) {
    profile.presetName = std::string(controlPresetName(preset));
    profile.actionBindings.clear();
    profile.actionSet = preset == ControlPreset::BeatEmUpModern || preset == ControlPreset::OpenBorClassic
        ? InputActionSet::BeatEmUp
        : InputActionSet::Fighting;
    profile.deadzone = 10000;
    profile.triggerThreshold = 10000;

    for (auto binding : keyboardBindingsForPlayer(playerIndex)) {
        profile.actionBindings.push_back(std::move(binding));
    }
    addCommonGamepadBindings(profile.actionBindings);

    if (preset == ControlPreset::BeatEmUpModern) {
        mergeBinding(profile, binding(InputAction::Jump, { gamepadButtonBinding(SDL_GAMEPAD_BUTTON_SOUTH) }));
        mergeBinding(profile, binding(InputAction::LightAttack, { gamepadButtonBinding(SDL_GAMEPAD_BUTTON_WEST) }));
        mergeBinding(profile, binding(InputAction::HeavyAttack, { gamepadButtonBinding(SDL_GAMEPAD_BUTTON_NORTH) }));
        mergeBinding(profile, binding(InputAction::Special, { gamepadButtonBinding(SDL_GAMEPAD_BUTTON_EAST) }));
        mergeBinding(profile, binding(InputAction::Guard, { gamepadButtonBinding(SDL_GAMEPAD_BUTTON_LEFT_SHOULDER) }));
        mergeBinding(profile, binding(InputAction::Dash, { gamepadButtonBinding(SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER) }));
    } else if (preset == ControlPreset::OpenBorClassic) {
        mergeBinding(profile, binding(InputAction::Jump, { gamepadButtonBinding(SDL_GAMEPAD_BUTTON_SOUTH) }));
        mergeBinding(profile, binding(InputAction::LightAttack, { gamepadButtonBinding(SDL_GAMEPAD_BUTTON_WEST) }));
        mergeBinding(profile, binding(InputAction::Special, { gamepadButtonBinding(SDL_GAMEPAD_BUTTON_NORTH) }));
        mergeBinding(profile, binding(InputAction::Grab, { gamepadButtonBinding(SDL_GAMEPAD_BUTTON_EAST) }));
    }
}

const ControlActionBinding* findActionBinding(
    const ControlProfileBinding& profile,
    InputAction action) {
    const auto it = std::find_if(profile.actionBindings.begin(), profile.actionBindings.end(), [&](const auto& binding) {
        return binding.action == action;
    });
    return it == profile.actionBindings.end() ? nullptr : &*it;
}

ControlActionBinding& ensureActionBinding(
    ControlProfileBinding& profile,
    InputAction action) {
    auto it = std::find_if(profile.actionBindings.begin(), profile.actionBindings.end(), [&](const auto& binding) {
        return binding.action == action;
    });
    if (it != profile.actionBindings.end()) {
        return *it;
    }
    ControlActionBinding binding;
    binding.action = action;
    profile.actionBindings.push_back(std::move(binding));
    return profile.actionBindings.back();
}

void setPrimaryActionBinding(
    ControlProfileBinding& profile,
    InputAction action,
    PhysicalInputBinding binding) {
    auto& actionBinding = ensureActionBinding(profile, action);
    if (actionBinding.bindings.empty()) {
        actionBinding.bindings.push_back(binding);
    } else {
        actionBinding.bindings.front() = binding;
    }
}

const ControlProfileBinding* findControlProfile(
    const ControlsSettings& controls,
    std::string_view profileId) {
    const std::string id = normalizeProfileId(profileId, 0);
    auto it = std::find_if(controls.profiles.begin(), controls.profiles.end(), [&](const auto& profile) {
        return equalsNoCase(profile.profileId, id);
    });
    return it == controls.profiles.end() ? nullptr : &*it;
}

ControlProfileBinding& ensureControlProfile(
    ControlsSettings& controls,
    std::string_view profileId,
    int playerIndex) {
    const std::string id = normalizeProfileId(profileId, playerIndex);
    auto it = std::find_if(controls.profiles.begin(), controls.profiles.end(), [&](const auto& profile) {
        return equalsNoCase(profile.profileId, id);
    });
    if (it != controls.profiles.end()) {
        return *it;
    }
    controls.profiles.push_back(makeDefaultControlProfile(id, playerIndex));
    return controls.profiles.back();
}

void syncDefaultControlProfilesForPlayers(
    ControlsSettings& controls,
    const std::array<std::string, kControlPlayerCount>& profileIds) {
    for (int i = 0; i < kControlPlayerCount; ++i) {
        ensureControlProfile(controls, profileIds[static_cast<size_t>(i)], i);
    }
}

std::string physicalInputToken(PhysicalInputBinding binding) {
    switch (binding.kind) {
    case PhysicalInputKind::KeyboardScancode:
        return "key:" + std::to_string(binding.code);
    case PhysicalInputKind::GamepadButton:
        return "pad:" + std::to_string(binding.code);
    case PhysicalInputKind::GamepadAxisPositive:
        return "axis+:" + std::to_string(binding.code);
    case PhysicalInputKind::GamepadAxisNegative:
        return "axis-:" + std::to_string(binding.code);
    case PhysicalInputKind::None:
    default:
        return "none";
    }
}

std::optional<PhysicalInputBinding> parsePhysicalInputToken(std::string_view token) {
    const std::string value = trim(token);
    const auto colon = value.find(':');
    if (colon == std::string::npos) {
        return std::nullopt;
    }
    const std::string kind = lowercaseAscii(std::string_view(value).substr(0, colon));
    const std::string rawCode = trim(std::string_view(value).substr(colon + 1));
    int code = 0;
    try {
        size_t consumed = 0;
        code = std::stoi(rawCode, &consumed, 10);
        if (consumed == 0) {
            return std::nullopt;
        }
    } catch (...) {
        return std::nullopt;
    }

    if (kind == "key") {
        return keyBinding(static_cast<SDL_Scancode>(code));
    }
    if (kind == "pad") {
        return gamepadButtonBinding(static_cast<SDL_GamepadButton>(code));
    }
    if (kind == "axis+") {
        return gamepadAxisPositiveBinding(static_cast<SDL_GamepadAxis>(code));
    }
    if (kind == "axis-") {
        return gamepadAxisNegativeBinding(static_cast<SDL_GamepadAxis>(code));
    }
    return std::nullopt;
}

std::string physicalInputLabel(
    PhysicalInputBinding binding,
    GamepadPromptStyle promptStyle) {
    switch (binding.kind) {
    case PhysicalInputKind::KeyboardScancode:
        return scancodeName(static_cast<SDL_Scancode>(binding.code));
    case PhysicalInputKind::GamepadButton:
        return gamepadButtonName(static_cast<SDL_GamepadButton>(binding.code), promptStyle);
    case PhysicalInputKind::GamepadAxisPositive:
        return axisName(static_cast<SDL_GamepadAxis>(binding.code), true);
    case PhysicalInputKind::GamepadAxisNegative:
        return axisName(static_cast<SDL_GamepadAxis>(binding.code), false);
    case PhysicalInputKind::None:
    default:
        return "-";
    }
}

std::string actionBindingLabel(
    const ControlProfileBinding& profile,
    InputAction action,
    GamepadPromptStyle promptStyle) {
    const ControlActionBinding* binding = findActionBinding(profile, action);
    if (!binding || binding->bindings.empty()) {
        return "-";
    }
    std::string out;
    for (const auto& input : binding->bindings) {
        if (!out.empty()) {
            out += " / ";
        }
        out += physicalInputLabel(input, promptStyle);
    }
    return out.empty() ? "-" : out;
}

std::vector<std::string> missingRequiredControlActions(const ControlProfileBinding& profile) {
    std::vector<std::string> out;
    for (InputAction action : requiredFightingInputActions()) {
        const ControlActionBinding* binding = findActionBinding(profile, action);
        if (!binding || binding->bindings.empty()) {
            out.emplace_back(inputActionLabel(action));
        }
    }
    return out;
}

std::vector<std::string> controlBindingConflicts(const ControlProfileBinding& profile) {
    std::vector<std::string> out;
    const auto actions = fightingInputActions();
    for (size_t i = 0; i < actions.size(); ++i) {
        const auto* lhs = findActionBinding(profile, actions[i]);
        if (!lhs) {
            continue;
        }
        for (size_t j = i + 1; j < actions.size(); ++j) {
            const auto* rhs = findActionBinding(profile, actions[j]);
            if (!rhs) {
                continue;
            }
            if (isContextualControlAction(actions[i]) || isContextualControlAction(actions[j])) {
                continue;
            }
            const bool important =
                isGameplayCriticalAction(actions[i]) || isGameplayCriticalAction(actions[j]);
            if (!important) {
                continue;
            }
            for (const auto& left : lhs->bindings) {
                for (const auto& right : rhs->bindings) {
                    if (samePhysicalInput(left, right)) {
                        out.push_back(
                            std::string(inputActionLabel(actions[i]))
                            + " / "
                            + std::string(inputActionLabel(actions[j]))
                            + " share "
                            + physicalInputLabel(left));
                    }
                }
            }
        }
    }
    return out;
}

bool samePhysicalInput(PhysicalInputBinding lhs, PhysicalInputBinding rhs) {
    return lhs.kind == rhs.kind && lhs.code == rhs.code;
}

PhysicalInputBinding keyBinding(SDL_Scancode scancode) {
    return { PhysicalInputKind::KeyboardScancode, static_cast<int>(scancode) };
}

PhysicalInputBinding gamepadButtonBinding(SDL_GamepadButton button) {
    return { PhysicalInputKind::GamepadButton, static_cast<int>(button) };
}

PhysicalInputBinding gamepadAxisPositiveBinding(SDL_GamepadAxis axis) {
    return { PhysicalInputKind::GamepadAxisPositive, static_cast<int>(axis) };
}

PhysicalInputBinding gamepadAxisNegativeBinding(SDL_GamepadAxis axis) {
    return { PhysicalInputKind::GamepadAxisNegative, static_cast<int>(axis) };
}

} // namespace dragon
