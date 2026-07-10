bool handleMatchResultKey(SDL_Renderer* renderer, AppState& state, SDL_Keycode key) {
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
        if (state.frontend.pendingMode == PendingMode::Arena) {
            switch (state.frontend.selectedMatchResultOption) {
            case 0:
                resetFightState(state);
                break;
            case 1:
                unloadCharacterRuntime(state);
                state.frontend.screen = Screen::ArenaSetup;
                state.frontend.screenFrame = 0;
                break;
            case 2:
                unloadCharacterRuntime(state);
                resetSingleFightCharacterConfirms(state);
                state.frontend.screen = Screen::CharacterSelect;
                state.frontend.screenFrame = 0;
                break;
            case 3:
                unloadCharacterRuntime(state);
                state.frontend.exitConfirmOpen = false;
                state.frontend.screen = Screen::ModeSelect;
                state.frontend.screenFrame = 0;
                break;
            default:
                break;
            }
        } else if (state.frontend.pendingMode == PendingMode::Story) {
            switch (state.frontend.selectedMatchResultOption) {
            case 0:
                if (storyCanContinueRoute(state)) {
                    const int nextBoard = nextStoryRouteBoardNodeIndex(state, state.story.activeBoardNode);
                    if (const StoryBoardNode* node = storyBoardNodeAt(state, nextBoard);
                        node && node->kind == StoryBoardNodeKind::Shop) {
                        playMenuCursorDoneSound(state);
                        enterStoryRouteShopDemo(renderer, state, nextBoard);
                        break;
                    }
                    state.story.selectedBoardNode = nextBoard;
                    state.story.activeBoardNode = nextBoard;
                    syncStorySelectedStageToBoardNode(state);
                    configureFightSessionSlotsFromSelection(state);
                    unloadCharacterRuntime(state);
                    state.frontend.screen = Screen::VersusScreen;
                    state.frontend.screenFrame = 0;
                    state.fightSessionPrepared = false;
                    state.fightSessionLoadFailed = false;
                    startLoadingProgress(state.loadingProgress, "Waiting to load");
                } else {
                    resetFightState(state);
                }
                break;
            case 1:
                unloadCharacterRuntime(state);
                resetSingleFightCharacterConfirms(state);
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
                state.frontend.exitConfirmOpen = false;
                state.frontend.screen = Screen::ModeSelect;
                state.frontend.screenFrame = 0;
                break;
            default:
                break;
            }
        } else {
            switch (state.frontend.selectedMatchResultOption) {
            case 0:
                resetFightState(state);
                break;
            case 1:
                unloadCharacterRuntime(state);
                resetSingleFightCharacterConfirms(state);
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
                state.frontend.exitConfirmOpen = false;
                state.frontend.screen = Screen::ModeSelect;
                state.frontend.screenFrame = 0;
                break;
            default:
                break;
            }
        }
        break;
    case SDLK_ESCAPE:
    case SDLK_F2:
        unloadCharacterRuntime(state);
        state.frontend.exitConfirmOpen = false;
        state.frontend.screen = Screen::ModeSelect;
        state.frontend.screenFrame = 0;
        break;
    default:
        break;
    }
    return true;
}
