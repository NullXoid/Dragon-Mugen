// App.cpp-internal Controls Options screen-flow glue. Include only from App.cpp after AppState helpers are defined.

ControlsOptionsContext controlsOptionsContext(const AppState& state) {
    ControlsOptionsContext context;
    context.settings = state.mainSettings;
    context.controls = &state.controls;
    context.playerProfileIds = controlProfileIdsForPlayers(state);
    context.playerProfileNames = controlProfileNamesForPlayers(state);
    for (int i = 0; i < kControlPlayerCount; ++i) {
        context.gamepadAssignmentText[static_cast<size_t>(i)] = gamepadAssignmentText(state, i);
    }
    context.padSummary = mainSettingsPadSummary(state);
    context.promptStyle = effectiveGamepadPromptStyle(state, std::clamp(state.mainSettings.selectedControlPlayer, 0, 1));
    return context;
}

int cycleGamepadAssignmentForOptions(int assignment, int deviceCount, int direction) {
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

GamepadPromptStyle cyclePromptStyleForOptions(GamepadPromptStyle style, int direction) {
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

void enterOptionsScreen(AppState& state, OptionsMenuScreen screen) {
    state.mainSettings.optionsScreen = screen;
    state.mainSettings.awaitingControlBinding = false;
    state.mainSettings.guidedControlSetup = false;
    setCurrentOptionsSelection(state.mainSettings, currentOptionsSelection(state.mainSettings));
}

void backFromOptionsScreen(AppState& state) {
    state.mainSettings.awaitingControlBinding = false;
    state.mainSettings.guidedControlSetup = false;
    switch (state.mainSettings.optionsScreen) {
    case OptionsMenuScreen::Root:
        state.frontend.exitConfirmOpen = false;
        state.frontend.screen = Screen::ModeSelect;
        playMenuCancelSound(state);
        break;
    case OptionsMenuScreen::Gameplay:
    case OptionsMenuScreen::Video:
    case OptionsMenuScreen::Controls:
        enterOptionsScreen(state, OptionsMenuScreen::Root);
        playMenuCancelSound(state);
        break;
    case OptionsMenuScreen::PlayerControls:
    case OptionsMenuScreen::KeyboardSetup:
    case OptionsMenuScreen::ControllerSetup:
    case OptionsMenuScreen::InputTest:
    case OptionsMenuScreen::RestoreDefaults:
        enterOptionsScreen(state, OptionsMenuScreen::Controls);
        playMenuCancelSound(state);
        break;
    }
}

InputAction pendingControlBindingAction(const MainSettings& settings) {
    const auto actions = controlsPlayerActionRows(settings);
    const int index = std::clamp(settings.controlBindingActionIndex, 0, std::max(0, static_cast<int>(actions.size()) - 1));
    return actions[static_cast<size_t>(index)];
}

void beginControlBinding(AppState& state, InputAction action, bool guided) {
    const int row = controlsPlayerActionToRow(state.mainSettings, action);
    const auto actions = controlsPlayerActionRows(state.mainSettings);
    int index = 0;
    if (row >= kControlPlayerStaticRows) {
        index = row - kControlPlayerStaticRows;
    } else {
        const auto it = std::find(actions.begin(), actions.end(), action);
        index = it == actions.end() ? 0 : static_cast<int>(std::distance(actions.begin(), it));
    }
    state.mainSettings.awaitingControlBinding = true;
    state.mainSettings.guidedControlSetup = guided;
    state.mainSettings.controlBindingActionIndex = std::clamp(index, 0, std::max(0, static_cast<int>(actions.size()) - 1));
    state.mainSettings.controlStatusMessage =
        "BIND " + std::string(inputActionLabel(pendingControlBindingAction(state.mainSettings)));
}

void captureControlBinding(AppState& state, PhysicalInputBinding binding) {
    if (!state.mainSettings.awaitingControlBinding || binding.kind == PhysicalInputKind::None) {
        return;
    }
    ControlProfileBinding& profile = controlProfileForPlayer(state, state.mainSettings.selectedControlPlayer);
    const InputAction action = pendingControlBindingAction(state.mainSettings);
    setPrimaryActionBinding(profile, action, binding);
    state.mainSettings.controlStatusMessage =
        std::string(inputActionLabel(action)) + " = " + physicalInputLabel(binding, effectiveGamepadPromptStyle(state, 0));

    if (state.mainSettings.guidedControlSetup) {
        const auto required = requiredFightingInputActions();
        auto current = std::find(required.begin(), required.end(), action);
        if (current != required.end()) {
            ++current;
        }
        if (current != required.end()) {
            beginControlBinding(state, *current, true);
            saveControlsStateQuietly(state);
            return;
        }
    }

    state.mainSettings.awaitingControlBinding = false;
    state.mainSettings.guidedControlSetup = false;
    saveControlsStateQuietly(state);
}

void restoreControlProfileDefaults(AppState& state, int playerIndex, ControlPreset preset = ControlPreset::ArcadeFighter) {
    ControlProfileBinding& profile = controlProfileForPlayer(state, playerIndex);
    const std::string profileId = profile.profileId;
    profile = makeDefaultControlProfile(profileId, playerIndex, preset);
}

void restoreAllControlDefaults(AppState& state) {
    state.controls.profiles.clear();
    state.controls.gamepadAssignments = { 0, 0, 0, 0 };
    syncControlsWithProfiles(state);
}

void cycleOptionsValue(AppState& state, int direction) {
    if (direction == 0) {
        return;
    }
    const int row = currentOptionsSelection(state.mainSettings);
    switch (state.mainSettings.optionsScreen) {
    case OptionsMenuScreen::Gameplay:
        if (row == 0) {
            state.mainSettings = cycleMainSetting(state.mainSettings, 0, direction, static_cast<int>(state.gamepads.size()));
        } else if (row == 1 || row == 3) {
            cycleMainProfileSetting(state, row == 1 ? kMainSettingP1ProfileOption : kMainSettingP2ProfileOption, direction);
            syncControlsWithProfiles(state);
        } else if (row == 5) {
            state.mainSettings.fallFallbacksEnabled = !state.mainSettings.fallFallbacksEnabled;
        }
        break;
    case OptionsMenuScreen::Video:
        if (row == 0) {
            state.mainSettings = cycleMainSetting(state.mainSettings, 1, direction, static_cast<int>(state.gamepads.size()));
        } else if (row == 1) {
            state.mainSettings = cycleMainSetting(state.mainSettings, 2, direction, static_cast<int>(state.gamepads.size()));
        } else if (row == 2) {
            state.mainSettings.fpsCapEnabled = !state.mainSettings.fpsCapEnabled;
        } else if (row == 3) {
            state.mainSettings.performanceHudMode =
                cyclePerformanceHudMode(state.mainSettings.performanceHudMode, direction);
        }
        break;
    case OptionsMenuScreen::PlayerControls: {
        const int player = state.mainSettings.selectedControlPlayer;
        if (row == 1) {
            setGamepadAssignmentForPlayer(
                state,
                player,
                cycleGamepadAssignmentForOptions(gamepadAssignmentForPlayer(state, player), static_cast<int>(state.gamepads.size()), direction));
            saveControlsStateQuietly(state);
        } else if (row == 2) {
            ControlProfileBinding& profile = controlProfileForPlayer(state, player);
            applyControlPreset(profile, cycleControlPreset(profile.presetName, direction), player);
            saveControlsStateQuietly(state);
        } else if (row == 3) {
            ControlProfileBinding& profile = controlProfileForPlayer(state, player);
            profile.actionSet = cycleInputActionSet(profile.actionSet, direction);
            saveControlsStateQuietly(state);
        }
        break;
    }
    case OptionsMenuScreen::ControllerSetup:
        if (row == 0) {
            state.mainSettings.gamepadPromptStyle = cyclePromptStyleForOptions(state.mainSettings.gamepadPromptStyle, direction);
        } else if (row >= 1 && row <= 4) {
            const int player = row - 1;
            setGamepadAssignmentForPlayer(
                state,
                player,
                cycleGamepadAssignmentForOptions(gamepadAssignmentForPlayer(state, player), static_cast<int>(state.gamepads.size()), direction));
            saveControlsStateQuietly(state);
        }
        break;
    default:
        break;
    }
}

void acceptOptionsSelection(AppState& state) {
    const int row = currentOptionsSelection(state.mainSettings);
    switch (state.mainSettings.optionsScreen) {
    case OptionsMenuScreen::Root:
        if (row == 0) enterOptionsScreen(state, OptionsMenuScreen::Gameplay);
        else if (row == 1) enterOptionsScreen(state, OptionsMenuScreen::Video);
        else if (row == 2) enterOptionsScreen(state, OptionsMenuScreen::Controls);
        else backFromOptionsScreen(state);
        break;
    case OptionsMenuScreen::Gameplay:
        if (row == 6) {
            enterOptionsScreen(state, OptionsMenuScreen::Root);
        } else if (row == 2 || row == 4) {
            createMainProfileForSetting(state, row == 2 ? kMainSettingP1ProfileOption : kMainSettingP2ProfileOption);
            syncControlsWithProfiles(state);
        } else {
            cycleOptionsValue(state, 1);
        }
        break;
    case OptionsMenuScreen::Video:
        if (row == kOptionsVideoCount - 1) {
            enterOptionsScreen(state, OptionsMenuScreen::Root);
        } else {
            cycleOptionsValue(state, 1);
        }
        break;
    case OptionsMenuScreen::Controls:
        if (row >= 0 && row <= 3) {
            state.mainSettings.selectedControlPlayer = row;
            enterOptionsScreen(state, OptionsMenuScreen::PlayerControls);
        } else if (row == 4) {
            enterOptionsScreen(state, OptionsMenuScreen::KeyboardSetup);
        } else if (row == 5) {
            enterOptionsScreen(state, OptionsMenuScreen::ControllerSetup);
        } else if (row == 6) {
            enterOptionsScreen(state, OptionsMenuScreen::InputTest);
        } else if (row == 7) {
            enterOptionsScreen(state, OptionsMenuScreen::RestoreDefaults);
        } else {
            enterOptionsScreen(state, OptionsMenuScreen::Root);
        }
        break;
    case OptionsMenuScreen::PlayerControls: {
        InputAction action;
        if (controlsPlayerRowToAction(state.mainSettings, row, &action)) {
            beginControlBinding(state, action, false);
        } else {
            const int firstCommandRowAfterActions =
                kControlPlayerStaticRows + static_cast<int>(controlsPlayerActionRows(state.mainSettings).size());
            if (row == firstCommandRowAfterActions) {
                restoreControlProfileDefaults(state, state.mainSettings.selectedControlPlayer);
                saveControlsStateQuietly(state);
            } else if (row == firstCommandRowAfterActions + 1) {
                beginControlBinding(state, requiredFightingInputActions().front(), true);
            } else if (row == firstCommandRowAfterActions + 2) {
                enterOptionsScreen(state, OptionsMenuScreen::Controls);
            } else {
                cycleOptionsValue(state, 1);
            }
        }
        break;
    }
    case OptionsMenuScreen::KeyboardSetup:
        if (row >= 0 && row <= 3) {
            state.mainSettings.selectedControlPlayer = row;
            enterOptionsScreen(state, OptionsMenuScreen::PlayerControls);
        } else if (row == 4) {
            for (int player = 0; player < kControlPlayerCount; ++player) {
                restoreControlProfileDefaults(state, player, ControlPreset::KeyboardClassic);
            }
            saveControlsStateQuietly(state);
        } else {
            enterOptionsScreen(state, OptionsMenuScreen::Controls);
        }
        break;
    case OptionsMenuScreen::ControllerSetup:
        if (row == 7) {
            enterOptionsScreen(state, OptionsMenuScreen::Controls);
        } else {
            cycleOptionsValue(state, 1);
        }
        break;
    case OptionsMenuScreen::InputTest:
        if (row == 1) {
            enterOptionsScreen(state, OptionsMenuScreen::Controls);
        }
        break;
    case OptionsMenuScreen::RestoreDefaults:
        if (row == 0) {
            restoreAllControlDefaults(state);
            saveControlsStateQuietly(state);
        } else if (row == 1) {
            restoreControlProfileDefaults(state, state.mainSettings.selectedControlPlayer);
            saveControlsStateQuietly(state);
        } else {
            enterOptionsScreen(state, OptionsMenuScreen::Controls);
        }
        break;
    }
}

void handleOptionsKey(AppState& state, SDL_Keycode key) {
    if (state.mainSettings.awaitingControlBinding) {
        if (key == SDLK_ESCAPE) {
            state.mainSettings.awaitingControlBinding = false;
            state.mainSettings.guidedControlSetup = false;
            state.mainSettings.controlStatusMessage = "BIND CANCELLED";
            playMenuCancelSound(state);
            return;
        }
        const SDL_Scancode scancode = SDL_GetScancodeFromKey(key, nullptr);
        if (scancode != SDL_SCANCODE_UNKNOWN) {
            captureControlBinding(state, keyBinding(scancode));
            playMenuCursorDoneSound(state);
        }
        return;
    }

    if (state.mainSettings.optionsScreen == OptionsMenuScreen::InputTest && key != SDLK_ESCAPE) {
        const SDL_Scancode scancode = SDL_GetScancodeFromKey(key, nullptr);
        state.mainSettings.controlStatusMessage =
            scancode == SDL_SCANCODE_UNKNOWN ? "UNKNOWN KEY" : "KEY " + physicalInputLabel(keyBinding(scancode));
        playMenuCursorDoneSound(state);
        return;
    }

    const auto frontendKeyFromOptionsKey = [](SDL_Keycode keycode) {
        switch (keycode) {
        case SDLK_ESCAPE:
            return FrontendKey::Escape;
        case SDLK_UP:
            return FrontendKey::Up;
        case SDLK_DOWN:
            return FrontendKey::Down;
        case SDLK_LEFT:
            return FrontendKey::Left;
        case SDLK_RIGHT:
            return FrontendKey::Right;
        case SDLK_RETURN:
        case SDLK_KP_ENTER:
        case SDLK_SPACE:
            return FrontendKey::Accept;
        default:
            return FrontendKey::Other;
        }
    };
    const auto cycleDirectionFromOptionsKey = [](FrontendKey frontendKey) {
        if (frontendKey == FrontendKey::Left) {
            return -1;
        }
        if (frontendKey == FrontendKey::Right) {
            return 1;
        }
        return 0;
    };

    const FrontendKey frontendKey = frontendKeyFromOptionsKey(key);
    const int previousSelection = currentOptionsSelection(state.mainSettings);
    setCurrentOptionsSelection(state.mainSettings, moveCurrentOptionsSelection(state.mainSettings, frontendKey));
    if (currentOptionsSelection(state.mainSettings) != previousSelection) {
        playMenuCursorMoveSound(state);
    }

    if (frontendKey == FrontendKey::Escape) {
        backFromOptionsScreen(state);
        return;
    }

    const int cycleDirection = cycleDirectionFromOptionsKey(frontendKey);
    if (cycleDirection != 0) {
        cycleOptionsValue(state, cycleDirection);
        playMenuCursorDoneSound(state);
        return;
    }

    if (frontendKey == FrontendKey::Accept) {
        acceptOptionsSelection(state);
        playMenuCursorDoneSound(state);
    }
}

void handleOptionsGamepadButton(AppState& state, SDL_GamepadButton button) {
    if (state.mainSettings.awaitingControlBinding) {
        if (button == SDL_GAMEPAD_BUTTON_EAST) {
            state.mainSettings.awaitingControlBinding = false;
            state.mainSettings.guidedControlSetup = false;
            state.mainSettings.controlStatusMessage = "BIND CANCELLED";
            playMenuCancelSound(state);
            return;
        }
        captureControlBinding(state, gamepadButtonBinding(button));
        playMenuCursorDoneSound(state);
        return;
    }

    if (state.mainSettings.optionsScreen == OptionsMenuScreen::InputTest) {
        if (button == SDL_GAMEPAD_BUTTON_EAST || button == SDL_GAMEPAD_BUTTON_BACK) {
            enterOptionsScreen(state, OptionsMenuScreen::Controls);
            playMenuCancelSound(state);
            return;
        }
        state.mainSettings.controlStatusMessage =
            "PAD " + physicalInputLabel(gamepadButtonBinding(button), effectiveGamepadPromptStyle(state, 0));
    }
}

void handleOptionsGamepadAxis(AppState& state, SDL_GamepadAxis axis, Sint16 value) {
    constexpr Sint16 kCaptureThreshold = 16000;
    if (std::abs(static_cast<int>(value)) < kCaptureThreshold) {
        return;
    }

    const PhysicalInputBinding binding =
        value > 0 ? gamepadAxisPositiveBinding(axis) : gamepadAxisNegativeBinding(axis);
    if (state.mainSettings.awaitingControlBinding) {
        captureControlBinding(state, binding);
        playMenuCursorDoneSound(state);
        return;
    }

    if (state.mainSettings.optionsScreen == OptionsMenuScreen::InputTest) {
        state.mainSettings.controlStatusMessage =
            "PAD " + physicalInputLabel(binding, effectiveGamepadPromptStyle(state, 0));
    }
}
