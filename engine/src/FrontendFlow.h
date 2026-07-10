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

FrontendKey p2CharacterSelectKeyFromSdl(SDL_Keycode key) {
    switch (key) {
    case SDLK_I:
        return FrontendKey::Up;
    case SDLK_K:
        return FrontendKey::Down;
    case SDLK_J:
        return FrontendKey::Left;
    case SDLK_L:
        return FrontendKey::Right;
    case SDLK_SEMICOLON:
        return FrontendKey::Accept;
    default:
        return FrontendKey::Other;
    }
}

FrontendKey frontendKeyFromGamepadButton(SDL_GamepadButton button) {
    switch (button) {
    case SDL_GAMEPAD_BUTTON_DPAD_UP:
        return FrontendKey::Up;
    case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
        return FrontendKey::Down;
    case SDL_GAMEPAD_BUTTON_DPAD_LEFT:
        return FrontendKey::Left;
    case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:
        return FrontendKey::Right;
    case SDL_GAMEPAD_BUTTON_SOUTH:
    case SDL_GAMEPAD_BUTTON_START:
        return FrontendKey::Accept;
    case SDL_GAMEPAD_BUTTON_EAST:
    case SDL_GAMEPAD_BUTTON_BACK:
        return FrontendKey::Escape;
    default:
        return FrontendKey::Other;
    }
}

int gamepadPlayerIndexForId(const AppState& state, SDL_JoystickID id) {
    for (int playerIndex = 0; playerIndex < 2; ++playerIndex) {
        if (const GamepadDevice* gamepad = assignedGamepad(state, playerIndex)) {
            if (gamepad->id == id) {
                return playerIndex;
            }
        }
    }
    return -1;
}

bool isTrainingShowShortcutButton(const AppState& state, int playerIndex, SDL_GamepadButton button) {
    return gamepadButtonMatchesControlAction(state, playerIndex, button, InputAction::TrainingShow);
}

bool trainingShowShortcutContext(const AppState& state) {
    return state.frontend.screen == Screen::FightView
        && state.frontend.pendingMode == PendingMode::Training
        && !state.frontend.fightPauseOpen
        && !state.frontend.screenshotFreeze
        && !state.training.options.menuOpen
        && state.training.options.showCommandHud
        && !trainingCommandDemoActive(state);
}

bool triggerTrainingShowShortcut(AppState& state) {
    if (!trainingShowShortcutContext(state)) {
        return false;
    }
    beginTrainingCommandDemo(state);
    state.trainingShowSelectHoldTicks = 0;
    state.trainingShowSelectHoldFired = true;
    return true;
}

bool startPauseKey(const AppState& state, SDL_Keycode key) {
    return keyMatchesControlAction(state, key, 0, InputAction::Pause)
        || key == SDLK_RETURN
        || key == SDLK_KP_ENTER
        || key == SDLK_PAUSE;
}

bool fightOptionsKey(SDL_Keycode key) {
    return key == SDLK_F2;
}

void openLightFightPause(AppState& state) {
    state.frontend.fightPauseOpen = true;
    state.frontend.screenshotFreeze = false;
    state.frontend.screenshotFreezeNoticeTicks = 0;
    playMenuCursorDoneSound(state);
}

void resumeLightFightPause(AppState& state) {
    state.frontend.fightPauseOpen = false;
    playMenuCursorDoneSound(state);
}

void openFightOptionsFromPause(AppState& state) {
    state.frontend.fightPauseOpen = false;
    if (state.frontend.pendingMode == PendingMode::Training) {
        state.training.options.menuOpen = true;
        state.training.options.moveListOpen = false;
        playMenuCursorDoneSound(state);
        return;
    }
    if (isMatchMode(state)) {
        state.frontend.singleFightPauseOpen = true;
        state.frontend.selectedSingleFightPauseOption = 0;
        playMenuCursorDoneSound(state);
    }
}

