#pragma once

// Internal App.cpp implementation shard.
// Gamepad/device helpers and frontend menu screen assembly.

GamepadDevice* findGamepadDevice(AppState& state, SDL_JoystickID id) {
    auto it = std::find_if(state.gamepads.begin(), state.gamepads.end(), [id](const GamepadDevice& device) {
        return device.id == id;
    });
    return it == state.gamepads.end() ? nullptr : &*it;
}

const GamepadDevice* findGamepadDevice(const AppState& state, SDL_JoystickID id) {
    auto it = std::find_if(state.gamepads.begin(), state.gamepads.end(), [id](const GamepadDevice& device) {
        return device.id == id;
    });
    return it == state.gamepads.end() ? nullptr : &*it;
}

void openGamepadDevice(AppState& state, SDL_JoystickID id) {
    if (findGamepadDevice(state, id) || !SDL_IsGamepad(id)) {
        return;
    }

    SDL_Gamepad* gamepad = SDL_OpenGamepad(id);
    if (!gamepad) {
        SDL_Log("Failed to open gamepad %u: %s", static_cast<unsigned int>(id), SDL_GetError());
        return;
    }

    const char* name = SDL_GetGamepadName(gamepad);
    GamepadDevice device;
    device.id = id;
    device.handle = gamepad;
    device.name = name && *name ? name : "Gamepad";
    device.type = SDL_GetGamepadType(gamepad);
    state.gamepads.push_back(std::move(device));
    SDL_Log("Gamepad connected: %s (%s)", state.gamepads.back().name.c_str(), gamepadFamilyName(state.gamepads.back().type).c_str());
}

void openExistingGamepads(AppState& state) {
    int count = 0;
    SDL_JoystickID* ids = SDL_GetGamepads(&count);
    if (!ids) {
        return;
    }
    for (int i = 0; i < count; ++i) {
        openGamepadDevice(state, ids[i]);
    }
    SDL_free(ids);
}

void refreshGamepadDevice(AppState& state, SDL_JoystickID id) {
    GamepadDevice* device = findGamepadDevice(state, id);
    if (!device || !device->handle) {
        return;
    }
    const char* name = SDL_GetGamepadName(device->handle);
    device->name = name && *name ? name : "Gamepad";
    device->type = SDL_GetGamepadType(device->handle);
}

void closeGamepadDevice(AppState& state, SDL_JoystickID id) {
    auto it = std::find_if(state.gamepads.begin(), state.gamepads.end(), [id](const GamepadDevice& device) {
        return device.id == id;
    });
    if (it == state.gamepads.end()) {
        return;
    }
    if (it->handle) {
        SDL_CloseGamepad(it->handle);
    }
    SDL_Log("Gamepad disconnected: %s", it->name.c_str());
    state.gamepads.erase(it);
}

void closeAllGamepads(AppState& state) {
    for (auto& device : state.gamepads) {
        if (device.handle) {
            SDL_CloseGamepad(device.handle);
            device.handle = nullptr;
        }
    }
    state.gamepads.clear();
}

int gamepadAssignmentForPlayer(const AppState& state, int playerIndex) {
    const int safePlayer = std::clamp(playerIndex, 0, kControlPlayerCount - 1);
    if (safePlayer < static_cast<int>(state.controls.gamepadAssignments.size())) {
        return state.controls.gamepadAssignments[static_cast<size_t>(safePlayer)];
    }
    return safePlayer == 0 ? state.mainSettings.p1GamepadAssignment : state.mainSettings.p2GamepadAssignment;
}

void setGamepadAssignmentForPlayer(AppState& state, int playerIndex, int assignment) {
    const int safePlayer = std::clamp(playerIndex, 0, kControlPlayerCount - 1);
    state.controls.gamepadAssignments[static_cast<size_t>(safePlayer)] = assignment;
    if (safePlayer == 0) {
        state.mainSettings.p1GamepadAssignment = assignment;
    } else if (safePlayer == 1) {
        state.mainSettings.p2GamepadAssignment = assignment;
    }
}

