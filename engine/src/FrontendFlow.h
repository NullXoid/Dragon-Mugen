#pragma once

// Internal App.cpp extraction file.
// This file depends on App.cpp-local AppState and helper functions.
// Include only from App.cpp after the required types/helpers are defined.
// Do not include from other translation units.

FrontendKey frontendKeyFromSdl(SDL_Keycode key, bool spaceAccept = false) {
    switch (key) {
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
        return FrontendKey::Accept;
    case SDLK_SPACE:
        return spaceAccept ? FrontendKey::Accept : FrontendKey::Other;
    default:
        return FrontendKey::Other;
    }
}

int settingCycleDirection(FrontendKey key) {
    if (key == FrontendKey::Left) {
        return -1;
    }
    if (key == FrontendKey::Right || key == FrontendKey::Accept) {
        return 1;
    }
    return 0;
}

void handleKey(SDL_Renderer* renderer, AppState& state, SDL_Keycode key) {
    if (state.frontend.screen == Screen::ModeSelect) {
        const FrontendKey frontendKey = frontendKeyFromSdl(key);
        state.frontend.selectedMode = moveMainMenuSelection(state.frontend.selectedMode, frontendKey);
        FrontendAction action;
        if (frontendKey == FrontendKey::Escape) {
            action = { FrontendActionKind::ExitApp };
        } else if (frontendKey == FrontendKey::Accept) {
            action = decideMainMenuAction(state.frontend.selectedMode);
        }

        switch (action.kind) {
        case FrontendActionKind::ExitApp:
            state.running = false;
            break;
        case FrontendActionKind::OpenMode:
            unloadCharacterRuntime(state);
            state.frontend.pendingMode = action.mode;
            state.frontend.screen = Screen::CharacterSelect;
            break;
        case FrontendActionKind::OpenOptions:
            state.frontend.screen = Screen::MainSettings;
            break;
        default:
            break;
        }
        return;
    }

    if (state.frontend.screen == Screen::MainSettings) {
        const FrontendKey frontendKey = frontendKeyFromSdl(key, true);
        state.mainSettings.selectedOption = moveOptionsSelection(state.mainSettings.selectedOption, frontendKey);
        const int cycleDirection = settingCycleDirection(frontendKey);
        if (cycleDirection != 0 && state.mainSettings.selectedOption < kMainSettingsCount - 1) {
            state.mainSettings = cycleMainSetting(
                state.mainSettings,
                state.mainSettings.selectedOption,
                cycleDirection,
                static_cast<int>(state.gamepads.size()));
        }

        const FrontendAction action = decideOptionsAction(state.mainSettings, frontendKey);
        if (action.kind == FrontendActionKind::BackToMain) {
            state.frontend.screen = Screen::ModeSelect;
        }
        return;
    }

    if (state.frontend.screen == Screen::CharacterSelect) {
        const int characterCount = static_cast<int>(state.selection.characters.size());
        const FrontendKey frontendKey = frontendKeyFromSdl(key);
        state.selection.selectedCharacter = moveCharacterCursor(state.selection.selectedCharacter, characterCount, frontendKey);
        const FrontendAction action = decideCharacterSelectAction(state.selection.selectedCharacter, characterCount, frontendKey);
        switch (action.kind) {
        case FrontendActionKind::BackToMain:
            unloadCharacterRuntime(state);
            state.frontend.screen = Screen::ModeSelect;
            break;
        case FrontendActionKind::CharacterChosen:
            state.selection.selectedCharacter = action.index;
            configureFightSessionSlotsFromSelection(state);
            selectPreferredStage(state);
            state.frontend.screen = Screen::StageSelect;
            break;
        default:
            break;
        }
        return;
    }

    if (state.frontend.screen == Screen::StageSelect) {
        const int stageCount = static_cast<int>(state.selection.stages.size());
        const FrontendKey frontendKey = frontendKeyFromSdl(key);
        state.selection.selectedStage = moveStageCursor(state.selection.selectedStage, stageCount, frontendKey);
        const FrontendAction action = decideStageSelectAction(state.selection.selectedStage, stageCount, frontendKey);
        switch (action.kind) {
        case FrontendActionKind::BackToCharacterSelect:
            unloadCharacterRuntime(state);
            state.frontend.screen = Screen::CharacterSelect;
            break;
        case FrontendActionKind::StageChosen:
            state.selection.selectedStage = action.index;
            if (!characterSlotAt(state.selection, state.selection.sessionSlots.p1Character)) {
                configureFightSessionSlotsFromSelection(state);
            }
            unloadCharacterRuntime(state);
            state.frontend.screen = Screen::VersusScreen;
            state.frontend.screenFrame = 0;
            state.fightSessionPrepared = false;
            state.fightSessionLoadFailed = false;
            break;
        default:
            break;
        }
        return;
    }

    if (state.frontend.screen == Screen::VersusScreen) {
        const FrontendAction action = decideVsScreenAction(frontendKeyFromSdl(key));
        switch (action.kind) {
        case FrontendActionKind::BackToStageSelect:
            unloadCharacterRuntime(state);
            state.frontend.screen = Screen::StageSelect;
            state.frontend.screenFrame = 0;
            break;
        case FrontendActionKind::StartFightRequested:
            if (state.fightSessionPrepared || prepareFightSession(renderer, state)) {
                beginFight(state);
            }
            break;
        default:
            break;
        }
        return;
    }

    if (state.frontend.screen == Screen::FightView) {
        if (isMatchMode(state)) {
            if (state.frontend.singleFightPauseOpen) {
                switch (key) {
                case SDLK_ESCAPE:
                case SDLK_F2:
                    state.frontend.singleFightPauseOpen = false;
                    break;
                case SDLK_UP:
                    state.frontend.selectedSingleFightPauseOption =
                        (state.frontend.selectedSingleFightPauseOption + kSingleFightPauseOptionCount - 1) % kSingleFightPauseOptionCount;
                    break;
                case SDLK_DOWN:
                    state.frontend.selectedSingleFightPauseOption =
                        (state.frontend.selectedSingleFightPauseOption + 1) % kSingleFightPauseOptionCount;
                    break;
                case SDLK_RETURN:
                case SDLK_KP_ENTER:
                case SDLK_SPACE:
                    switch (state.frontend.selectedSingleFightPauseOption) {
                    case 0:
                        state.frontend.singleFightPauseOpen = false;
                        break;
                    case 1:
                        resetFightState(state);
                        break;
                    case 2:
                        state.frontend.singleFightPauseOpen = false;
                        unloadCharacterRuntime(state);
                        state.frontend.screen = Screen::CharacterSelect;
                        state.frontend.screenFrame = 0;
                        break;
                    case 3:
                        state.frontend.singleFightPauseOpen = false;
                        unloadCharacterRuntime(state);
                        state.frontend.screen = Screen::StageSelect;
                        state.frontend.screenFrame = 0;
                        break;
                    case 4:
                        state.frontend.singleFightPauseOpen = false;
                        unloadCharacterRuntime(state);
                        state.frontend.screen = Screen::ModeSelect;
                        state.frontend.screenFrame = 0;
                        break;
                    default:
                        break;
                    }
                    break;
                default:
                    break;
                }
                return;
            }

            if (state.matchPhase == MatchPhase::MatchResult) {
                switch (key) {
                case SDLK_UP:
                    state.frontend.selectedMatchResultOption =
                        (state.frontend.selectedMatchResultOption + kMatchResultOptionCount - 1) % kMatchResultOptionCount;
                    break;
                case SDLK_DOWN:
                    state.frontend.selectedMatchResultOption =
                        (state.frontend.selectedMatchResultOption + 1) % kMatchResultOptionCount;
                    break;
                case SDLK_R:
                    resetFightState(state);
                    break;
                case SDLK_RETURN:
                case SDLK_KP_ENTER:
                case SDLK_SPACE:
                    switch (state.frontend.selectedMatchResultOption) {
                    case 0:
                        resetFightState(state);
                        break;
                    case 1:
                        unloadCharacterRuntime(state);
                        state.frontend.screen = Screen::CharacterSelect;
                        state.frontend.screenFrame = 0;
                        break;
                    case 2:
                        unloadCharacterRuntime(state);
                        state.frontend.screen = Screen::StageSelect;
                        state.frontend.screenFrame = 0;
                        break;
                    case 3:
                        unloadCharacterRuntime(state);
                        state.frontend.screen = Screen::ModeSelect;
                        state.frontend.screenFrame = 0;
                        break;
                    default:
                        break;
                    }
                    break;
                case SDLK_ESCAPE:
                case SDLK_F2:
                    unloadCharacterRuntime(state);
                    state.frontend.screen = Screen::ModeSelect;
                    state.frontend.screenFrame = 0;
                    break;
                default:
                    break;
                }
                return;
            }

            if (key == SDLK_ESCAPE || key == SDLK_F2) {
                state.frontend.singleFightPauseOpen = true;
                state.frontend.selectedSingleFightPauseOption = 0;
            } else if (key == SDLK_R) {
                resetFightState(state);
            }
            return;
        }

        if (state.trainingOptions.menuOpen) {
            if (state.trainingOptions.moveListOpen) {
                const auto entries = displayableMoveListEntries(state);
                constexpr int visibleRows = 7;
                const int maxScroll = std::max(0, static_cast<int>(entries.size()) - visibleRows);
                const int maxSelected = std::max(0, static_cast<int>(entries.size()) - 1);
                switch (key) {
                case SDLK_ESCAPE:
                case SDLK_F2:
                case SDLK_RETURN:
                case SDLK_KP_ENTER:
                case SDLK_SPACE:
                    state.trainingOptions.moveListOpen = false;
                    break;
                case SDLK_UP:
                    state.trainingOptions.selectedMoveListEntry =
                        std::max(0, state.trainingOptions.selectedMoveListEntry - 1);
                    if (state.trainingOptions.selectedMoveListEntry < state.trainingOptions.moveListScroll) {
                        state.trainingOptions.moveListScroll = state.trainingOptions.selectedMoveListEntry;
                    }
                    break;
                case SDLK_DOWN:
                    state.trainingOptions.selectedMoveListEntry =
                        std::min(maxSelected, state.trainingOptions.selectedMoveListEntry + 1);
                    if (state.trainingOptions.selectedMoveListEntry >= state.trainingOptions.moveListScroll + visibleRows) {
                        state.trainingOptions.moveListScroll =
                            std::min(maxScroll, state.trainingOptions.selectedMoveListEntry - visibleRows + 1);
                    }
                    break;
                default:
                    break;
                }
                state.trainingOptions.moveListScroll = std::clamp(state.trainingOptions.moveListScroll, 0, maxScroll);
                return;
            }

            switch (key) {
            case SDLK_ESCAPE:
            case SDLK_F2:
                state.trainingOptions.menuOpen = false;
                state.trainingOptions.moveListOpen = false;
                break;
            case SDLK_UP:
                state.trainingOptions.selectedOption =
                    (state.trainingOptions.selectedOption + kTrainingOptionCount - 1) % kTrainingOptionCount;
                break;
            case SDLK_DOWN:
                state.trainingOptions.selectedOption =
                    (state.trainingOptions.selectedOption + 1) % kTrainingOptionCount;
                break;
            case SDLK_LEFT:
            case SDLK_RIGHT:
            case SDLK_RETURN:
            case SDLK_KP_ENTER:
            case SDLK_SPACE:
                if (state.trainingOptions.selectedOption == kTrainingResetOption) {
                    resetTrainingPositions(state);
                } else {
                    const int toggledOption = state.trainingOptions.selectedOption;
                    toggleTrainingOption(state.trainingOptions, state.trainingOptions.selectedOption);
                    if (toggledOption == kTrainingP2ControlOption) {
                        clearFighterHitRuntime(state.fighters[1]);
                        enterState(state, state.fighters[1], 0);
                        state.lastHitText = state.trainingOptions.p2Controlled ? "P2 control enabled" : "P2 dummy enabled";
                        state.lastHitTextTicks = 90;
                    } else if (toggledOption == kTrainingPowerOption) {
                        applyTrainingPowerMode(state);
                        state.lastHitText = state.trainingOptions.powerMode == TrainingPowerMode::Max ? "Training power max" : "Training power normal";
                        state.lastHitTextTicks = 90;
                    }
                }
                break;
            default:
                break;
            }
            return;
        }

        if (key == SDLK_ESCAPE) {
            unloadCharacterRuntime(state);
            state.frontend.screen = Screen::StageSelect;
            state.frontend.screenFrame = 0;
        } else if (key == SDLK_F1) {
            state.trainingOptions.showHitboxes = !state.trainingOptions.showHitboxes;
        } else if (key == SDLK_F2) {
            state.trainingOptions.menuOpen = true;
        } else if (key == SDLK_R) {
            resetFightState(state);
        }
        return;
    }
}

