#pragma once

// Internal App.cpp implementation shard.
// Main fight update loop and freeze-watch include wiring.

void updateFight(AppState& state) {
    tickFightRuntimeControllerTracking(state);

    if (state.frontend.pendingMode == PendingMode::Story) {
        FramePerfScope scope(state.framePerf, FramePerfSection::StoryArenaRuntime);
        updateStoryFight(state);
        return;
    }

    if (state.frontend.pendingMode == PendingMode::Arena) {
        FramePerfScope scope(state.framePerf, FramePerfSection::StoryArenaRuntime);
        updateArenaFight(state);
        return;
    }

    const StageSlot fallbackStage;
    const StageSlot& stage = selectedStageSlot(state.selection) ? *selectedStageSlot(state.selection) : fallbackStage;
    auto& p1 = state.fighters[0];
    auto& p2 = state.fighters[1];
    const int p1StateNoAtFrameStart = p1.stateNo;
    const int p2StateNoAtFrameStart = p2.stateNo;

    updateRuntimeEffects(state);
    clearFightAssertSpecialFlags(state);
    updateFightAssertSpecialControllers(state, stage);
    resetFighterOneTickBounds(state);
    updateTrainingCommandPracticeTimers(state);

    if (isMatchMode(state) && state.matchPhase == MatchPhase::RoundStart) {
        updateRoundStartWorld(state, stage);
        updateSingleFightPhaseTimers(state);
        updateCamera(state, stage);
        return;
    }

    if (isMatchMode(state) && state.matchPhase == MatchPhase::RoundFinish) {
        updateSingleFightRoundFinishWorld(state, stage);
        updateSingleFightPhaseTimers(state);
        return;
    }

    if (isMatchMode(state)
        && (state.matchPhase == MatchPhase::RoundResult || state.matchPhase == MatchPhase::MatchResult)) {
        updateSingleFightPhaseTimers(state);
        updateCamera(state, stage);
        return;
    }

    const bool* keys = gFightInputOverride ? nullptr : SDL_GetKeyboardState(nullptr);
    updateFighterFacing(state);

    const FighterInputState liveP1Input = gFightInputOverride && gFightInputOverride->p1
        ? *gFightInputOverride->p1
        : collectMappedFighterInput(keys, controlProfileForPlayer(state, 0), assignedGamepad(state, 0));
    const FighterInputState neutralP1Input;
    const FighterInputState& p1Input = trainingCommandDemoActive(state) ? neutralP1Input : liveP1Input;
    if (fighterCanUpdateDuringGlobalPause(state, 0)) {
        const FighterState p1BeforeUpdate = p1;
        updateControlledFighter(state, p1, &p2, p1Input);
        updateTrainingCommandPracticeProgress(state, p1BeforeUpdate, p1, &p2);
    }
    if (!fighterCanUpdateDuringGlobalPause(state, 1)) {
        p2.vx = 0.0f;
        p2.vy = 0.0f;
    } else if (usesLocalP2Controls(state)) {
        const FighterInputState p2Input = gFightInputOverride && gFightInputOverride->p2
            ? *gFightInputOverride->p2
            : collectMappedFighterInput(keys, controlProfileForPlayer(state, 1), assignedGamepad(state, 1));
        updateControlledFighter(state, p2, &p1, p2Input);
    } else if (activeOpponentType(state) == OpponentType::Dummy && trainingCommandDemoActive(state)) {
        const FighterInputState demoInput = nextTrainingCommandDemoInput(state, p2);
        int selected = -1;
        const CommandStateEntry* preferredEntry = selectedTrainingCommandEntry(state, &selected);
        updateControlledFighter(state, p2, &p1, demoInput, preferredEntry);
    } else if (activeOpponentType(state) == OpponentType::Dummy) {
        updateTrainingDummy(state, p2);
    } else if (state.suppressArenaCpu) {
        const FighterInputState neutralCpuInput;
        updateControlledFighter(state, p2, &p1, neutralCpuInput);
    } else {
        updateCpuOpponent(state, p2, p1);
    }

    updateStateZeroFromMovement(state, p1);
    updateStateZeroFromMovement(state, p2);
    const bool p1CanUpdate = fighterCanUpdateDuringGlobalPause(state, 0);
    const bool p2CanUpdate = fighterCanUpdateDuringGlobalPause(state, 1);
    if (p1CanUpdate) {
        updateStateMovementControllers(state, p1, &p2, &stage);
        updateStateHelperControllers(state, p1, &p2, &stage);
        updateStateProjectileControllers(state, p1, &p2, &stage);
        updateStateMakeDustControllers(state, p1, &p2, stage);
        updateStateExplodControllers(state, p1, &p2, stage);
    }
    if (p2CanUpdate) {
        updateStateMovementControllers(state, p2, &p1, &stage);
        updateStateHelperControllers(state, p2, &p1, &stage);
        updateStateProjectileControllers(state, p2, &p1, &stage);
        updateStateMakeDustControllers(state, p2, &p1, stage);
        updateStateExplodControllers(state, p2, &p1, stage);
    }
    updateHelperActors(state, stage);
    if (p1CanUpdate) {
        updateStateTargetControllers(state, p1, &p2, &stage);
    }
    if (p2CanUpdate) {
        updateStateTargetControllers(state, p2, &p1, &stage);
    }
    const bool p1ChangedBeforePhysics = p1CanUpdate && updateStateChangeStateControllers(state, p1, &p2, &stage);
    const bool p2ChangedBeforePhysics = p2CanUpdate && updateStateChangeStateControllers(state, p2, &p1, &stage);
    if (p1CanUpdate) {
        updateFighterPhysics(state, p1, stage);
    }
    if (p2CanUpdate) {
        updateFighterPhysics(state, p2, stage);
    }
    updateCommonAirRecoveryState(state, p1);
    updateCommonAirRecoveryState(state, p2);
    updateCommonDizzyState(state, p1);
    updateCommonDizzyState(state, p2);
    if (p1CanUpdate && !p1ChangedBeforePhysics && updateStateChangeStateControllers(state, p1, &p2, &stage) && p1.y >= 0.0f && p1.stateType != 'A') {
        p1.y = 0.0f;
        p1.vy = 0.0f;
        p1.onGround = true;
    }
    if (p2CanUpdate && !p2ChangedBeforePhysics && updateStateChangeStateControllers(state, p2, &p1, &stage) && p2.y >= 0.0f && p2.stateType != 'A') {
        p2.y = 0.0f;
        p2.vy = 0.0f;
        p2.onGround = true;
    }
    if (p1CanUpdate) {
        updateNeutralAirLandingFallback(state, p1);
    }
    if (p2CanUpdate) {
        updateNeutralAirLandingFallback(state, p2);
    }
    applyPlayerPush(state, stage);
    updateFighterFacing(state);
    updateStateZeroFromMovement(state, p1);
    updateStateZeroFromMovement(state, p2);
    updateComboCounterBreaks(state);
    updateRuntimeProjectiles(state, stage);
    if (!globalPauseActive(state) || state.globalPauseOwnerMoveTicks > 0) {
        applyHitIfNeeded(state);
    }
    if (p1CanUpdate) {
        updateStateTargetControllers(state, p1, &p2, &stage);
        updateStateChangeAnimControllers(state, p1, &p2, &stage);
        updateStatePosAddControllers(state, p1, &p2, &stage);
        updateStateCtrlControllers(state, p1);
        updateStateAudioControllers(state, p1, &p2, &stage);
    }
    if (p2CanUpdate) {
        updateStateTargetControllers(state, p2, &p1, &stage);
        updateStateChangeAnimControllers(state, p2, &p1, &stage);
        updateStatePosAddControllers(state, p2, &p1, &stage);
        updateStateCtrlControllers(state, p2);
        updateStateAudioControllers(state, p2, &p1, &stage);
    }
    applyTargetBindings(state);
    updateFightAssertSpecialControllers(state, stage);
    if (state.frontend.pendingMode == PendingMode::Training
        && activeOpponentType(state) == OpponentType::Dummy
        && (state.training.options.dummyAutoLife || state.training.options.dummyInvincible)) {
        p2.life = characterMaxLifeForActor(state, p2);
    }
    updateSingleFightRules(state);
    updateCamera(state, stage);
    applyScreenBounds(state, stage);
    if (state.messages.lastHitTextTicks > 0) {
        --state.messages.lastHitTextTicks;
    }
    updateComboDisplayTimers(state);
    const bool p1EnteredNewStateThisFrame = p1.stateNo != p1StateNoAtFrameStart && p1.stateTime == 0;
    const bool p2EnteredNewStateThisFrame = p2.stateNo != p2StateNoAtFrameStart && p2.stateTime == 0;
    if (!p1EnteredNewStateThisFrame && p1.hitPauseTicks <= 0 && fighterCanUpdateDuringGlobalPause(state, 0)) {
        ++p1.animTick;
        ++p1.stateTime;
        updateAfterImageEffect(p1);
    }
    if (!p2EnteredNewStateThisFrame && p2.hitPauseTicks <= 0 && fighterCanUpdateDuringGlobalPause(state, 1)) {
        ++p2.animTick;
        ++p2.stateTime;
        updateAfterImageEffect(p2);
    }
    updateGlobalPauseTimers(state);
    finishStateIfAnimationEnded(state, p1);
    finishStateIfAnimationEnded(state, p2);
}

#include "FightFreezeWatchRuntime.h"