const GamepadDevice* assignedGamepad(const AppState& state, int playerIndex) {
    const int assignment = gamepadAssignmentForPlayer(state, playerIndex);
    if (assignment < 0 || state.gamepads.empty()) {
        return nullptr;
    }
    if (assignment > 0) {
        const int index = assignment - 1;
        return index >= 0 && index < static_cast<int>(state.gamepads.size())
            ? &state.gamepads[static_cast<size_t>(index)]
            : nullptr;
    }
    return playerIndex >= 0 && playerIndex < static_cast<int>(state.gamepads.size())
        ? &state.gamepads[static_cast<size_t>(playerIndex)]
        : nullptr;
}

std::string gamepadAssignmentText(const AppState& state, int playerIndex) {
    const int assignment = gamepadAssignmentForPlayer(state, playerIndex);
    if (assignment < 0) {
        return "OFF";
    }
    const GamepadDevice* device = assignedGamepad(state, playerIndex);
    if (assignment == 0) {
        return device ? "AUTO " + gamepadFamilyName(device->type) : "AUTO NONE";
    }
    if (!device) {
        return "PAD " + std::to_string(assignment) + " MISSING";
    }
    return "PAD " + std::to_string(assignment) + " " + gamepadFamilyName(device->type);
}

GamepadPromptStyle effectiveGamepadPromptStyle(const AppState& state, int playerIndex) {
    if (state.mainSettings.gamepadPromptStyle != GamepadPromptStyle::Auto) {
        return state.mainSettings.gamepadPromptStyle;
    }
    const GamepadDevice* device = assignedGamepad(state, playerIndex);
    return device && isPlaystationGamepad(device->type) ? GamepadPromptStyle::Playstation : GamepadPromptStyle::Xbox;
}

std::string gamepadActionLayoutText(const AppState& state, int playerIndex) {
    return effectiveGamepadPromptStyle(state, playerIndex) == GamepadPromptStyle::Playstation
        ? "Sq/Tri/L1 Cross/Cir/R1"
        : "X/Y/LB A/B/RB";
}

#include "UiRenderHelpers.h"

std::string mainSettingsPadSummary(const AppState& state) {
    if (state.gamepads.empty()) {
        return "PADS NONE";
    }
    return "PADS " + std::to_string(state.gamepads.size()) + "  " + gamepadFamilyName(state.gamepads.front().type);
}

std::array<std::string_view, kMainMenuOptionCount> mainMenuLabelViews(const MainMenuPresentationConfig& config) {
    std::array<std::string_view, kMainMenuOptionCount> labels{};
    for (std::size_t i = 0; i < labels.size(); ++i) {
        labels[i] = config.labels[i];
    }
    return labels;
}

MainMenuLogoView mainMenuLogoView(const SystemScreenAssets& screens) {
    const MainMenuPresentationConfig& config = screens.mainMenu;
    const TextureSprite* sprite = nullptr;
    if (config.logoMode == MainMenuLogoMode::Motif) {
        sprite = &screens.titleLogo;
    } else if (config.logoMode == MainMenuLogoMode::Image) {
        sprite = &screens.mainMenuLogo;
    }
    return MainMenuLogoView{
        sprite ? uiSpriteView(sprite) : UiSpriteView{},
        sprite && sprite->texture,
        config.logoX,
        config.logoY,
        config.logoScale,
        config.logoAlpha,
    };
}

ControlsOptionsContext controlsOptionsContext(const AppState& state);