std::optional<SDL_Keycode> gamepadMenuKeyForButton(const AppState& state, SDL_GamepadButton button) {
    const bool fightOverlayOpen =
        state.frontend.screen == Screen::FightView
        && ((state.frontend.pendingMode == PendingMode::Training && state.trainingOptions.menuOpen)
            || (isMatchMode(state) && state.frontend.singleFightPauseOpen));

    if (state.frontend.screen == Screen::FightView && !fightOverlayOpen) {
        if (isMatchMode(state) && state.matchPhase == MatchPhase::MatchResult) {
            if (button == SDL_GAMEPAD_BUTTON_SOUTH || button == SDL_GAMEPAD_BUTTON_START) {
                return SDLK_RETURN;
            }
            if (button == SDL_GAMEPAD_BUTTON_BACK || button == SDL_GAMEPAD_BUTTON_EAST) {
                return SDLK_ESCAPE;
            }
        }
        if (button == SDL_GAMEPAD_BUTTON_START) {
            return SDLK_F2;
        }
        if (button == SDL_GAMEPAD_BUTTON_BACK) {
            return SDLK_ESCAPE;
        }
        return std::nullopt;
    }

    switch (button) {
    case SDL_GAMEPAD_BUTTON_DPAD_UP:
        return SDLK_UP;
    case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
        return SDLK_DOWN;
    case SDL_GAMEPAD_BUTTON_DPAD_LEFT:
        return SDLK_LEFT;
    case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:
        return SDLK_RIGHT;
    case SDL_GAMEPAD_BUTTON_SOUTH:
    case SDL_GAMEPAD_BUTTON_START:
        return SDLK_RETURN;
    case SDL_GAMEPAD_BUTTON_EAST:
    case SDL_GAMEPAD_BUTTON_BACK:
        return SDLK_ESCAPE;
    default:
        return std::nullopt;
    }
}
