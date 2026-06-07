#pragma once

// Internal App.cpp implementation header.
// This file depends on App.cpp-local AppState, FighterState, loading/reset helpers,
// selected-stage/session helpers, audio state, and fight setup state.
// Include only from App.cpp after the required types/helpers are defined.
// Do not include from other translation units.

void configureFightSessionSlotsFromSelection(AppState& state) {
    const int characterCount = static_cast<int>(state.selection.characters.size());
    state.selection.sessionSlots.p1Character = characterCount > 0
        ? std::clamp(state.selection.selectedCharacter, 0, characterCount - 1)
        : -1;
    state.selection.sessionSlots.opponentType = defaultOpponentTypeForMode(state.frontend.pendingMode);
    if (state.frontend.pendingMode == PendingMode::Arena) {
        setArenaCpuCount(state, state.selection.sessionSlots.arenaCpuCount);
        chooseArenaCpuCharacters(state);
        state.selection.sessionSlots.opponentCharacter = -1;
    } else if (state.frontend.pendingMode == PendingMode::SingleFight) {
        if (!characterSlotAt(state.selection, state.selection.selectedP2Character)) {
            state.selection.selectedP2Character =
                defaultP2CharacterIndex(state.selection, state.selection.sessionSlots.p1Character);
        }
        state.selection.sessionSlots.opponentCharacter =
            safeCharacterIndex(state.selection, state.selection.selectedP2Character);
    } else {
        state.selection.sessionSlots.opponentCharacter = -1;
    }
}

void enterRoundInitialState(const AppState& state, FighterState& fighter) {
    enterState(state, fighter, findStateDefinition(state, 5900) ? 5900 : 0);
}

void clearFighterVariables(FighterState& fighter) {
    fighter.vars.fill(0);
    fighter.sysVars.fill(0);
    fighter.fvars.fill(0.0f);
    fighter.sysFvars.fill(0.0f);
}

void clearGlobalPause(AppState& state) {
    state.globalPauseTicks = 0;
    state.globalPauseOwnerIndex = -1;
    state.globalPauseOwnerMoveTicks = 0;
    state.globalPauseIsSuper = false;
}

void clearEnvShake(AppState& state) {
    state.display.envShakeTicks = 0;
    state.display.envShakeTotalTicks = 0;
    state.display.envShakeFrequency = 60;
    state.display.envShakeAmplitude = 0.0f;
    state.display.envShakePhase = 0;
    state.display.envShakeOffsetY = 0.0f;
}

void clearPaletteRuntime(AppState& state) {
    state.backgroundPaletteEffect = {};
    state.envColor = {};
    for (auto& fighter : state.fighters) {
        fighter.paletteEffect = {};
    }
}

void resetTrainingPositions(AppState& state) {
    const StageSlot fallbackStage;
    const StageSlot& stage = selectedStageSlot(state.selection) ? *selectedStageSlot(state.selection) : fallbackStage;

    state.training.commandDemo = {};
    state.fighters[0].x = stage.p1startx;
    state.fighters[0].y = stage.p1starty;
    state.fighters[1].x = stage.p2startx;
    state.fighters[1].y = stage.p2starty;
    state.fighters[0].inputHistory.clear();
    state.fighters[1].inputHistory.clear();
    clearFighterHitRuntime(state.fighters[0]);
    clearFighterHitRuntime(state.fighters[1]);
    clearFighterVariables(state.fighters[0]);
    clearFighterVariables(state.fighters[1]);
    state.fighters[0].power = 0;
    state.fighters[1].power = 0;
    applyTrainingPowerMode(state);
    state.fighters[0].hitCount = 0;
    state.fighters[1].hitCount = 0;
    state.fighters[0].defenceMultiplier = 1.0f;
    state.fighters[1].defenceMultiplier = 1.0f;
    state.fighters[0].attackMultiplier = 1.0f;
    state.fighters[1].attackMultiplier = 1.0f;
    state.fighters[0].drawAngle = 0.0f;
    state.fighters[1].drawAngle = 0.0f;
    state.fighters[0].angleDrawActive = false;
    state.fighters[1].angleDrawActive = false;
    state.fighters[0].displayOffsetX = 0.0f;
    state.fighters[1].displayOffsetX = 0.0f;
    state.fighters[0].displayOffsetY = 0.0f;
    state.fighters[1].displayOffsetY = 0.0f;
    state.fighters[0].debugClipboard.clear();
    state.fighters[1].debugClipboard.clear();
    state.fighters[0].victoryQuote = -1;
    state.fighters[1].victoryQuote = -1;
    state.fighters[0].paletteRemaps.clear();
    state.fighters[1].paletteRemaps.clear();
    state.fighters[0].paletteNo = 1;
    state.fighters[1].paletteNo = 1;
    state.fighters[0].attackDistanceOverride = -1;
    state.fighters[1].attackDistanceOverride = -1;
    state.fighters[0].facing = state.fighters[0].x <= state.fighters[1].x ? 1 : -1;
    state.fighters[1].facing = -state.fighters[0].facing;
    state.cameraX = stage.cameraStartx;
    state.cameraY = stage.cameraStarty;
    state.runtimeEffects.clear();
    state.helpers.clear();
    state.projectiles.clear();
    clearGlobalPause(state);
    clearEnvShake(state);
    clearPaletteRuntime(state);
    clearComboCounters(state);
    enterRoundInitialState(state, state.fighters[0]);
    enterRoundInitialState(state, state.fighters[1]);
    state.messages.lastHitText = "Training positions reset";
    state.messages.lastHitTextTicks = 90;
}