void toggleScreenshotFreeze(AppState& state) {
    state.frontend.screenshotFreeze = !state.frontend.screenshotFreeze;
    state.frontend.fightPauseOpen = false;
    state.frontend.screenshotFreezeNoticeTicks = state.frontend.screenshotFreeze ? 75 : 45;
    playMenuCursorDoneSound(state);
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

void resetSingleFightCharacterConfirms(AppState& state) {
    state.selection.p1CharacterConfirmed = false;
    state.selection.p2CharacterConfirmed = false;
}

void finishSingleFightCharacterSelect(AppState& state) {
    configureFightSessionSlotsFromSelection(state);
    selectPreferredStage(state);
    state.frontend.screen = Screen::StageSelect;
}

bool handleSingleFightCharacterSelectKey(AppState& state, int playerIndex, FrontendKey frontendKey) {
    if (frontendKey == FrontendKey::Other) {
        return false;
    }

    const int characterCount = static_cast<int>(state.selection.characters.size());
    if (frontendKey == FrontendKey::Escape) {
        unloadCharacterRuntime(state);
        resetSingleFightCharacterConfirms(state);
        state.frontend.screen = Screen::ModeSelect;
        playMenuCancelSound(state);
        return true;
    }

    if (playerIndex < 0 || playerIndex > 1 || characterCount <= 0) {
        return true;
    }

    int& selected = playerIndex == 0 ? state.selection.selectedCharacter : state.selection.selectedP2Character;
    bool& confirmed = playerIndex == 0 ? state.selection.p1CharacterConfirmed : state.selection.p2CharacterConfirmed;
    const int previousSelection = selected;
    selected = moveCharacterCursor(selected, characterCount, frontendKey);
    if (selected != previousSelection) {
        confirmed = false;
        playMenuCursorMoveSound(state);
    }

    if (frontendKey == FrontendKey::Accept) {
        selected = safeCharacterIndex(state.selection, selected);
        confirmed = true;
        playMenuCursorDoneSound(state);
    }

    if (state.selection.p1CharacterConfirmed && state.selection.p2CharacterConfirmed) {
        finishSingleFightCharacterSelect(state);
    }
    return true;
}

#include "FrontendMatchResultFlow.inl"

void handleKey(SDL_Renderer* renderer, AppState& state, SDL_Keycode key) {
    if (isMatchMode(state) && state.matchPhase == MatchPhase::MatchResult) {
        handleMatchResultKey(renderer, state, key);
        return;
    }

    if (state.frontend.screen == Screen::ModeSelect) {
        const FrontendKey frontendKey = frontendKeyFromSdl(key);
        if (state.frontend.exitConfirmOpen) {
            if (frontendKey == FrontendKey::Escape) {
                state.frontend.exitConfirmOpen = false;
                playMenuCancelSound(state);
            } else if (frontendKey == FrontendKey::Accept) {
                playMenuCursorDoneSound(state);
                state.running = false;
            } else {
                const int previousSelection = state.frontend.selectedMode;
                state.frontend.selectedMode = moveMainMenuSelection(state.frontend.selectedMode, frontendKey);
                if (state.frontend.selectedMode != previousSelection) {
                    state.frontend.exitConfirmOpen = false;
                    playMenuCursorMoveSound(state);
                }
            }
            return;
        }

        const int previousSelection = state.frontend.selectedMode;
        state.frontend.selectedMode = moveMainMenuSelection(state.frontend.selectedMode, frontendKey);
        if (state.frontend.selectedMode != previousSelection) {
            playMenuCursorMoveSound(state);
        }
        FrontendAction action;
        if (frontendKey == FrontendKey::Escape) {
            action = { FrontendActionKind::ExitApp };
        } else if (frontendKey == FrontendKey::Accept) {
            action = decideMainMenuAction(state.frontend.selectedMode);
        }

        switch (action.kind) {
        case FrontendActionKind::ExitApp:
            state.frontend.exitConfirmOpen = true;
            if (frontendKey == FrontendKey::Escape) {
                playMenuCancelSound(state);
            } else {
                playMenuCursorDoneSound(state);
            }
            break;
        case FrontendActionKind::OpenMode:
            unloadCharacterRuntime(state);
            playMenuCursorDoneSound(state);
            state.frontend.pendingMode = action.mode;
            resetSingleFightCharacterConfirms(state);
            if (action.mode == PendingMode::SingleFight) {
                state.selection.selectedP2Character =
                    defaultP2CharacterIndex(state.selection, state.selection.selectedCharacter);
            } else if (action.mode == PendingMode::Arena) {
                setArenaDefaultsFromConfig(state);
                selectArenaDefaultStage(state);
            } else if (action.mode == PendingMode::Story) {
                selectStoryDefaultBoardNode(state);
            }
            state.frontend.screen = Screen::CharacterSelect;
            break;
        case FrontendActionKind::OpenShopDemo:
            unloadCharacterRuntime(state);
            state.story.resumeRouteAfterShop = false;
            state.story.resumeBoardNodeAfterShop = -1;
            playMenuCursorDoneSound(state);
            enterShopDemo(renderer, state);
            break;
        case FrontendActionKind::OpenOptions:
            playMenuCursorDoneSound(state);
            enterOptionsScreen(state, OptionsMenuScreen::Root);
            state.frontend.screen = Screen::MainSettings;
            break;
        default:
            break;
        }
        return;
    }

    if (state.frontend.screen == Screen::MainSettings) {
        handleOptionsKey(state, key);
        return;
    }

    if (state.frontend.screen == Screen::ShopDemo) {
        handleShopDemoKey(renderer, state, key);
        return;
    }

    if (state.frontend.screen == Screen::CharacterSelect) {
        const int characterCount = static_cast<int>(state.selection.characters.size());
        const FrontendKey frontendKey = frontendKeyFromSdl(key);
        if (state.frontend.pendingMode == PendingMode::SingleFight) {
            if (handleSingleFightCharacterSelectKey(state, 0, frontendKey)) {
                return;
            }
            handleSingleFightCharacterSelectKey(state, 1, p2CharacterSelectKeyFromSdl(key));
            return;
        }

        state.selection.selectedCharacter = moveCharacterCursor(state.selection.selectedCharacter, characterCount, frontendKey);
        const FrontendAction action = decideCharacterSelectAction(state.selection.selectedCharacter, characterCount, frontendKey);
        switch (action.kind) {
        case FrontendActionKind::BackToMain:
            unloadCharacterRuntime(state);
            state.frontend.exitConfirmOpen = false;
            state.frontend.screen = Screen::ModeSelect;
            break;
        case FrontendActionKind::CharacterChosen:
            state.selection.selectedCharacter = action.index;
            if (state.frontend.pendingMode == PendingMode::Arena) {
                state.selection.sessionSlots.p1Character = action.index;
                state.selection.sessionSlots.opponentType = OpponentType::Cpu;
                selectArenaDefaultStage(state);
                state.frontend.selectedArenaSetupOption = 0;
                state.frontend.screen = Screen::ArenaSetup;
                break;
            }
            if (state.frontend.pendingMode == PendingMode::Story) {
                state.selection.sessionSlots.p1Character = action.index;
                state.selection.sessionSlots.opponentType = OpponentType::Cpu;
                selectStoryDefaultBoardNode(state);
                configureFightSessionSlotsFromSelection(state);
                state.frontend.screen = Screen::StageSelect;
                break;
            }
            configureFightSessionSlotsFromSelection(state);
            selectPreferredStage(state);
            state.frontend.screen = Screen::StageSelect;
            break;
        default:
            break;
        }
        return;
    }

    if (state.frontend.screen == Screen::ArenaSetup) {
        const FrontendKey frontendKey = frontendKeyFromSdl(key, true);
        const auto startArenaMatch = [&]() {
            configureFightSessionSlotsFromSelection(state);
            unloadCharacterRuntime(state);
            state.frontend.screen = Screen::VersusScreen;
            state.frontend.screenFrame = 0;
            state.fightSessionPrepared = false;
            state.fightSessionLoadFailed = false;
            startLoadingProgress(state.loadingProgress, "Waiting to load");
            playMenuCursorDoneSound(state);
        };
        const auto backToFighterSelect = [&]() {
            unloadCharacterRuntime(state);
            state.frontend.screen = Screen::CharacterSelect;
            state.frontend.screenFrame = 0;
            playMenuCancelSound(state);
        };

        if (frontendKey == FrontendKey::Up) {
            state.frontend.selectedArenaSetupOption =
                (state.frontend.selectedArenaSetupOption + kArenaSetupOptionCount - 1) % kArenaSetupOptionCount;
            playMenuCursorMoveSound(state);
        } else if (frontendKey == FrontendKey::Down) {
            state.frontend.selectedArenaSetupOption =
                (state.frontend.selectedArenaSetupOption + 1) % kArenaSetupOptionCount;
            playMenuCursorMoveSound(state);
        } else if (frontendKey == FrontendKey::Escape) {
            backToFighterSelect();
        } else if (state.frontend.selectedArenaSetupOption == 0
            && (frontendKey == FrontendKey::Left || frontendKey == FrontendKey::Right || frontendKey == FrontendKey::Accept)) {
            setArenaCpuCount(state, arenaCpuCount(state) + (frontendKey == FrontendKey::Left ? -1 : 1));
            playMenuCursorMoveSound(state);
        } else if (state.frontend.selectedArenaSetupOption >= 1
            && state.frontend.selectedArenaSetupOption <= 3
            && (frontendKey == FrontendKey::Left || frontendKey == FrontendKey::Right || frontendKey == FrontendKey::Accept)) {
            const int slot = state.frontend.selectedArenaSetupOption - 1;
            if (slot < arenaCpuCount(state)) {
                cycleArenaCpuCharacter(state, slot, frontendKey == FrontendKey::Left ? -1 : 1);
                playMenuCursorMoveSound(state);
            }
        } else if (state.frontend.selectedArenaSetupOption == 4
            && (frontendKey == FrontendKey::Left || frontendKey == FrontendKey::Right || frontendKey == FrontendKey::Accept)) {
            const int stageCount = static_cast<int>(state.selection.stages.size());
            if (stageCount > 0) {
                const int direction = frontendKey == FrontendKey::Left ? -1 : 1;
                state.selection.selectedStage = (state.selection.selectedStage + direction + stageCount) % stageCount;
                playMenuCursorMoveSound(state);
            }
        } else if (state.frontend.selectedArenaSetupOption == 5
            && (frontendKey == FrontendKey::Left || frontendKey == FrontendKey::Right || frontendKey == FrontendKey::Accept)) {
            cycleArenaTimer(state, frontendKey == FrontendKey::Left ? -1 : 1);
            playMenuCursorMoveSound(state);
        } else if (state.frontend.selectedArenaSetupOption == 6
            && (frontendKey == FrontendKey::Left || frontendKey == FrontendKey::Right || frontendKey == FrontendKey::Accept)) {
            state.selection.sessionSlots.arenaZAxisEnabled = !state.selection.sessionSlots.arenaZAxisEnabled;
            playMenuCursorMoveSound(state);
        } else if (state.frontend.selectedArenaSetupOption == 7
            && (frontendKey == FrontendKey::Left || frontendKey == FrontendKey::Right || frontendKey == FrontendKey::Accept)) {
            state.selection.sessionSlots.arenaCameraRotationEnabled = !state.selection.sessionSlots.arenaCameraRotationEnabled;
            playMenuCursorMoveSound(state);
        } else if (state.frontend.selectedArenaSetupOption == 8 && frontendKey == FrontendKey::Accept) {
            startArenaMatch();
        } else if (state.frontend.selectedArenaSetupOption == 9
            && (frontendKey == FrontendKey::Accept || frontendKey == FrontendKey::Escape)) {
            backToFighterSelect();
        }
        return;
    }

    if (state.frontend.screen == Screen::StageSelect) {
        const int stageCount = static_cast<int>(state.selection.stages.size());
        const FrontendKey frontendKey = frontendKeyFromSdl(key);
        if (state.frontend.pendingMode == PendingMode::Story) {
            ensureStoryBoardRouteLoaded(state);
            if (frontendKey == FrontendKey::Up || frontendKey == FrontendKey::Down) {
                state.story.difficulty = cycleStoryDifficulty(
                    state.story.difficulty,
                    frontendKey == FrontendKey::Up ? 1 : -1);
                playMenuCursorMoveSound(state);
                return;
            }
            if (frontendKey == FrontendKey::Left || frontendKey == FrontendKey::Right) {
                moveStoryBoardNodeSelection(state, frontendKey == FrontendKey::Left ? -1 : 1);
                playMenuCursorMoveSound(state);
                return;
            }
            if (frontendKey == FrontendKey::Escape) {
                unloadCharacterRuntime(state);
                state.frontend.screen = Screen::CharacterSelect;
                resetSingleFightCharacterConfirms(state);
                playMenuCancelSound(state);
                return;
            }
            if (frontendKey == FrontendKey::Accept) {
                commitSelectedStoryBoardNode(state);
                if (const StoryBoardNode* node = selectedStoryBoardNode(state);
                    node && node->kind == StoryBoardNodeKind::Shop) {
                    playMenuCursorDoneSound(state);
                    enterStoryRouteShopDemo(renderer, state, state.story.activeBoardNode);
                    return;
                }
                if (!characterSlotAt(state.selection, state.selection.sessionSlots.p1Character)) {
                    configureFightSessionSlotsFromSelection(state);
                }
                unloadCharacterRuntime(state);
                state.frontend.screen = Screen::VersusScreen;
                state.frontend.screenFrame = 0;
                state.fightSessionPrepared = false;
                state.fightSessionLoadFailed = false;
                startLoadingProgress(state.loadingProgress, "Waiting to load");
                playMenuCursorDoneSound(state);
                return;
            }
            return;
        }

        if (state.frontend.pendingMode != PendingMode::Story) {
            state.selection.selectedStage = moveStageCursor(state.selection.selectedStage, stageCount, frontendKey);
        }
        const FrontendAction action = decideStageSelectAction(state.selection.selectedStage, stageCount, frontendKey);
        switch (action.kind) {
        case FrontendActionKind::BackToCharacterSelect:
            unloadCharacterRuntime(state);
            state.frontend.screen = Screen::CharacterSelect;
            resetSingleFightCharacterConfirms(state);
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
            startLoadingProgress(state.loadingProgress, "Waiting to load");
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
        if (key == SDLK_F3) {
            state.freezeWatch.visible = !state.freezeWatch.visible;
            return;
        }
        if (key == SDLK_F4) {
            toggleScreenshotFreeze(state);
            return;
        }

        if (state.frontend.fightPauseOpen) {
            if (startPauseKey(state, key)) {
                resumeLightFightPause(state);
            } else if (fightOptionsKey(key)) {
                openFightOptionsFromPause(state);
            } else if (key == SDLK_ESCAPE) {
                state.frontend.fightPauseOpen = false;
                playMenuCancelSound(state);
            } else if (state.frontend.pendingMode == PendingMode::Training && key == SDLK_H) {
                state.frontend.fightPauseOpen = false;
                beginTrainingCommandDemo(state);
            } else if (state.frontend.pendingMode == PendingMode::Training && key == SDLK_PAGEDOWN) {
                cycleSelectedTrainingCommandEntry(state, 1);
            } else if (state.frontend.pendingMode == PendingMode::Training && key == SDLK_PAGEUP) {
                cycleSelectedTrainingCommandEntry(state, -1);
            }
            return;
        }

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
                    if (state.frontend.pendingMode == PendingMode::Arena) {
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
                            resetSingleFightCharacterConfirms(state);
                            state.frontend.screen = Screen::CharacterSelect;
                            state.frontend.screenFrame = 0;
                            break;
                        case 3:
                            state.frontend.singleFightPauseOpen = false;
                            unloadCharacterRuntime(state);
                            state.frontend.screen = Screen::ArenaSetup;
                            state.frontend.screenFrame = 0;
                            break;
                        case 4:
                            state.frontend.singleFightPauseOpen = false;
                            unloadCharacterRuntime(state);
                            state.frontend.exitConfirmOpen = false;
                            state.frontend.screen = Screen::ModeSelect;
                            state.frontend.screenFrame = 0;
                            break;
                        default:
                            break;
                        }
                    } else {
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
                            resetSingleFightCharacterConfirms(state);
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
                            state.frontend.exitConfirmOpen = false;
                            state.frontend.screen = Screen::ModeSelect;
                            state.frontend.screenFrame = 0;
                            break;
                        default:
                            break;
                        }
                    }
                    break;
                default:
                    break;
                }
                return;
            }

            if (startPauseKey(state, key)) {
                openLightFightPause(state);
            } else if (key == SDLK_ESCAPE || key == SDLK_F2) {
                state.frontend.singleFightPauseOpen = true;
                state.frontend.selectedSingleFightPauseOption = 0;
            } else if (key == SDLK_R) {
                resetFightState(state);
            }
            return;
        }

        if (state.training.options.menuOpen) {
            if (state.training.options.moveListOpen) {
                const auto& entries = activeDisplayableMoveListEntries(state);
                const int maxSelected = std::max(0, static_cast<int>(entries.size()) - 1);
                switch (key) {
                case SDLK_ESCAPE:
                case SDLK_F2:
                case SDLK_RETURN:
                case SDLK_KP_ENTER:
                case SDLK_SPACE:
                    state.training.options.moveListOpen = false;
                    break;
                case SDLK_UP:
                    state.training.options.selectedMoveListEntry =
                        std::max(0, state.training.options.selectedMoveListEntry - 1);
                    break;
                case SDLK_DOWN:
                    state.training.options.selectedMoveListEntry =
                        std::min(maxSelected, state.training.options.selectedMoveListEntry + 1);
                    break;
                case SDLK_LEFT:
                case SDLK_Q:
                    setTrainingMoveListTabPreservingSelection(state, TrainingMoveListTab::Main);
                    break;
                case SDLK_RIGHT:
                case SDLK_E:
                    setTrainingMoveListTabPreservingSelection(state, TrainingMoveListTab::All);
                    break;
                case SDLK_H:
                    beginTrainingCommandDemo(state);
                    state.training.options.menuOpen = false;
                    state.training.options.moveListOpen = false;
                    break;
                default:
                    break;
                }
                clampTrainingMoveListSelection(state);
                return;
            }

            switch (key) {
            case SDLK_ESCAPE:
            case SDLK_F2:
                state.training.options.menuOpen = false;
                state.training.options.moveListOpen = false;
                break;
            case SDLK_UP:
                state.training.options.selectedOption =
                    (state.training.options.selectedOption + kTrainingOptionCount - 1) % kTrainingOptionCount;
                break;
            case SDLK_DOWN:
                state.training.options.selectedOption =
                    (state.training.options.selectedOption + 1) % kTrainingOptionCount;
                break;
            case SDLK_LEFT:
            case SDLK_RIGHT:
            case SDLK_RETURN:
            case SDLK_KP_ENTER:
            case SDLK_SPACE:
                if (state.training.options.selectedOption == kTrainingResetOption) {
                    resetTrainingPositions(state);
                } else {
                    const int toggledOption = state.training.options.selectedOption;
                    toggleTrainingOption(state.training.options, state.training.options.selectedOption);
                    if (toggledOption == kTrainingP2ControlOption) {
                        clearFighterHitRuntime(state.fighters[1]);
                        enterState(state, state.fighters[1], 0);
                        state.messages.lastHitText = state.training.options.p2Controlled ? "P2 control enabled" : "P2 dummy enabled";
                        state.messages.lastHitTextTicks = 90;
                    } else if (toggledOption == kTrainingPowerOption) {
                        applyTrainingPowerMode(state);
                        state.messages.lastHitText = state.training.options.powerMode == TrainingPowerMode::Max ? "Training power max" : "Training power normal";
                        state.messages.lastHitTextTicks = 90;
                    }
                }
                break;
            default:
                break;
            }
            return;
        }

        if (startPauseKey(state, key)) {
            openLightFightPause(state);
        } else if (key == SDLK_ESCAPE) {
            unloadCharacterRuntime(state);
            state.frontend.screen = Screen::StageSelect;
            state.frontend.screenFrame = 0;
        } else if (key == SDLK_F1) {
            state.training.options.showHitboxes = !state.training.options.showHitboxes;
        } else if (key == SDLK_F2) {
            state.training.options.menuOpen = true;
        } else if (key == SDLK_R) {
            resetFightState(state);
        } else if (key == SDLK_H) {
            beginTrainingCommandDemo(state);
        } else if (key == SDLK_PAGEDOWN) {
            cycleSelectedTrainingCommandEntry(state, 1);
        } else if (key == SDLK_PAGEUP) {
            cycleSelectedTrainingCommandEntry(state, -1);
        }
        return;
    }
}