void drawModeSelect(SDL_Renderer* renderer, const AppState& state) {
    const UiRenderContext ui = uiRenderContext(renderer, state);

    if (state.systemScreens.mainMenu.backgroundMode == MainMenuBackgroundMode::Motif) {
        drawTitleBackground(renderer, state);
    } else {
        drawDragonMainMenuBackdrop(ui, MainMenuBackdropView{
            uiSpriteView(&state.systemScreens.mainMenuBackground),
            state.systemScreens.mainMenu.fallbackGrid,
            state.systemScreens.mainMenu.backgroundPanX,
            state.systemScreens.mainMenu.backgroundDimAlpha,
        });
    }
    drawMainMenuTitleText(ui, MainMenuTitleBarView{
        state.systemScreens.mainMenu.titleLeft,
        state.systemScreens.mainMenu.titleCenter,
        state.systemScreens.mainMenu.titleBarVisible,
    });
    drawMainMenuOverlay(ui, MainMenuView{
        state.frontend.selectedMode,
        state.frontend.screenFrame,
        state.frontend.exitConfirmOpen,
        state.systemScreens.mainMenu.panelLeftText,
        state.systemScreens.mainMenu.panelRightText,
        mainMenuLabelViews(state.systemScreens.mainMenu),
        state.systemScreens.mainMenu.panel,
        mainMenuLogoView(state.systemScreens) });

    drawFpsCounter(renderer, state);
    presentPresentationFrame(renderer, state);
}

void drawMainSettings(SDL_Renderer* renderer, const AppState& state) {
    drawTitleBackground(renderer, state);

    std::vector<OptionsMenuRowView> rows;
    const OptionsMenuView view = buildControlsOptionsView(controlsOptionsContext(state), rows);
    drawOptionsMenuOverlay(
        uiRenderContext(renderer, state),
        view);

    drawFpsCounter(renderer, state);
    presentPresentationFrame(renderer, state);
}

std::string_view opponentSlotLabel(PendingMode mode) {
    return opponentTypeLabel(defaultOpponentTypeForMode(mode));
}

std::string_view opponentSlotLabel(const AppState& state) {
    return opponentTypeLabel(activeOpponentType(state));
}