void resetFightRound(AppState& state) {
    const StageSlot fallbackStage;
    const StageSlot& stage = selectedStageSlot(state.selection) ? *selectedStageSlot(state.selection) : fallbackStage;

    if (state.frontend.pendingMode == PendingMode::Arena) {
        const int fighterCount = std::clamp(arenaFighterCount(state), 2, 4);
        state.fighters.assign(static_cast<size_t>(fighterCount), FighterState{});
        state.helpers.clear();
        state.projectiles.clear();

        const std::array<float, 4> xPositions{
            stage.p1startx,
            stage.p2startx,
            stage.p1startx - 58.0f,
            stage.p2startx + 58.0f,
        };
        const std::array<float, 4> yPositions{
            stage.p1starty,
            stage.p2starty,
            stage.p1starty,
            stage.p2starty,
        };
        for (size_t i = 0; i < state.fighters.size(); ++i) {
            auto& fighter = state.fighters[i];
            fighter.x = clampFighterOriginToStage(xPositions[i], stage);
            fighter.y = yPositions[i];
            fighter.facing = fighter.x <= stage.cameraStartx ? 1 : -1;
            fighter.onGround = true;
            fighter.life = 1000;
            fighter.power = 0;
            enterRoundInitialState(state, fighter);
        }

        state.messages.lastHitText.clear();
        state.messages.lastHitTextTicks = 0;
        clearComboCounters(state);
        state.runtimeEffects.clear();
        clearGlobalPause(state);
        clearEnvShake(state);
        clearPaletteRuntime(state);
        state.audio.activeVoices.clear();
        if (state.audio.stream) {
            SDL_ClearAudioStream(state.audio.stream);
        }
        state.training.options.menuOpen = false;
        state.training.options.moveListOpen = false;
        state.training.commandDemo = {};
        state.frontend.singleFightPauseOpen = false;
        state.frontend.selectedSingleFightPauseOption = 0;
        state.frontend.selectedMatchResultOption = 0;
        state.matchPhase = MatchPhase::RoundStart;
        state.roundEndReason = RoundEndReason::None;
        state.matchTimerTicks = matchTimerTicksFromSettings(state.mainSettings);
        state.matchPhaseTicks = 0;
        state.roundWinner = 0;
        state.roundScoreApplied = false;
        state.roundPoseApplied = false;
        state.matchComplete = false;
        state.cameraX = stage.cameraStartx;
        state.cameraY = stage.cameraStarty;
        return;
    }

    state.fighters.resize(2);
    state.fighters[0] = {};
    state.fighters[1] = {};
    state.helpers.clear();
    state.projectiles.clear();
    state.fighters[0].x = stage.p1startx;
    state.fighters[0].y = stage.p1starty;
    state.fighters[1].x = stage.p2startx;
    state.fighters[1].y = stage.p2starty;
    state.fighters[0].facing = state.fighters[0].x <= state.fighters[1].x ? 1 : -1;
    state.fighters[1].facing = -state.fighters[0].facing;
    state.fighters[0].onGround = true;
    state.fighters[1].onGround = true;
    state.fighters[0].life = 1000;
    state.fighters[1].life = 1000;
    state.fighters[0].hitPauseTicks = 0;
    state.fighters[1].hitPauseTicks = 0;
    state.fighters[0].hitStunTicks = 0;
    state.fighters[1].hitStunTicks = 0;
    state.fighters[0].hitSlideTicks = 0;
    state.fighters[1].hitSlideTicks = 0;
    state.fighters[0].hitVelocityX = 0.0f;
    state.fighters[1].hitVelocityX = 0.0f;
    state.fighters[0].hitVelocityY = 0.0f;
    state.fighters[1].hitVelocityY = 0.0f;
    state.fighters[0].getHitAnimType = 0;
    state.fighters[1].getHitAnimType = 0;
    state.fighters[0].getHitGroundType = 0;
    state.fighters[1].getHitGroundType = 0;
    state.fighters[0].getHitAirType = 0;
    state.fighters[1].getHitAirType = 0;
    state.fighters[0].getHitSlideTime = 0;
    state.fighters[1].getHitSlideTime = 0;
    state.fighters[0].getHitHitTime = 0;
    state.fighters[1].getHitHitTime = 0;
    state.fighters[0].getHitCtrlTime = 0;
    state.fighters[1].getHitCtrlTime = 0;
    state.fighters[0].getHitHitCount = 0;
    state.fighters[1].getHitHitCount = 0;
    state.fighters[0].hitRecoverAnim = 5005;
    state.fighters[1].hitRecoverAnim = 5005;
    state.fighters[0].appliedHitDefIds.clear();
    state.fighters[1].appliedHitDefIds.clear();
    state.fighters[0].firedStateSoundControllerIds.clear();
    state.fighters[1].firedStateSoundControllerIds.clear();
    state.fighters[0].firedStateCtrlControllerIds.clear();
    state.fighters[1].firedStateCtrlControllerIds.clear();
    state.fighters[0].firedStatePosAddControllerIds.clear();
    state.fighters[1].firedStatePosAddControllerIds.clear();
    state.fighters[0].firedStateChangeAnimControllerIds.clear();
    state.fighters[1].firedStateChangeAnimControllerIds.clear();
    state.fighters[0].firedStateRuntimeControllerIds.clear();
    state.fighters[1].firedStateRuntimeControllerIds.clear();
    state.fighters[0].moveContact = false;
    state.fighters[1].moveContact = false;
    state.fighters[0].moveHit = false;
    state.fighters[1].moveHit = false;
    state.fighters[0].moveGuarded = false;
    state.fighters[1].moveGuarded = false;
    state.messages.lastHitText.clear();
    state.messages.lastHitTextTicks = 0;
    clearComboCounters(state);
    state.runtimeEffects.clear();
    clearGlobalPause(state);
    clearEnvShake(state);
    clearPaletteRuntime(state);
    state.audio.activeVoices.clear();
    if (state.audio.stream) {
        SDL_ClearAudioStream(state.audio.stream);
    }
    state.training.options.menuOpen = false;
    state.training.options.moveListOpen = false;
    state.training.commandDemo = {};
    state.frontend.singleFightPauseOpen = false;
    state.frontend.selectedSingleFightPauseOption = 0;
    state.frontend.selectedMatchResultOption = 0;
    state.matchPhase = isMatchMode(state) ? MatchPhase::RoundStart : MatchPhase::Fight;
    state.roundEndReason = RoundEndReason::None;
    state.matchTimerTicks = matchTimerTicksFromSettings(state.mainSettings);
    state.matchPhaseTicks = 0;
    state.roundWinner = 0;
    state.roundScoreApplied = false;
    state.roundPoseApplied = false;
    state.cameraX = stage.cameraStartx;
    state.cameraY = stage.cameraStarty;
    enterRoundInitialState(state, state.fighters[0]);
    enterRoundInitialState(state, state.fighters[1]);
}

