#pragma once

// Internal App.cpp implementation shard.
// Event pump, frame update, presentation helpers, and verification bridge include wiring.

void applyWindowPresentation(SDL_Window* window, const AppState& state) {
    if (!window) {
        return;
    }

    if (!state.windowPresentation.fullscreen) {
        if (!SDL_SetWindowFullscreen(window, false)) {
            SDL_Log("SDL_SetWindowFullscreen(false) failed: %s", SDL_GetError());
        }
        SDL_SetWindowSize(window, kWindowWidth, kWindowHeight);
        return;
    }

    SDL_SetWindowSize(window, kWindowWidth, kWindowHeight);
    if (!SDL_SetWindowFullscreen(window, true)) {
        SDL_Log("SDL_SetWindowFullscreen(true) failed: %s", SDL_GetError());
    }
}

void setWindowFullscreen(SDL_Window* window, AppState& state, bool fullscreen) {
    state.windowPresentation.fullscreen = fullscreen;
    applyWindowPresentation(window, state);
}

bool handleWindowShortcut(SDL_Window* window, AppState& state, SDL_Keycode key) {
    const SDL_Keymod modifiers = SDL_GetModState();
    const bool altHeld = (modifiers & SDL_KMOD_ALT) != 0;

    if (key == SDLK_F11 || (altHeld && key == SDLK_RETURN)) {
        setWindowFullscreen(window, state, !state.windowPresentation.fullscreen);
        return true;
    }

    if (altHeld && key == SDLK_M) {
        if (state.windowPresentation.fullscreen) {
            setWindowFullscreen(window, state, false);
        }
        if (!SDL_MinimizeWindow(window)) {
            SDL_Log("SDL_MinimizeWindow failed: %s", SDL_GetError());
        }
        return true;
    }

    return false;
}

void pumpEvents(SDL_Window* window, SDL_Renderer* renderer, AppState& state) {
    SDL_Event event{};
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_EVENT_QUIT:
            state.running = false;
            break;
        case SDL_EVENT_KEY_DOWN:
            if (!event.key.repeat) {
                if (handleWindowShortcut(window, state, event.key.key)) {
                    break;
                }
                handleKey(renderer, state, event.key.key);
            }
            break;
        case SDL_EVENT_GAMEPAD_ADDED:
            openGamepadDevice(state, event.gdevice.which);
            break;
        case SDL_EVENT_GAMEPAD_REMOVED:
            closeGamepadDevice(state, event.gdevice.which);
            break;
        case SDL_EVENT_GAMEPAD_REMAPPED:
            refreshGamepadDevice(state, event.gdevice.which);
            break;
        case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
            handleGamepadButton(
                renderer,
                state,
                event.gbutton.which,
                static_cast<SDL_GamepadButton>(event.gbutton.button));
            break;
        case SDL_EVENT_GAMEPAD_AXIS_MOTION:
            handleOptionsGamepadAxis(
                state,
                static_cast<SDL_GamepadAxis>(event.gaxis.axis),
                event.gaxis.value);
            break;
        default:
            break;
        }
    }
}

bool trainingShowSelectHoldContext(const AppState& state) {
    return trainingShowShortcutContext(state);
}

bool p1GamepadSelectHeld(const AppState& state) {
    const GamepadDevice* gamepad = assignedGamepad(state, 0);
    return gamepad
        && gamepad->handle
        && SDL_GetGamepadButton(gamepad->handle, SDL_GAMEPAD_BUTTON_BACK);
}

void updateTrainingShowSelectHold(AppState& state, bool selectHeld) {
    constexpr int kShowCommandSelectHoldFrames = 120;
    if (!trainingShowSelectHoldContext(state)) {
        state.trainingShowSelectHoldTicks = 0;
        state.trainingShowSelectHoldFired = false;
        return;
    }

    if (selectHeld) {
        if (!state.trainingShowSelectHoldFired) {
            ++state.trainingShowSelectHoldTicks;
            if (state.trainingShowSelectHoldTicks >= kShowCommandSelectHoldFrames) {
                beginTrainingCommandDemo(state);
                state.trainingShowSelectHoldFired = true;
            }
        }
        return;
    }

    if (state.trainingShowSelectHoldTicks > 0 && !state.trainingShowSelectHoldFired) {
        cycleSelectedTrainingCommandEntry(state, 1);
    }
    state.trainingShowSelectHoldTicks = 0;
    state.trainingShowSelectHoldFired = false;
}

