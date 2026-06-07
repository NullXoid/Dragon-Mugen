#pragma once

// Internal App.cpp implementation header for the Arena DLC fight loop.
// Include only from App.cpp after the classic fight helpers are defined.

void startArenaResult(AppState& state, int winnerIndex) {
    state.matchPhase = MatchPhase::RoundResult;
    state.matchPhaseTicks = 0;
    state.roundWinner = winnerIndex >= 0 ? winnerIndex + 1 : 0;
    state.roundEndReason = RoundEndReason::Ko;
    state.matchComplete = true;
    state.frontend.selectedMatchResultOption = 0;
    for (size_t i = 0; i < state.fighters.size(); ++i) {
        auto& fighter = state.fighters[i];
        fighter.ctrl = false;
        fighter.vx = 0.0f;
        fighter.vy = 0.0f;
        if (static_cast<int>(i) == winnerIndex) {
            enterStateIfAvailable(state, fighter, 181);
        } else if (fighter.life <= 0) {
            enterStateIfAvailable(state, fighter, 170);
        }
    }
    state.messages.lastHitText = winnerIndex >= 0
        ? state.arenaConfig.winTitle + ": " + arenaFighterName(state, static_cast<size_t>(winnerIndex))
        : "DRAW GAME";
    state.messages.lastHitTextTicks = singleFightRoundResultHoldTicks(state);
}

void updateArenaPhaseTimers(AppState& state) {
    if (state.messages.lastHitTextTicks > 0) {
        --state.messages.lastHitTextTicks;
    }
    updateComboDisplayTimers(state);

    switch (state.matchPhase) {
    case MatchPhase::RoundStart:
        ++state.matchPhaseTicks;
        if (state.matchPhaseTicks >= singleFightRoundStartTotalTicks(state)) {
            state.matchPhase = MatchPhase::Fight;
            state.matchPhaseTicks = 0;
        }
        break;
    case MatchPhase::RoundResult:
        ++state.matchPhaseTicks;
        if (state.matchPhaseTicks >= singleFightRoundResultHoldTicks(state)) {
            state.matchPhase = MatchPhase::MatchResult;
            state.matchPhaseTicks = 0;
            state.frontend.selectedMatchResultOption = 0;
        }
        break;
    case MatchPhase::MatchResult:
        ++state.matchPhaseTicks;
        break;
    case MatchPhase::RoundFinish:
    case MatchPhase::Fight:
    default:
        break;
    }
}

void stopDefeatedArenaFighters(AppState& state) {
    for (auto& fighter : state.fighters) {
        if (fighter.life > 0) {
            continue;
        }
        fighter.life = 0;
        fighter.ctrl = false;
        fighter.vx = 0.0f;
        fighter.vy = 0.0f;
        fighter.targetIndex = -1;
        fighter.targetTicks = 0;
    }
}

