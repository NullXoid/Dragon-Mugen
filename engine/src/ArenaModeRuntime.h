#pragma once

// Internal App.cpp implementation header for the Arena DLC fight loop.
// Include only from App.cpp after the classic fight helpers are defined.

bool arenaFighterHasDefeatMotion(const FighterState& fighter) {
    return fighter.moveType == 'H'
        || fighter.guarding
        || fighter.hitPauseTicks > 0
        || fighter.hitFall
        || fighter.customHitState
        || (fighter.stateNo >= 5000 && fighter.stateNo < 5200)
        || !fighter.onGround
        || fighter.stateType == 'A';
}

bool arenaFighterNeedsForcedDefeatPose(const FighterState& fighter) {
    return fighter.life <= 0
        && !fighter.hitFall
        && (fighter.hitPauseTicks > 0
            || fighter.moveType == 'H'
            || fighter.guarding
            || fighter.stateNo < 5000
            || fighter.stateNo >= 5200);
}

void enterArenaFallbackDefeatPose(const AppState& state, FighterState& fighter, bool force = false) {
    if (!force && arenaFighterHasDefeatMotion(fighter)) {
        return;
    }
    const int action = firstExistingActionForActor(state, fighter, { 5110, 5120, 5100, 5000, 0 });
    fighter.life = 0;
    fighter.ctrl = false;
    fighter.targetIndex = -1;
    fighter.targetTicks = 0;
    fighter.guarding = false;
    fighter.customHitState = false;
    fighter.customStateOwnerIndex = -1;
    fighter.hitPauseTicks = 0;
    fighter.hitSlideTicks = 0;
    fighter.hitStunTicks = std::max(fighter.hitStunTicks, 2);
    fighter.hitFall = action != 0;
    fighter.hitFallRecover = false;
    fighter.hitVelocityX = 0.0f;
    fighter.hitVelocityY = 0.0f;
    fighter.vx = 0.0f;
    fighter.vy = 0.0f;
    fighter.y = 0.0f;
    fighter.onGround = true;
    if (action != 0) {
        fighter.stateNo = action;
        fighter.stateTime = 0;
        fighter.stateType = action >= 5110 ? 'L' : 'S';
        fighter.moveType = 'H';
        fighter.physics = 'N';
        setFighterAction(fighter, action);
    }
}

void holdArenaDefeatedRecoveryPose(const AppState& state, FighterState& fighter) {
    if (fighter.life > 0) {
        return;
    }
    fighter.hitFallRecover = false;
    if (fighter.stateNo == 5120 && findExactClipForActor(state, fighter, 5110)) {
        fighter.stateNo = 5110;
        fighter.stateTime = 0;
        fighter.stateType = 'L';
        fighter.physics = 'N';
        fighter.onGround = true;
        fighter.y = 0.0f;
        fighter.vx = 0.0f;
        fighter.vy = 0.0f;
        setFighterAction(fighter, 5110);
    }
    if (fighter.hitFall && fighter.stateNo == 5110) {
        fighter.hitStunTicks = std::max(fighter.hitStunTicks, 2);
        fighter.vx = 0.0f;
        fighter.vy = 0.0f;
    }
}

void markArenaDefeatedFighters(AppState& state) {
    for (auto& fighter : state.fighters) {
        if (fighter.life > 0) {
            continue;
        }
        fighter.life = 0;
        fighter.ctrl = false;
        fighter.targetIndex = -1;
        fighter.targetTicks = 0;
        enterArenaFallbackDefeatPose(state, fighter, arenaFighterNeedsForcedDefeatPose(fighter));
        holdArenaDefeatedRecoveryPose(state, fighter);
        if (!arenaFighterHasDefeatMotion(fighter)) {
            fighter.vx = 0.0f;
            fighter.vy = 0.0f;
        }
    }
}

void forceArenaDefeatedFightersToGround(AppState& state) {
    for (auto& fighter : state.fighters) {
        if (fighter.life > 0) {
            continue;
        }
        enterArenaFallbackDefeatPose(state, fighter, true);
        holdArenaDefeatedRecoveryPose(state, fighter);
    }
}