void updateTrainingShowSelectHold(AppState& state) {
    updateTrainingShowSelectHold(state, p1GamepadSelectHeld(state));
}

void collectFramePerformanceCounters(AppState& state) {
    FramePerfCounters counters;
    counters.fighters = static_cast<int>(state.fighters.size());
    if (state.frontend.pendingMode == PendingMode::Story) {
        counters.activeStoryEnemies = state.story.activeWaveEnemyCount;
    }
    for (const auto& helper : state.helpers) {
        if (!helper.destroyRequested) {
            ++counters.helpers;
        }
    }
    counters.projectiles = static_cast<int>(state.projectiles.size());
    counters.effects = static_cast<int>(state.runtimeEffects.size());
    counters.activeSounds = static_cast<int>(state.audio.activeVoices.size());
    counters.stageBgElements = static_cast<int>(state.stageBackground.size());
    counters.globalPauseTicks = state.globalPauseTicks;
    counters.superpauseTicks = state.globalPauseIsSuper ? state.globalPauseTicks : 0;
    for (const auto& fighter : state.fighters) {
        if (fighter.hitPauseTicks > 0) {
            ++counters.hitpauseActors;
        }
    }
    for (const auto& helper : state.helpers) {
        if (!helper.destroyRequested && helper.hitPauseTicks > 0) {
            ++counters.hitpauseActors;
        }
    }
    state.framePerf.setCounters(counters);
}

void consumeStoryShopDoorTransition(SDL_Renderer* renderer, AppState& state) {
    if (!state.story.pendingShopDoorTransition) {
        return;
    }
    state.story.pendingShopDoorTransition = false;
    state.story.shopDoorAvailable = false;
    playMenuCursorDoneSound(state);
    const int shopBoard = nextStoryRouteBoardNodeIndex(state, state.story.activeBoardNode);
    enterStoryRouteShopDemo(renderer, state, shopBoard);
}

void fixedUpdate(SDL_Renderer* renderer, AppState& state) {
    ++state.frame;
    ++state.frontend.screenFrame;
    updateTrainingShowSelectHold(state);
    if (state.frontend.screenshotFreezeNoticeTicks > 0) {
        --state.frontend.screenshotFreezeNoticeTicks;
    }
    updateShopDemo(state);
    if (state.frontend.screen == Screen::VersusScreen && state.fightSessionPrepared && state.frontend.screenFrame > 120) {
        beginFight(state);
    }
    const bool fightPaused =
        state.frontend.fightPauseOpen
        || state.frontend.screenshotFreeze
        || (state.frontend.pendingMode == PendingMode::Training && state.training.options.menuOpen)
        || (isMatchMode(state) && state.frontend.singleFightPauseOpen);
    if (state.frontend.screen == Screen::FightView && !fightPaused) {
        updateFight(state);
        applyTrainingPowerMode(state);
        consumeStoryShopDoorTransition(renderer, state);
    }
    updateFightFreezeWatch(state, fightPaused);
    {
        FramePerfScope scope(state.framePerf, FramePerfSection::AudioQueue);
        updateAudioMixer(state);
    }
}

void applyLogicalPresentation(SDL_Renderer* renderer, const AppState& state) {
    SDL_SetRenderLogicalPresentation(renderer, logicalWidth(state), logicalHeight(state), SDL_LOGICAL_PRESENTATION_LETTERBOX);
}

void clearPhysicalFrame(SDL_Renderer* renderer) {
    SDL_SetRenderLogicalPresentation(renderer, 0, 0, SDL_LOGICAL_PRESENTATION_DISABLED);
    SDL_SetRenderViewport(renderer, nullptr);
    setColor(renderer, 10, 12, 16);
    SDL_RenderClear(renderer);
}

SDL_Texture* gPresentationFrameTarget = nullptr;
int gPresentationFrameTargetWidth = 0;
int gPresentationFrameTargetHeight = 0;
bool gPresentationFrameTargetActive = false;