void updateArenaFight(AppState& state) {
    const StageSlot fallbackStage;
    const StageSlot& stage = selectedStageSlot(state.selection) ? *selectedStageSlot(state.selection) : fallbackStage;

    updateRuntimeEffects(state);
    clearFightAssertSpecialFlags(state);
    updateFightAssertSpecialControllers(state, stage);
    resetFighterOneTickBounds(state);

    if (state.matchPhase == MatchPhase::RoundStart) {
        updateArenaPhaseTimers(state);
        updateArenaCamera(state, stage);
        return;
    }
    if (state.matchPhase == MatchPhase::RoundResult || state.matchPhase == MatchPhase::MatchResult) {
        updateArenaPhaseTimers(state);
        updateArenaCamera(state, stage);
        return;
    }

    const bool* keys = gFightInputOverride ? nullptr : SDL_GetKeyboardState(nullptr);
    updateArenaFighterFacing(state);
    stopDefeatedArenaFighters(state);

    std::vector<int> stateNoAtFrameStart;
    stateNoAtFrameStart.reserve(state.fighters.size());
    for (const auto& fighter : state.fighters) {
        stateNoAtFrameStart.push_back(fighter.stateNo);
    }

    if (!state.fighters.empty() && state.fighters[0].life > 0 && fighterCanUpdateDuringGlobalPause(state, 0)) {
        const int targetIndex = nearestLivingEnemyIndex(state, 0);
        FighterState* target = targetIndex >= 0 ? &state.fighters[static_cast<size_t>(targetIndex)] : nullptr;
        const FighterInputState p1Input = gFightInputOverride && gFightInputOverride->p1
            ? *gFightInputOverride->p1
            : collectFighterInput(keys, p1Controls(), assignedGamepad(state, 0));
        updateControlledFighter(state, state.fighters[0], target, p1Input);
    }

    for (size_t i = 1; i < state.fighters.size(); ++i) {
        auto& fighter = state.fighters[i];
        const int targetIndex = nearestLivingEnemyIndex(state, static_cast<int>(i));
        if (fighter.life <= 0 || targetIndex < 0 || !fighterCanUpdateDuringGlobalPause(state, static_cast<int>(i))) {
            fighter.vx = 0.0f;
            fighter.vy = 0.0f;
            continue;
        }
        updateCpuOpponent(state, fighter, state.fighters[static_cast<size_t>(targetIndex)]);
    }

    for (auto& fighter : state.fighters) {
        updateStateZeroFromMovement(state, fighter);
    }

    std::vector<bool> canUpdate(state.fighters.size(), false);
    for (size_t i = 0; i < state.fighters.size(); ++i) {
        canUpdate[i] = state.fighters[i].life > 0 && fighterCanUpdateDuringGlobalPause(state, static_cast<int>(i));
        if (!canUpdate[i]) {
            continue;
        }
        const int targetIndex = nearestLivingEnemyIndex(state, static_cast<int>(i));
        FighterState* target = targetIndex >= 0 ? &state.fighters[static_cast<size_t>(targetIndex)] : nullptr;
        updateStateMovementControllers(state, state.fighters[i], target, &stage);
        updateStateHelperControllers(state, state.fighters[i], target, &stage);
        updateStateProjectileControllers(state, state.fighters[i], target, &stage);
        updateStateMakeDustControllers(state, state.fighters[i], target, stage);
        updateStateExplodControllers(state, state.fighters[i], target, stage);
    }

    updateHelperActors(state, stage);

    std::vector<bool> changedBeforePhysics(state.fighters.size(), false);
    for (size_t i = 0; i < state.fighters.size(); ++i) {
        if (!canUpdate[i]) {
            continue;
        }
        const int targetIndex = nearestLivingEnemyIndex(state, static_cast<int>(i));
        FighterState* target = targetIndex >= 0 ? &state.fighters[static_cast<size_t>(targetIndex)] : nullptr;
        changedBeforePhysics[i] = updateStateChangeStateControllers(state, state.fighters[i], target, &stage);
        updateFighterPhysics(state, state.fighters[i], stage);
        updateCommonAirRecoveryState(state, state.fighters[i]);
        updateCommonDizzyState(state, state.fighters[i]);
        if (!changedBeforePhysics[i]
            && updateStateChangeStateControllers(state, state.fighters[i], target, &stage)
            && state.fighters[i].y >= 0.0f
            && state.fighters[i].stateType != 'A') {
            state.fighters[i].y = 0.0f;
            state.fighters[i].vy = 0.0f;
            state.fighters[i].onGround = true;
        }
    }

    applyArenaPlayerPush(state, stage);
    updateArenaFighterFacing(state);
    for (auto& fighter : state.fighters) {
        updateStateZeroFromMovement(state, fighter);
    }
    updateComboCounterBreaks(state);
    updateRuntimeProjectiles(state, stage);
    if (!globalPauseActive(state) || state.globalPauseOwnerMoveTicks > 0) {
        applyArenaHitIfNeeded(state);
    }

    for (size_t i = 0; i < state.fighters.size(); ++i) {
        if (!canUpdate[i] || state.fighters[i].life <= 0) {
            continue;
        }
        const int targetIndex = nearestLivingEnemyIndex(state, static_cast<int>(i));
        FighterState* target = targetIndex >= 0 ? &state.fighters[static_cast<size_t>(targetIndex)] : nullptr;
        updateStateTargetControllers(state, state.fighters[i], target, &stage);
        updateStateChangeAnimControllers(state, state.fighters[i], target, &stage);
        updateStatePosAddControllers(state, state.fighters[i], target, &stage);
        updateStateCtrlControllers(state, state.fighters[i]);
        updateStateAudioControllers(state, state.fighters[i], target, &stage);
    }

    updateFightAssertSpecialControllers(state, stage);
    stopDefeatedArenaFighters(state);
    if (livingArenaFighterCount(state) <= 1) {
        startArenaResult(state, arenaWinnerIndex(state));
        updateArenaCamera(state, stage);
        return;
    }

    updateArenaCamera(state, stage);
    applyScreenBounds(state, stage);
    if (state.messages.lastHitTextTicks > 0) {
        --state.messages.lastHitTextTicks;
    }
    updateComboDisplayTimers(state);

    for (size_t i = 0; i < state.fighters.size(); ++i) {
        const bool enteredNewState = i < stateNoAtFrameStart.size()
            && state.fighters[i].stateNo != stateNoAtFrameStart[i]
            && state.fighters[i].stateTime == 0;
        if (!enteredNewState
            && state.fighters[i].hitPauseTicks <= 0
            && state.fighters[i].life > 0
            && fighterCanUpdateDuringGlobalPause(state, static_cast<int>(i))) {
            ++state.fighters[i].animTick;
            ++state.fighters[i].stateTime;
            updateAfterImageEffect(state.fighters[i]);
        }
    }
    updateGlobalPauseTimers(state);
    for (auto& fighter : state.fighters) {
        if (fighter.life > 0) {
            finishStateIfAnimationEnded(state, fighter);
        }
    }
}