std::optional<SDL_Keycode> gamepadMenuKeyForButton(const AppState& state, SDL_GamepadButton button);

void handleGamepadButton(
    SDL_Renderer* renderer,
    AppState& state,
    SDL_JoystickID gamepadId,
    SDL_GamepadButton button) {
    const int playerIndex = gamepadPlayerIndexForId(state, gamepadId);
    if (state.frontend.screen == Screen::MainSettings) {
        const bool captureActive = state.mainSettings.awaitingControlBinding;
        handleOptionsGamepadButton(state, button);
        if (captureActive || state.mainSettings.optionsScreen == OptionsMenuScreen::InputTest) {
            return;
        }
    }
    if (playerIndex == 0 && isTrainingShowShortcutButton(state, playerIndex, button) && triggerTrainingShowShortcut(state)) {
        return;
    }

    if (state.frontend.screen == Screen::CharacterSelect
        && state.frontend.pendingMode == PendingMode::SingleFight) {
        if (playerIndex >= 0) {
            handleSingleFightCharacterSelectKey(state, playerIndex, frontendKeyFromGamepadButton(button));
        }
        return;
    }

    if (state.frontend.screen == Screen::ShopDemo) {
        const int shopPlayerIndex = playerIndex < 0 ? 0 : playerIndex;
        if (gamepadButtonMatchesControlAction(state, shopPlayerIndex, button, InputAction::LK)
            && handleShopDemoShopActionButton(state)) {
            playMenuCursorDoneSound(state);
            return;
        }
    }

    if (const auto key = gamepadMenuKeyForButton(state, button)) {
        handleKey(renderer, state, *key);
    }
}