void updateArenaDefeatedFighterPose(AppState& state, size_t fighterIndex, const StageSlot& stage) {
    if (fighterIndex >= state.fighters.size()) {
        return;
    }
    auto& fighter = state.fighters[fighterIndex];
    if (fighter.life > 0) {
        return;
    }

    fighter.life = 0;
    fighter.ctrl = false;
    fighter.targetIndex = -1;
    fighter.targetTicks = 0;
    if (arenaFighterNeedsForcedDefeatPose(fighter)) {
        enterArenaFallbackDefeatPose(state, fighter, true);
    }
    if (!fighterCanUpdateDuringGlobalPause(state, static_cast<int>(fighterIndex))) {
        fighter.vx = 0.0f;
        fighter.vy = 0.0f;
        return;
    }

    const int stateNoAtStart = fighter.stateNo;
    if (fighter.guarding) {
        updateGroundGuardState(state, fighter);
    } else if (fighter.moveType == 'H' && !fighter.customHitState) {
        updateGroundGetHitState(state, fighter);
    } else if (!arenaFighterHasDefeatMotion(fighter)) {
        fighter.vx = 0.0f;
        fighter.vy = 0.0f;
    }
    holdArenaDefeatedRecoveryPose(state, fighter);

    updateFighterPhysics(state, fighter, stage);
    resolveArenaFallGrounding(state, fighter);
    updateCommonAirRecoveryState(state, fighter);
    updateCommonDizzyState(state, fighter);
    holdArenaDefeatedRecoveryPose(state, fighter);

    const bool enteredNewState = fighter.stateNo != stateNoAtStart && fighter.stateTime == 0;
    if (!enteredNewState && fighter.hitPauseTicks <= 0) {
        ++fighter.animTick;
        ++fighter.stateTime;
        updateAfterImageEffect(fighter);
    }
}

void startArenaRoundFinish(AppState& state, int winnerIndex, RoundEndReason reason = RoundEndReason::Ko) {
    state.matchPhase = MatchPhase::RoundFinish;
    state.matchPhaseTicks = 0;
    state.roundWinner = winnerIndex >= 0 ? winnerIndex + 1 : 0;
    state.roundEndReason = reason;
    state.matchComplete = true;
    state.frontend.selectedMatchResultOption = 0;
    for (size_t i = 0; i < state.fighters.size(); ++i) {
        auto& fighter = state.fighters[i];
        fighter.ctrl = false;
        if (static_cast<int>(i) == winnerIndex) {
            fighter.vx = 0.0f;
            fighter.vy = 0.0f;
            enterStateIfAvailable(state, fighter, 181);
        } else if (!arenaFighterHasDefeatMotion(fighter) || arenaFighterNeedsForcedDefeatPose(fighter)) {
            enterArenaFallbackDefeatPose(state, fighter, arenaFighterNeedsForcedDefeatPose(fighter));
        }
        holdArenaDefeatedRecoveryPose(state, fighter);
    }
    state.messages.lastHitText = roundFinishCalloutText(state);
    state.messages.lastHitTextTicks = singleFightRoundFinishHoldTicks(state);
}

int arenaTimeOverWinnerIndex(const AppState& state) {
    int winner = -1;
    int bestLife = -1;
    bool tied = false;
    for (size_t i = 0; i < state.fighters.size(); ++i) {
        const int life = state.fighters[i].life;
        if (life <= 0) {
            continue;
        }
        if (life > bestLife) {
            bestLife = life;
            winner = static_cast<int>(i);
            tied = false;
        } else if (life == bestLife) {
            tied = true;
        }
    }
    return tied ? -1 : winner;
}