void drawCharacterSelect(SDL_Renderer* renderer, const AppState& state) {
    drawSelectBackground(renderer, state);

    std::vector<CharacterCellView> cells;
    int selectedCell = 0;
    int p2SelectedCell = 0;
    std::string activePlayerLabel = "P1";
    std::string selectedName;
    std::string profileName;
    std::string selectedProgressionLabel;
    std::string opponentProfileName;
    std::string opponentProgressionLabel;
    std::string opponentName = std::string(opponentSlotLabel(state.frontend.pendingMode));
    std::string preferredStageLabel;
    UiSpriteView selectedPortrait;
    UiSpriteView opponentPortrait;

    if (!state.selection.characters.empty()) {
        const bool vsSelect = state.frontend.pendingMode == PendingMode::SingleFight;
        const int p1Selected = safeCharacterIndex(state.selection, state.selection.selectedCharacter);
        const int p2Selected = safeCharacterIndex(state.selection, state.selection.selectedP2Character);
        const int p1DisplayIndex = p1Selected >= 0 ? p1Selected : 0;
        const int p2DisplayIndex = p2Selected >= 0 ? p2Selected : p1DisplayIndex;
        const int page = p1DisplayIndex / kCharacterSelectPageSize;
        const int firstIndex = page * kCharacterSelectPageSize;
        const int lastIndex = std::min(
            firstIndex + kCharacterSelectPageSize,
            static_cast<int>(state.selection.characters.size()));

        cells.reserve(static_cast<size_t>(lastIndex - firstIndex));
        for (int i = firstIndex; i < lastIndex; ++i) {
            cells.push_back(CharacterCellView{
                uiSpriteView(spriteAt(state.characterIconSprites, i)),
                true,
            });
        }

        selectedCell = p1DisplayIndex - firstIndex;
        p2SelectedCell = p2DisplayIndex - firstIndex;
        const auto& p1Character = state.selection.characters[static_cast<size_t>(p1DisplayIndex)];
        selectedName = compactSettingText(p1Character.displayName, 15);
        preferredStageLabel = compactSettingText(characterPreferredStageName(state.selection, p1DisplayIndex), 22);
        selectedPortrait = uiSpriteView(spriteAt(state.characterFaceSprites, p1DisplayIndex));
        if (state.progression.loaded && state.progression.data.config.enabled) {
            const std::string p1ProfileId = dragonProgressionPlayerProfileId(state.progression.save, 0);
            profileName = compactSettingText(
                dragonProgressionProfileDisplayName(state.progression.save, p1ProfileId),
                14);
            selectedProgressionLabel = compactSettingText(
                dragonProgressionCharacterSummaryForProfile(
                    state.progression.data,
                    state.progression.save,
                    p1ProfileId,
                    p1Character.id),
                20);
        }

        if (state.frontend.pendingMode == PendingMode::Arena) {
            activePlayerLabel = "ARENA";
            opponentName = "RANDOM CPU";
            preferredStageLabel = compactSettingText(selectedStageName(state.selection), 22);
        }
        if (state.frontend.pendingMode == PendingMode::Story) {
            activePlayerLabel = "STORY";
            opponentName = "ENEMY WAVES";
            preferredStageLabel = compactSettingText(selectedStageName(state.selection), 22);
        }

        if (vsSelect) {
            activePlayerLabel = "P1 / P2";
            const auto& p2Character = state.selection.characters[static_cast<size_t>(p2DisplayIndex)];
            opponentName = compactSettingText(p2Character.displayName, 15);
            opponentPortrait = uiSpriteView(spriteAt(state.characterFaceSprites, p2DisplayIndex));
            if (state.progression.loaded && state.progression.data.config.enabled) {
                const std::string p2ProfileId = dragonProgressionPlayerProfileId(state.progression.save, 1);
                opponentProfileName = compactSettingText(
                    dragonProgressionProfileDisplayName(state.progression.save, p2ProfileId),
                    14);
                opponentProgressionLabel = isDragonProgressionGuestProfile(p2ProfileId)
                    ? "GUEST NO SAVE"
                    : compactSettingText(
                        dragonProgressionCharacterSummaryForProfile(
                            state.progression.data,
                            state.progression.save,
                            p2ProfileId,
                            p2Character.id),
                        20);
            }
        }
    }

    drawCharacterSelectOverlay(
        uiRenderContext(renderer, state),
        CharacterSelectView{
            cells,
            std::string(pendingModeTitle(state.frontend.pendingMode)),
            activePlayerLabel,
            selectedName,
            profileName,
            selectedProgressionLabel,
            opponentProfileName,
            opponentProgressionLabel,
            opponentName,
            preferredStageLabel,
            selectedPortrait,
            opponentPortrait,
            uiSpriteView(&state.systemScreens.selectCell),
            uiSpriteView(&state.systemScreens.selectP1Cursor),
            selectedCell,
            p2SelectedCell,
            kCharacterSelectColumns,
            state.frame,
            state.frontend.pendingMode == PendingMode::SingleFight,
            state.selection.p1CharacterConfirmed,
            state.selection.p2CharacterConfirmed,
            activeOpponentType(state) == OpponentType::Dummy,
    });
    drawFpsCounter(renderer, state);
    presentPresentationFrame(renderer, state);
}

const AnimationClip* findClip(const AppState& state, int action) {
    for (const auto& clip : state.characterClips) {
        if (clip.action == action) {
            return &clip;
        }
    }
    for (const auto& clip : state.characterClips) {
        if (clip.action == 0) {
            return &clip;
        }
    }
    return state.characterClips.empty() ? nullptr : &state.characterClips.front();
}

const AnimationClip* findClipInSet(const std::vector<AnimationClip>& clips, int action) {
    for (const auto& clip : clips) {
        if (clip.action == action) {
            return &clip;
        }
    }
    for (const auto& clip : clips) {
        if (clip.action == 0) {
            return &clip;
        }
    }
    return clips.empty() ? nullptr : &clips.front();
}

const ArenaCharacterRuntime* arenaRuntimeForFighterIndex(const AppState& state, size_t fighterIndex) {
    if (!(isArenaMode(state) || isStoryMode(state)) || fighterIndex >= state.arenaRuntimes.size()) {
        return nullptr;
    }
    const auto& runtime = state.arenaRuntimes[fighterIndex];
    return runtime.stateDefs.empty() && runtime.clips.empty() ? nullptr : &runtime;
}

