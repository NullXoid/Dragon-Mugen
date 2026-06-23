#pragma once

// Internal App.cpp implementation shard.
// Event pump, frame update, presentation helpers, and verification bridge include wiring.

void pumpEvents(SDL_Renderer* renderer, AppState& state) {
    SDL_Event event{};
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_EVENT_QUIT:
            state.running = false;
            break;
        case SDL_EVENT_KEY_DOWN:
            if (!event.key.repeat) {
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

void fixedUpdate(AppState& state) {
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
    }
    updateFightFreezeWatch(state, fightPaused);
    {
        FramePerfScope scope(state.framePerf, FramePerfSection::AudioQueue);
        updateAudioMixer(state);
    }
}

void applyLogicalPresentation(SDL_Renderer* renderer, const AppState& state) {
    SDL_SetRenderLogicalPresentation(renderer, logicalWidth(state), kLogicalHeight, SDL_LOGICAL_PRESENTATION_LETTERBOX);
}

void clearPhysicalFrame(SDL_Renderer* renderer) {
    SDL_SetRenderLogicalPresentation(renderer, 0, 0, SDL_LOGICAL_PRESENTATION_DISABLED);
    SDL_SetRenderViewport(renderer, nullptr);
    setColor(renderer, 10, 12, 16);
    SDL_RenderClear(renderer);
}

#include "AppVerificationBridge.h"