void destroyPresentationFrameTarget() {
    if (gPresentationFrameTarget) {
        SDL_DestroyTexture(gPresentationFrameTarget);
        gPresentationFrameTarget = nullptr;
    }
    gPresentationFrameTargetWidth = 0;
    gPresentationFrameTargetHeight = 0;
    gPresentationFrameTargetActive = false;
}

bool ensurePresentationFrameTarget(SDL_Renderer* renderer) {
    const CanvasDimensions target = presentationFrameTargetDimensions();
    if (target.width <= 0 || target.height <= 0) {
        return false;
    }

    if (gPresentationFrameTarget
        && gPresentationFrameTargetWidth == target.width
        && gPresentationFrameTargetHeight == target.height) {
        return true;
    }

    destroyPresentationFrameTarget();
    gPresentationFrameTarget = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGBA32,
        SDL_TEXTUREACCESS_TARGET,
        target.width,
        target.height);
    if (!gPresentationFrameTarget) {
        SDL_Log("SDL_CreateTexture presentation target %dx%d failed: %s", target.width, target.height, SDL_GetError());
        return false;
    }

    SDL_SetTextureScaleMode(gPresentationFrameTarget, SDL_SCALEMODE_LINEAR);
    gPresentationFrameTargetWidth = target.width;
    gPresentationFrameTargetHeight = target.height;
    return true;
}

SDL_FRect centeredOutputRectForTarget(SDL_Renderer* renderer, int targetWidth, int targetHeight) {
    int outputWidth = 0;
    int outputHeight = 0;
    if (!SDL_GetCurrentRenderOutputSize(renderer, &outputWidth, &outputHeight) || outputWidth <= 0 || outputHeight <= 0) {
        return SDL_FRect{
            0.0f,
            0.0f,
            static_cast<float>(std::max(1, targetWidth)),
            static_cast<float>(std::max(1, targetHeight)),
        };
    }

    const float scale = std::min(
        static_cast<float>(outputWidth) / static_cast<float>(std::max(1, targetWidth)),
        static_cast<float>(outputHeight) / static_cast<float>(std::max(1, targetHeight)));
    const float width = static_cast<float>(targetWidth) * scale;
    const float height = static_cast<float>(targetHeight) * scale;
    return SDL_FRect{
        (static_cast<float>(outputWidth) - width) * 0.5f,
        (static_cast<float>(outputHeight) - height) * 0.5f,
        width,
        height,
    };
}

void beginPresentationFrame(SDL_Renderer* renderer, const AppState& state) {
    if (!renderer || !ensurePresentationFrameTarget(renderer)) {
        gPresentationFrameTargetActive = false;
        clearPhysicalFrame(renderer);
        applyLogicalPresentation(renderer, state);
        return;
    }

    if (!SDL_SetRenderTarget(renderer, gPresentationFrameTarget)) {
        SDL_Log("SDL_SetRenderTarget presentation target failed: %s", SDL_GetError());
        gPresentationFrameTargetActive = false;
        clearPhysicalFrame(renderer);
        applyLogicalPresentation(renderer, state);
        return;
    }

    gPresentationFrameTargetActive = true;
    SDL_SetRenderViewport(renderer, nullptr);
    applyLogicalPresentation(renderer, state);
    setColor(renderer, 10, 12, 16);
    SDL_RenderClear(renderer);
}

void presentPresentationFrame(SDL_Renderer* renderer, const AppState& state) {
    (void)state;
    if (!renderer) {
        return;
    }

    if (!gPresentationFrameTargetActive || !gPresentationFrameTarget) {
        SDL_RenderPresent(renderer);
        return;
    }

    gPresentationFrameTargetActive = false;
    SDL_SetRenderTarget(renderer, nullptr);
    clearPhysicalFrame(renderer);
    SDL_SetTextureScaleMode(gPresentationFrameTarget, SDL_SCALEMODE_LINEAR);
    const SDL_FRect dst = centeredOutputRectForTarget(
        renderer,
        gPresentationFrameTargetWidth,
        gPresentationFrameTargetHeight);
    SDL_RenderTexture(renderer, gPresentationFrameTarget, nullptr, &dst);
    SDL_RenderPresent(renderer);
}

#include "AppVerificationBridge.h"