std::optional<SDL_Keycode> gamepadMenuKeyForButton(const AppState& state, SDL_GamepadButton button) {
    if (isMatchMode(state) && state.matchPhase == MatchPhase::MatchResult) {
        if (button == SDL_GAMEPAD_BUTTON_DPAD_UP) {
            return SDLK_UP;
        }
        if (button == SDL_GAMEPAD_BUTTON_DPAD_DOWN) {
            return SDLK_DOWN;
        }
        if (button == SDL_GAMEPAD_BUTTON_SOUTH || button == SDL_GAMEPAD_BUTTON_START) {
            return SDLK_RETURN;
        }
        if (button == SDL_GAMEPAD_BUTTON_BACK || button == SDL_GAMEPAD_BUTTON_EAST) {
            return SDLK_ESCAPE;
        }
    }

    const bool fightOverlayOpen =
        state.frontend.screen == Screen::FightView
        && (state.frontend.fightPauseOpen
            || (state.frontend.pendingMode == PendingMode::Training && state.training.options.menuOpen)
            || (isMatchMode(state) && state.frontend.singleFightPauseOpen));

    if (state.frontend.screen == Screen::FightView && state.frontend.fightPauseOpen) {
        if (state.frontend.pendingMode == PendingMode::Training) {
            if (isTrainingShowShortcutButton(state, 0, button)) {
                return SDLK_H;
            }
            if (button == SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER) {
                return SDLK_PAGEDOWN;
            }
            if (button == SDL_GAMEPAD_BUTTON_LEFT_SHOULDER) {
                return SDLK_PAGEUP;
            }
        }
        if (gamepadButtonMatchesControlAction(state, 0, button, InputAction::Pause) || button == SDL_GAMEPAD_BUTTON_SOUTH) {
            return SDLK_RETURN;
        }
        if (button == SDL_GAMEPAD_BUTTON_BACK) {
            return SDLK_F2;
        }
        if (button == SDL_GAMEPAD_BUTTON_EAST) {
            return SDLK_ESCAPE;
        }
        return std::nullopt;
    }

    if (state.frontend.screen == Screen::FightView && !fightOverlayOpen) {
        if (isMatchMode(state) && state.matchPhase == MatchPhase::MatchResult) {
            if (button == SDL_GAMEPAD_BUTTON_DPAD_UP) {
                return SDLK_UP;
            }
            if (button == SDL_GAMEPAD_BUTTON_DPAD_DOWN) {
                return SDLK_DOWN;
            }
            if (button == SDL_GAMEPAD_BUTTON_SOUTH || button == SDL_GAMEPAD_BUTTON_START) {
                return SDLK_RETURN;
            }
            if (button == SDL_GAMEPAD_BUTTON_BACK || button == SDL_GAMEPAD_BUTTON_EAST) {
                return SDLK_ESCAPE;
            }
        }
        if (gamepadButtonMatchesControlAction(state, 0, button, InputAction::Pause)) {
            return SDLK_RETURN;
        }
        if (button == SDL_GAMEPAD_BUTTON_BACK && state.frontend.pendingMode == PendingMode::Training) {
            return std::nullopt;
        }
        if (button == SDL_GAMEPAD_BUTTON_BACK) {
            return SDLK_ESCAPE;
        }
        return std::nullopt;
    }

    if (state.frontend.screen == Screen::FightView
        && state.frontend.pendingMode == PendingMode::Training
        && state.training.options.menuOpen
        && state.training.options.moveListOpen) {
        if (button == SDL_GAMEPAD_BUTTON_WEST) {
            return SDLK_H;
        }
        if (button == SDL_GAMEPAD_BUTTON_LEFT_SHOULDER) {
            return SDLK_Q;
        }
        if (button == SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER) {
            return SDLK_E;
        }
    }

    if (state.frontend.screen == Screen::ShopDemo) {
        if (button == SDL_GAMEPAD_BUTTON_LEFT_SHOULDER) {
            return SDLK_Q;
        }
        if (button == SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER) {
            return SDLK_E;
        }
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
