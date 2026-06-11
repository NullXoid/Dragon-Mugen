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

FighterControls p2Controls() {
    FighterControls controls;
    controls.left = SDL_SCANCODE_J;
    controls.right = SDL_SCANCODE_L;
    controls.up = SDL_SCANCODE_I;
    controls.down = SDL_SCANCODE_K;
    controls.s = SDL_SCANCODE_SEMICOLON;
    controls.x = SDL_SCANCODE_U;
    controls.y = SDL_SCANCODE_O;
    controls.z = SDL_SCANCODE_P;
    controls.a = SDL_SCANCODE_N;
    controls.b = SDL_SCANCODE_M;
    controls.c = SDL_SCANCODE_COMMA;
    return controls;
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
    input.s = keys[controls.s];
    input.depthModifier = keys[SDL_SCANCODE_LSHIFT]
        || keys[SDL_SCANCODE_RSHIFT]
        || gamepadAxisGreaterThan(gamepad, SDL_GAMEPAD_AXIS_LEFT_TRIGGER, kGamepadAxisDeadzone);
    return input;
}

} // namespace dragon