void resetFightState(AppState& state) {
    state.roundWins = { 0, 0 };
    state.currentRound = 1;
    state.matchComplete = false;
    state.roundPoseApplied = false;
    resetFightRound(state);
}

bool prepareFightSession(SDL_Renderer* renderer, AppState& state) {
    state.fightSessionPrepared = false;
    state.fightSessionLoadFailed = false;

    if (const StageSlot* stage = selectedStageSlot(state.selection)) {
        state.content.stageName = stage->displayName;
    }

    if (!loadSelectedCharacterRuntime(renderer, state)) {
        state.fightSessionLoadFailed = true;
        return false;
    }

    std::vector<StageBackgroundElement> selectedBackground;
    if (const StageSlot* stage = selectedStageSlot(state.selection)) {
        try {
            selectedBackground = loadStageBackground(renderer, *stage);
        } catch (const std::exception& ex) {
            SDL_Log("Selected stage background load failed %s: %s", stage->displayName.c_str(), ex.what());
        }
    }
    destroyStageBackground(state.stageBackground);
    state.stageBackground = std::move(selectedBackground);
    state.stageBackgroundStageIndex = state.selection.selectedStage;

    resetFightState(state);
    state.frontend.screenFrame = 0;
    state.fightSessionPrepared = true;
    return true;
}

void beginFight(AppState& state) {
    if (!state.fightSessionPrepared) {
        return;
    }
    state.frontend.screen = Screen::FightView;
    state.frontend.screenFrame = 0;
}

void prepareVersusSessionAfterPresent(SDL_Renderer* renderer, AppState& state) {
    if (state.frontend.screen != Screen::VersusScreen
        || state.fightSessionPrepared
        || state.fightSessionLoadFailed
        || state.frontend.screenFrame < kVersusPrepareStartFrames) {
        return;
    }

    prepareFightSession(renderer, state);
}
