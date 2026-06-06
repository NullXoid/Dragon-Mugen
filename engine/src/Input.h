#pragma once

#include <SDL3/SDL.h>

#include <string>

namespace dragon {

struct FighterControls {
    SDL_Scancode left = SDL_SCANCODE_LEFT;
    SDL_Scancode right = SDL_SCANCODE_RIGHT;
    SDL_Scancode up = SDL_SCANCODE_UP;
    SDL_Scancode down = SDL_SCANCODE_DOWN;
    SDL_Scancode s = SDL_SCANCODE_SPACE;
    SDL_Scancode x = SDL_SCANCODE_A;
    SDL_Scancode y = SDL_SCANCODE_S;
    SDL_Scancode z = SDL_SCANCODE_D;
    SDL_Scancode a = SDL_SCANCODE_Z;
    SDL_Scancode b = SDL_SCANCODE_X;
    SDL_Scancode c = SDL_SCANCODE_C;
};

struct FighterInputState {
    bool left = false;
    bool right = false;
    bool up = false;
    bool down = false;
    bool s = false;
    bool x = false;
    bool y = false;
    bool z = false;
    bool a = false;
    bool b = false;
    bool c = false;
};

struct CommandInputFrame {
    FighterInputState input;
    int tick = 0;
};

struct GamepadDevice {
    SDL_JoystickID id = 0;
    SDL_Gamepad* handle = nullptr;
    std::string name;
    SDL_GamepadType type = SDL_GAMEPAD_TYPE_UNKNOWN;
};

bool isPlaystationGamepad(SDL_GamepadType type);
bool isXboxGamepad(SDL_GamepadType type);
std::string gamepadFamilyName(SDL_GamepadType type);

FighterControls p1Controls();
FighterControls p2Controls();
FighterInputState collectFighterInput(const bool* keys, const FighterControls& controls, const GamepadDevice* gamepad);

} // namespace dragon
