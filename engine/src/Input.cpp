#include "Input.h"

namespace dragon {
namespace {

constexpr Sint16 kGamepadAxisDeadzone = 10000;

bool gamepadButtonDown(const GamepadDevice* gamepad, SDL_GamepadButton button) {
    return gamepad && gamepad->handle && SDL_GetGamepadButton(gamepad->handle, button);
}

bool gamepadAxisLessThan(const GamepadDevice* gamepad, SDL_GamepadAxis axis, Sint16 threshold) {
    return gamepad && gamepad->handle && SDL_GetGamepadAxis(gamepad->handle, axis) < threshold;
}

bool gamepadAxisGreaterThan(const GamepadDevice* gamepad, SDL_GamepadAxis axis, Sint16 threshold) {
    return gamepad && gamepad->handle && SDL_GetGamepadAxis(gamepad->handle, axis) > threshold;
}

} // namespace

FighterControls p1Controls() {
    return FighterControls{};
}

bool isPlaystationGamepad(SDL_GamepadType type) {
    return type == SDL_GAMEPAD_TYPE_PS3 || type == SDL_GAMEPAD_TYPE_PS4 || type == SDL_GAMEPAD_TYPE_PS5;
}

bool isXboxGamepad(SDL_GamepadType type) {
    return type == SDL_GAMEPAD_TYPE_XBOX360 || type == SDL_GAMEPAD_TYPE_XBOXONE;
}

std::string gamepadFamilyName(SDL_GamepadType type) {
    if (isPlaystationGamepad(type)) {
        return "PlayStation";
    }
    if (isXboxGamepad(type)) {
        return "Xbox";
    }
    return "Standard";
}

bool gamepadButtonMapsToFighterStart(SDL_GamepadType type, SDL_GamepadButton button) {
    return isPlaystationGamepad(type) && button == SDL_GAMEPAD_BUTTON_TOUCHPAD;
}

FighterInputState collectFighterInput(const bool* keys, const FighterControls& controls, const GamepadDevice* gamepad) {
    FighterInputState input;
    input.left = keys[controls.left]
        || gamepadButtonDown(gamepad, SDL_GAMEPAD_BUTTON_DPAD_LEFT)
        || gamepadAxisLessThan(gamepad, SDL_GAMEPAD_AXIS_LEFTX, -kGamepadAxisDeadzone);
    input.right = keys[controls.right]
        || gamepadButtonDown(gamepad, SDL_GAMEPAD_BUTTON_DPAD_RIGHT)
        || gamepadAxisGreaterThan(gamepad, SDL_GAMEPAD_AXIS_LEFTX, kGamepadAxisDeadzone);
    input.up = keys[controls.up]
        || gamepadButtonDown(gamepad, SDL_GAMEPAD_BUTTON_DPAD_UP)
        || gamepadAxisLessThan(gamepad, SDL_GAMEPAD_AXIS_LEFTY, -kGamepadAxisDeadzone);
    input.down = keys[controls.down]
        || gamepadButtonDown(gamepad, SDL_GAMEPAD_BUTTON_DPAD_DOWN)
        || gamepadAxisGreaterThan(gamepad, SDL_GAMEPAD_AXIS_LEFTY, kGamepadAxisDeadzone);
    input.x = keys[controls.x] || gamepadButtonDown(gamepad, SDL_GAMEPAD_BUTTON_WEST);
    input.y = keys[controls.y] || gamepadButtonDown(gamepad, SDL_GAMEPAD_BUTTON_NORTH);
    input.z = keys[controls.z] || gamepadButtonDown(gamepad, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER);
    input.a = keys[controls.a] || gamepadButtonDown(gamepad, SDL_GAMEPAD_BUTTON_SOUTH);
    input.b = keys[controls.b] || gamepadButtonDown(gamepad, SDL_GAMEPAD_BUTTON_EAST);
    input.c = keys[controls.c] || gamepadButtonDown(gamepad, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER);
    input.s = keys[controls.s]
        || (gamepad
            && gamepadButtonMapsToFighterStart(gamepad->type, SDL_GAMEPAD_BUTTON_TOUCHPAD)
            && gamepadButtonDown(gamepad, SDL_GAMEPAD_BUTTON_TOUCHPAD));
    input.depthModifier = keys[SDL_SCANCODE_LSHIFT]
        || keys[SDL_SCANCODE_RSHIFT]
        || gamepadAxisGreaterThan(gamepad, SDL_GAMEPAD_AXIS_LEFT_TRIGGER, kGamepadAxisDeadzone);
    return input;
}

bool physicalInputDown(
    const bool* keys,
    const GamepadDevice* gamepad,
    PhysicalInputBinding binding,
    int deadzone,
    int triggerThreshold) {
    switch (binding.kind) {
    case PhysicalInputKind::KeyboardScancode:
        return keys && binding.code >= 0 && binding.code < SDL_SCANCODE_COUNT
            && keys[binding.code];
    case PhysicalInputKind::GamepadButton:
        return gamepadButtonDown(gamepad, static_cast<SDL_GamepadButton>(binding.code));
    case PhysicalInputKind::GamepadAxisPositive: {
        const int threshold = static_cast<SDL_GamepadAxis>(binding.code) == SDL_GAMEPAD_AXIS_LEFT_TRIGGER
                || static_cast<SDL_GamepadAxis>(binding.code) == SDL_GAMEPAD_AXIS_RIGHT_TRIGGER
            ? triggerThreshold
            : deadzone;
        return gamepadAxisGreaterThan(gamepad, static_cast<SDL_GamepadAxis>(binding.code), static_cast<Sint16>(threshold));
    }
    case PhysicalInputKind::GamepadAxisNegative:
        return gamepadAxisLessThan(gamepad, static_cast<SDL_GamepadAxis>(binding.code), static_cast<Sint16>(-deadzone));
    case PhysicalInputKind::None:
    default:
        return false;
    }
}

bool controlActionDown(
    const bool* keys,
    const GamepadDevice* gamepad,
    const ControlProfileBinding& profile,
    InputAction action) {
    const ControlActionBinding* binding = findActionBinding(profile, action);
    if (!binding) {
        return false;
    }
    for (const auto& input : binding->bindings) {
        if (physicalInputDown(keys, gamepad, input, profile.deadzone, profile.triggerThreshold)) {
            return true;
        }
    }
    return false;
}

FighterInputState collectMappedFighterInput(
    const bool* keys,
    const ControlProfileBinding& profile,
    const GamepadDevice* gamepad) {
    FighterInputState input;
    input.left = controlActionDown(keys, gamepad, profile, InputAction::MoveLeft);
    input.right = controlActionDown(keys, gamepad, profile, InputAction::MoveRight);
    input.up = controlActionDown(keys, gamepad, profile, InputAction::MoveUp);
    input.down = controlActionDown(keys, gamepad, profile, InputAction::MoveDown);
    input.x = controlActionDown(keys, gamepad, profile, InputAction::LP)
        || controlActionDown(keys, gamepad, profile, InputAction::LightAttack);
    input.y = controlActionDown(keys, gamepad, profile, InputAction::MP)
        || controlActionDown(keys, gamepad, profile, InputAction::HeavyAttack);
    input.z = controlActionDown(keys, gamepad, profile, InputAction::SP)
        || controlActionDown(keys, gamepad, profile, InputAction::Special);
    input.a = controlActionDown(keys, gamepad, profile, InputAction::LK)
        || controlActionDown(keys, gamepad, profile, InputAction::Jump);
    input.b = controlActionDown(keys, gamepad, profile, InputAction::MK)
        || controlActionDown(keys, gamepad, profile, InputAction::Grab);
    input.c = controlActionDown(keys, gamepad, profile, InputAction::SK)
        || controlActionDown(keys, gamepad, profile, InputAction::Guard);
    input.s = controlActionDown(keys, gamepad, profile, InputAction::Taunt);
    input.depthModifier = controlActionDown(keys, gamepad, profile, InputAction::DepthModifier);
    return input;
}

} // namespace dragon