void updateArenaPhaseTimers(AppState& state) {
    if (state.messages.lastHitTextTicks > 0) {
        --state.messages.lastHitTextTicks;
    }
    updateComboDisplayTimers(state);

    switch (state.matchPhase) {
    case MatchPhase::RoundStart:
        if (anyFighterHasAssertSpecialFlag(state, "intro")) {
            break;
        }
        ++state.matchPhaseTicks;
        if (state.matchPhaseTicks >= singleFightRoundStartTotalTicks(state)) {
            state.matchPhase = MatchPhase::Fight;
            state.matchPhaseTicks = 0;
        }
        break;
    case MatchPhase::RoundFinish:
        ++state.matchPhaseTicks;
        markArenaDefeatedFighters(state);
        if (state.matchPhaseTicks >= singleFightRoundFinishHoldTicks(state)) {
            forceArenaDefeatedFightersToGround(state);
            state.matchPhase = MatchPhase::RoundResult;
            state.matchPhaseTicks = 0;
            state.messages.lastHitText = roundResultText(state);
            state.messages.lastHitTextTicks = singleFightRoundResultHoldTicks(state);
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
    case MatchPhase::Fight:
        if (arenaTimerSeconds(state) > 0
            && state.matchTimerTicks > 0
            && !anyFighterHasAssertSpecialFlag(state, "timerfreeze")) {
            --state.matchTimerTicks;
        }
        if (arenaTimerSeconds(state) > 0
            && state.matchTimerTicks <= 0
            && !anyFighterHasAssertSpecialFlag(state, "roundnotover")
            && !anyFighterHasAssertSpecialFlag(state, "intro")) {
            startArenaRoundFinish(state, arenaTimeOverWinnerIndex(state), RoundEndReason::TimeUp);
        }
        break;
    default:
        break;
    }
}

void updateArenaRoundFinishWorld(AppState& state, const StageSlot& stage) {
    markArenaDefeatedFighters(state);
    updateHelperActors(state, stage);
    for (size_t i = 0; i < state.fighters.size(); ++i) {
        auto& fighter = state.fighters[i];
        fighter.ctrl = false;
        fighter.targetIndex = -1;
        fighter.targetTicks = 0;
        if (fighter.life <= 0) {
            updateArenaDefeatedFighterPose(state, i, stage);
            continue;
        }
        if (!fighterCanUpdateDuringGlobalPause(state, static_cast<int>(i))) {
            fighter.vx = 0.0f;
            fighter.vy = 0.0f;
            continue;
        }
        updateFighterPhysics(state, fighter, stage);
        resolveArenaFallGrounding(state, fighter);
        updateCommonAirRecoveryState(state, fighter);
        updateCommonDizzyState(state, fighter);
        if (fighter.hitPauseTicks <= 0) {
            ++fighter.animTick;
            ++fighter.stateTime;
            updateAfterImageEffect(fighter);
        }
        finishStateIfAnimationEnded(state, fighter);
    }
    updateGlobalPauseTimers(state);
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
    if (state.matchPhase == MatchPhase::RoundFinish) {
        updateArenaRoundFinishWorld(state, stage);
        updateArenaPhaseTimers(state);
        updateArenaCamera(state, stage);
        applyArenaScreenBounds(state, stage);
        return;
    }
    if (state.matchPhase == MatchPhase::RoundResult || state.matchPhase == MatchPhase::MatchResult) {
        updateArenaPhaseTimers(state);
        updateArenaCamera(state, stage);
        return;
    }

    const bool* keys = gFightInputOverride ? nullptr : SDL_GetKeyboardState(nullptr);
    updateArenaFighterFacing(state);
    markArenaDefeatedFighters(state);

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
            if (fighter.life > 0) {
                fighter.vx = 0.0f;
                fighter.vy = 0.0f;
            }
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
        resolveArenaFallGrounding(state, state.fighters[i]);
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
        updateNeutralAirLandingFallback(state, state.fighters[i]);
        resolveArenaFallGrounding(state, state.fighters[i]);
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
    markArenaDefeatedFighters(state);

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
    applyTargetBindings(state);

    for (auto& fighter : state.fighters) {
        updateStateZeroFromMovement(state, fighter);
    }

    updateFightAssertSpecialControllers(state, stage);
    if (livingArenaFighterCount(state) <= 1) {
        startArenaRoundFinish(state, arenaWinnerIndex(state));
        updateArenaCamera(state, stage);
        return;
    }

    for (size_t i = 0; i < state.fighters.size(); ++i) {
        updateArenaDefeatedFighterPose(state, i, stage);
    }

    updateArenaCamera(state, stage);
    applyArenaScreenBounds(state, stage);
    updateArenaPhaseTimers(state);

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
