#pragma once

// Internal App.cpp implementation shard.
// Round result, progression, controls save, and mode-runtime include wiring.

std::string fighterResultName(const AppState& state, int winner) {
    if (state.frontend.pendingMode == PendingMode::Arena) {
        if (winner > 0 && winner <= static_cast<int>(state.fighters.size())) {
            return arenaFighterName(state, static_cast<size_t>(winner - 1));
        }
        return "";
    }
    if (state.frontend.pendingMode == PendingMode::Story) {
        return winner == 1 ? selectedCharacterName(state.selection) : "Enemy Waves";
    }

    switch (winner) {
    case 1:
        return selectedCharacterName(state.selection);
    case 2:
        return opponentDisplayName(state);
    default:
        return "";
    }
}

int matchWinsRequired(const AppState& state) {
    return std::max(1, state.fightRoundSettings.matchWins);
}

bool isSingleFightResultPhase(const AppState& state) {
    return state.matchPhase == MatchPhase::RoundFinish
        || state.matchPhase == MatchPhase::RoundResult
        || state.matchPhase == MatchPhase::MatchResult;
}

std::string roundResultText(const AppState& state) {
    if (state.frontend.pendingMode == PendingMode::Arena) {
        if (state.roundWinner > 0 && state.roundWinner <= static_cast<int>(state.fighters.size())) {
            return state.arenaConfig.winTitle + ": " + uppercaseCopy(fighterResultName(state, state.roundWinner));
        }
        return "DRAW GAME";
    }
    if (state.frontend.pendingMode == PendingMode::Story) {
        return state.roundWinner == 1 ? "STAGE CLEAR" : "MISSION FAILED";
    }

    if (state.roundWinner == 1 || state.roundWinner == 2) {
        return uppercaseCopy(fighterResultName(state, state.roundWinner)) + " WINS";
    }
    return "DRAW GAME";
}

std::string roundFinishCalloutText(const AppState& state) {
    if (state.frontend.pendingMode == PendingMode::Story) {
        return state.roundWinner == 1 ? "STAGE CLEAR" : "MISSION FAILED";
    }
    switch (state.roundEndReason) {
    case RoundEndReason::TimeUp:
        return "TIME OVER";
    case RoundEndReason::DoubleKo:
        return "DOUBLE K.O.";
    case RoundEndReason::Ko:
        return "K.O.";
    default:
        return "ROUND OVER";
    }
}

std::string singleFightScoreText(const AppState& state) {
    if (state.frontend.pendingMode == PendingMode::Arena) {
        return "Free-for-all";
    }
    if (state.frontend.pendingMode == PendingMode::Story) {
        return "Wave " + std::to_string(std::clamp(state.story.waveIndex + 1, 1, kStoryWaveCount))
            + "/" + std::to_string(kStoryWaveCount);
    }
    return std::to_string(state.roundWins[0]) + " - " + std::to_string(state.roundWins[1]);
}

int singleFightRoundStartTotalTicks(const AppState& state) {
    const auto& round = state.fightRoundSettings;
    return std::max(1, round.startWaitTime + round.roundDisplayTime + round.ctrlTime);
}

int singleFightRoundDisplayEndTick(const AppState& state) {
    const auto& round = state.fightRoundSettings;
    return round.startWaitTime + round.roundDisplayTime;
}

int singleFightRoundFinishCalloutTicks(const AppState& state) {
    const auto& round = state.fightRoundSettings;
    switch (state.roundEndReason) {
    case RoundEndReason::DoubleKo:
        return std::max(1, round.dkoDisplayTime);
    case RoundEndReason::TimeUp:
        return std::max(1, round.timeOverDisplayTime);
    case RoundEndReason::Ko:
    default:
        return std::max(1, round.koDisplayTime);
    }
}

int singleFightRoundFinishHoldTicks(const AppState& state) {
    const auto& round = state.fightRoundSettings;
    return std::max({ 1, round.overTime, round.overHitTime, round.overWinTime, singleFightRoundFinishCalloutTicks(state) });
}

int singleFightRoundResultHoldTicks(const AppState& state) {
    return std::max(75, state.fightRoundSettings.winTime + 30);
}

bool canEnterState(const AppState& state, int stateNo) {
    const StateDefinition* stateDef = findStateDefinition(state, stateNo);
    return stateDef && (!stateDef->hasAnim || findExactClip(state, stateDef->anim));
}

bool canEnterStateForActor(const AppState& state, const FighterState& fighter, int stateNo) {
    const StateDefinition* stateDef = findStateDefinitionForActor(state, fighter, stateNo);
    return stateDef && (!stateDef->hasAnim || findExactClipForActor(state, fighter, stateDef->anim));
}

bool enterStateIfAvailable(const AppState& state, FighterState& fighter, int stateNo) {
    if (!canEnterStateForActor(state, fighter, stateNo)) {
        return false;
    }
    enterState(state, fighter, stateNo);
    return true;
}

void applySingleFightRoundPoses(AppState& state) {
    if (state.roundPoseApplied
        || state.matchPhaseTicks < state.fightRoundSettings.overWinTime
        || state.matchPhaseTicks < state.fightRoundSettings.overHitTime) {
        return;
    }

    auto& p1 = state.fighters[0];
    auto& p2 = state.fighters[1];
    p1.ctrl = false;
    p2.ctrl = false;
    p1.vx = 0.0f;
    p2.vx = 0.0f;

    if (state.roundEndReason == RoundEndReason::TimeUp) {
        if (state.roundWinner == 1) {
            enterStateIfAvailable(state, p1, 181);
            enterStateIfAvailable(state, p2, 170);
        } else if (state.roundWinner == 2) {
            enterStateIfAvailable(state, p2, 181);
            enterStateIfAvailable(state, p1, 170);
        } else {
            if (!enterStateIfAvailable(state, p1, 175)) {
                enterStateIfAvailable(state, p1, 170);
            }
            if (!enterStateIfAvailable(state, p2, 175)) {
                enterStateIfAvailable(state, p2, 170);
            }
        }
    } else if (state.roundEndReason == RoundEndReason::Ko) {
        if (state.roundWinner == 1) {
            enterStateIfAvailable(state, p1, 181);
            p2.ctrl = false;
        } else if (state.roundWinner == 2) {
            enterStateIfAvailable(state, p2, 181);
            p1.ctrl = false;
        }
    }

    state.roundPoseApplied = true;
}

std::string roundStartCalloutText(const AppState& state) {
    if (state.frontend.pendingMode == PendingMode::Arena) {
        return "ARENA";
    }
    if (state.frontend.pendingMode == PendingMode::Story) {
        return "STORY";
    }
    if (state.roundWins[0] == matchWinsRequired(state) - 1
        && state.roundWins[1] == matchWinsRequired(state) - 1) {
        return "FINAL ROUND";
    }
    return "ROUND " + std::to_string(state.currentRound);
}

int matchWinner(const AppState& state) {
    if (state.frontend.pendingMode == PendingMode::Arena) {
        return state.roundWinner;
    }
    if (state.frontend.pendingMode == PendingMode::Story) {
        return state.roundWinner;
    }

    const int required = matchWinsRequired(state);
    if (state.roundWins[0] >= required) {
        return 1;
    }
    if (state.roundWins[1] >= required) {
        return 2;
    }
    return 0;
}

std::string matchWinMethodText(const AppState& state) {
    if (state.frontend.pendingMode == PendingMode::Arena) {
        return state.roundWinner > 0 ? "Last Fighter Standing" : "No winner recorded";
    }
    if (state.frontend.pendingMode == PendingMode::Story) {
        return state.roundWinner == 1 ? "OpenBOR Stage Clear" : "Player Defeated";
    }

    if (matchWinner(state) == 0) {
        return "Draw Game";
    }
    switch (state.roundEndReason) {
    case RoundEndReason::Ko:
        return "Won by K.O.";
    case RoundEndReason::TimeUp:
        return "Won by Decision";
    default:
        return "Won by Decision";
    }
}

void loadProgressionState(AppState& state) {
    state.progression.savePath = dragonProgressionSavePath(state.gameRoot);
    try {
        state.progression.data = loadDragonProgressionData(state.gameRoot);
        state.progression.save = loadDragonProgressionSave(state.progression.savePath);
        state.progression.loaded = true;
    } catch (const std::exception& ex) {
        SDL_Log("Dragon progression load failed: %s", ex.what());
        state.progression = {};
        state.progression.loaded = true;
    }
}

std::array<std::string, kControlPlayerCount> controlProfileIdsForPlayers(const AppState& state) {
    std::array<std::string, kControlPlayerCount> ids{
        "player1",
        "guest",
        "player3",
        "player4",
    };
    if (state.progression.loaded) {
        ids[0] = dragonProgressionPlayerProfileId(state.progression.save, 0);
        ids[1] = dragonProgressionPlayerProfileId(state.progression.save, 1);
    }
    if (ids[0].empty()) {
        ids[0] = "player1";
    }
    if (ids[1].empty()) {
        ids[1] = dragonProgressionGuestProfileId();
    }
    return ids;
}

std::array<std::string, kControlPlayerCount> controlProfileNamesForPlayers(const AppState& state) {
    std::array<std::string, kControlPlayerCount> names{
        "Player 1",
        "Guest",
        "Player 3",
        "Player 4",
    };
    if (state.progression.loaded) {
        names[0] = dragonProgressionPlayerProfileDisplayName(state.progression.save, 0);
        names[1] = dragonProgressionPlayerProfileDisplayName(state.progression.save, 1);
    }
    return names;
}

void syncControlsWithProfiles(AppState& state) {
    syncDefaultControlProfilesForPlayers(state.controls, controlProfileIdsForPlayers(state));
    state.mainSettings.p1GamepadAssignment = gamepadAssignmentForPlayer(state, 0);
    state.mainSettings.p2GamepadAssignment = gamepadAssignmentForPlayer(state, 1);
}

void loadControlsState(AppState& state) {
    state.controlsSavePath = dragonControlsSavePath(state.gameRoot);
    try {
        state.controls = loadControlsSettings(state.controlsSavePath);
        syncControlsWithProfiles(state);
    } catch (const std::exception& ex) {
        SDL_Log("Dragon controls load failed: %s", ex.what());
        state.controls = {};
        syncControlsWithProfiles(state);
    }
}

void saveControlsStateQuietly(AppState& state) {
    try {
        saveControlsSettings(state.controlsSavePath.empty()
                ? dragonControlsSavePath(state.gameRoot)
                : state.controlsSavePath,
            state.controls);
    } catch (const std::exception& ex) {
        SDL_Log("Dragon controls save failed: %s", ex.what());
    }
}

ControlProfileBinding& controlProfileForPlayer(AppState& state, int playerIndex) {
    const auto ids = controlProfileIdsForPlayers(state);
    return ensureControlProfile(
        state.controls,
        ids[static_cast<size_t>(std::clamp(playerIndex, 0, kControlPlayerCount - 1))],
        playerIndex);
}

const ControlProfileBinding& controlProfileForPlayerConst(const AppState& state, int playerIndex) {
    const auto ids = controlProfileIdsForPlayers(state);
    const int safePlayer = std::clamp(playerIndex, 0, kControlPlayerCount - 1);
    if (const auto* profile = findControlProfile(state.controls, ids[static_cast<size_t>(safePlayer)])) {
        return *profile;
    }
    static const ControlProfileBinding fallback = makeDefaultControlProfile("player1", 0);
    return fallback;
}

bool keyMatchesControlAction(const AppState& state, SDL_Keycode key, int playerIndex, InputAction action) {
    const SDL_Scancode scancode = SDL_GetScancodeFromKey(key, nullptr);
    if (scancode == SDL_SCANCODE_UNKNOWN) {
        return false;
    }
    const auto& profile = controlProfileForPlayerConst(state, playerIndex);
    const auto* actionBinding = findActionBinding(profile, action);
    if (!actionBinding) {
        return false;
    }
    const PhysicalInputBinding keyInput = keyBinding(scancode);
    return std::any_of(actionBinding->bindings.begin(), actionBinding->bindings.end(), [&](const auto& binding) {
        return samePhysicalInput(binding, keyInput);
    });
}

bool gamepadButtonMatchesControlAction(
    const AppState& state,
    int playerIndex,
    SDL_GamepadButton button,
    InputAction action) {
    if (playerIndex < 0) {
        playerIndex = 0;
    }
    const auto& profile = controlProfileForPlayerConst(state, playerIndex);
    const auto* actionBinding = findActionBinding(profile, action);
    if (!actionBinding) {
        return false;
    }
    const PhysicalInputBinding padInput = gamepadButtonBinding(button);
    return std::any_of(actionBinding->bindings.begin(), actionBinding->bindings.end(), [&](const auto& binding) {
        return samePhysicalInput(binding, padInput);
    });
}

void clearProgressionMatchAward(AppState& state) {
    state.progression.matchAwardApplied = false;
    state.progression.lastAwardText.clear();
}

std::string compactProgressionAwardText(
    std::string_view playerLabel,
    const DragonProgressionAwardResult& award,
    int goldBalance) {
    if (!award.applied) {
        return {};
    }
    std::ostringstream out;
    out << playerLabel << " +" << award.xpGained << " XP";
    if (award.goldGained > 0) {
        out << " +" << award.goldGained << "G";
    }
    out << " LV " << award.newLevel << " BAL " << std::max(0, goldBalance) << "G";
    return out.str();
}

void awardProgressionForMatchIfNeeded(AppState& state) {
    if (!isMatchMode(state)
        || state.matchPhase != MatchPhase::MatchResult
        || state.progression.matchAwardApplied) {
        return;
    }
    state.progression.matchAwardApplied = true;
    if (!state.progression.loaded) {
        loadProgressionState(state);
    }
    if (!state.progression.data.config.enabled) {
        return;
    }

    const CharacterSlot* p1 = sessionP1CharacterSlot(state.selection);
    if (!p1) {
        return;
    }
    const int winner = matchWinner(state);
    const bool p1Won = winner == 1;
    const bool arenaMode = state.frontend.pendingMode == PendingMode::Arena
        || state.frontend.pendingMode == PendingMode::Story;
    const bool localVsMode = state.frontend.pendingMode == PendingMode::SingleFight
        && activeOpponentType(state) == OpponentType::LocalP2;
    const std::string p1ProfileId = dragonProgressionPlayerProfileId(state.progression.save, 0);
    const auto p1Award = recordDragonProgressionMatchForProfile(
        state.progression.data,
        state.progression.save,
        p1ProfileId,
        p1->id,
        p1->displayName,
        p1Won,
        arenaMode);

    bool anyAwardApplied = p1Award.applied;
    if (localVsMode) {
        std::vector<std::string> awardLines;
        const int p1GoldBalance = dragonProgressionGoldForProfile(state.progression.save, p1ProfileId);
        if (const auto text = compactProgressionAwardText("P1", p1Award, p1GoldBalance); !text.empty()) {
            awardLines.push_back(text);
        }

        const CharacterSlot* p2 = characterSlotAt(state.selection, state.selection.sessionSlots.opponentCharacter);
        const std::string p2ProfileId = dragonProgressionPlayerProfileId(state.progression.save, 1);
        if (p2 && !isDragonProgressionGuestProfile(p2ProfileId)) {
            const bool p2Won = winner == 2;
            const auto p2Award = recordDragonProgressionMatchForProfile(
                state.progression.data,
                state.progression.save,
                p2ProfileId,
                p2->id,
                p2->displayName,
                p2Won,
                false);
            anyAwardApplied = anyAwardApplied || p2Award.applied;
            const int p2GoldBalance = dragonProgressionGoldForProfile(state.progression.save, p2ProfileId);
            if (const auto text = compactProgressionAwardText("P2", p2Award, p2GoldBalance); !text.empty()) {
                awardLines.push_back(text);
            }
        } else if (const auto text = compactProgressionAwardText("P1", p1Award, p1GoldBalance); !text.empty()) {
            awardLines.push_back("P2 GUEST NO SAVE");
        }

        if (!awardLines.empty()) {
            std::ostringstream out;
            for (size_t i = 0; i < awardLines.size(); ++i) {
                if (i > 0) {
                    out << " | ";
                }
                out << awardLines[i];
            }
            state.progression.lastAwardText = out.str();
        } else {
            state.progression.lastAwardText.clear();
        }
    } else {
        const int p1GoldBalance = dragonProgressionGoldForProfile(state.progression.save, p1ProfileId);
        state.progression.lastAwardText = dragonProgressionAwardSummaryWithGoldBalance(p1Award, p1GoldBalance);
    }

    if (anyAwardApplied) {
        try {
            saveDragonProgressionSave(state.progression.savePath, state.progression.save);
        } catch (const std::exception& ex) {
            SDL_Log("Dragon progression save failed: %s", ex.what());
        }
    }
}

void finalizeSingleFightRoundAfterGrace(AppState& state) {
    if (state.roundEndReason != RoundEndReason::Ko || state.matchPhaseTicks < state.fightRoundSettings.overHitTime) {
        return;
    }
    if (state.fighters[0].life <= 0 && state.fighters[1].life <= 0) {
        state.roundWinner = 0;
        state.roundEndReason = RoundEndReason::DoubleKo;
        state.messages.lastHitText = roundFinishCalloutText(state);
        state.roundPoseApplied = false;
    }
}

void applySingleFightRoundScore(AppState& state) {
    if (state.roundScoreApplied) {
        return;
    }

    if (state.roundWinner == 1 || state.roundWinner == 2) {
        const size_t winIndex = static_cast<size_t>(state.roundWinner - 1);
        state.roundWins[winIndex] = std::min(matchWinsRequired(state), state.roundWins[winIndex] + 1);
        state.matchComplete = state.roundWins[winIndex] >= matchWinsRequired(state);
    } else {
        state.matchComplete = false;
    }
    state.roundScoreApplied = true;
    state.messages.lastHitText = roundResultText(state);
    state.messages.lastHitTextTicks = singleFightRoundResultHoldTicks(state);
}

void startSingleFightRoundFinish(AppState& state, int winner, RoundEndReason reason) {
    state.matchPhase = MatchPhase::RoundFinish;
    state.matchPhaseTicks = 0;
    state.roundWinner = winner;
    state.roundEndReason = reason;
    state.roundScoreApplied = false;
    state.roundPoseApplied = false;
    state.matchComplete = false;
    state.messages.lastHitText = roundFinishCalloutText(state);
    state.messages.lastHitTextTicks = singleFightRoundFinishHoldTicks(state);
}

void updateSingleFightRules(AppState& state) {
    if (!isMatchMode(state) || state.matchPhase != MatchPhase::Fight) {
        return;
    }

    const bool timerEnabled = state.mainSettings.matchTimerSeconds > 0;
    if (timerEnabled && state.matchTimerTicks > 0 && !anyFighterHasAssertSpecialFlag(state, "timerfreeze")) {
        --state.matchTimerTicks;
    }

    if (anyFighterHasAssertSpecialFlag(state, "roundnotover")
        || anyFighterHasAssertSpecialFlag(state, "intro")) {
        return;
    }

    const int p1Life = state.fighters[0].life;
    const int p2Life = state.fighters[1].life;
    if (p1Life <= 0 && p2Life <= 0) {
        startSingleFightRoundFinish(state, 0, RoundEndReason::DoubleKo);
    } else if (p1Life <= 0) {
        startSingleFightRoundFinish(state, 2, RoundEndReason::Ko);
    } else if (p2Life <= 0) {
        startSingleFightRoundFinish(state, 1, RoundEndReason::Ko);
    } else if (timerEnabled && state.matchTimerTicks <= 0) {
        if (p1Life > p2Life) {
            startSingleFightRoundFinish(state, 1, RoundEndReason::TimeUp);
        } else if (p2Life > p1Life) {
            startSingleFightRoundFinish(state, 2, RoundEndReason::TimeUp);
        } else {
            startSingleFightRoundFinish(state, 0, RoundEndReason::TimeUp);
        }
    }
}

void updateRoundStartWorld(AppState& state, const StageSlot& stage) {
    auto& p1 = state.fighters[0];
    auto& p2 = state.fighters[1];

    resetFighterOneTickBounds(state);
    updateStateMovementControllers(state, p1, &p2, &stage);
    updateStateHelperControllers(state, p1, &p2, &stage);
    updateStateProjectileControllers(state, p1, &p2, &stage);
    updateStateMakeDustControllers(state, p1, &p2, stage);
    updateStateExplodControllers(state, p1, &p2, stage);

    updateStateMovementControllers(state, p2, &p1, &stage);
    updateStateHelperControllers(state, p2, &p1, &stage);
    updateStateProjectileControllers(state, p2, &p1, &stage);
    updateStateMakeDustControllers(state, p2, &p1, stage);
    updateStateExplodControllers(state, p2, &p1, stage);

    updateHelperActors(state, stage);
    const bool p1ChangedState = updateStateChangeStateControllers(state, p1, &p2, &stage);
    const bool p2ChangedState = updateStateChangeStateControllers(state, p2, &p1, &stage);
    updateStateTargetControllers(state, p1, &p2, &stage);
    updateStateTargetControllers(state, p2, &p1, &stage);
    updateStateChangeAnimControllers(state, p1, &p2, &stage);
    updateStateChangeAnimControllers(state, p2, &p1, &stage);
    updateStatePosAddControllers(state, p1, &p2, &stage);
    updateStatePosAddControllers(state, p2, &p1, &stage);
    applyTargetBindings(state);
    updateStateCtrlControllers(state, p1);
    updateStateCtrlControllers(state, p2);
    updateStateAudioControllers(state, p1, &p2, &stage);
    updateStateAudioControllers(state, p2, &p1, &stage);
    updateFightAssertSpecialControllers(state, stage);

    if (!p1ChangedState && p1.hitPauseTicks <= 0) {
        ++p1.animTick;
        ++p1.stateTime;
        updateAfterImageEffect(p1);
    }
    if (!p2ChangedState && p2.hitPauseTicks <= 0) {
        ++p2.animTick;
        ++p2.stateTime;
        updateAfterImageEffect(p2);
    }
    updateGlobalPauseTimers(state);
    finishStateIfAnimationEnded(state, p1);
    finishStateIfAnimationEnded(state, p2);
}

void updateSingleFightRoundFinishWorld(AppState& state, const StageSlot& stage) {
    auto& p1 = state.fighters[0];
    auto& p2 = state.fighters[1];

    resetFighterOneTickBounds(state);
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
    if (fighterCanUpdateDuringGlobalPause(state, 0)) {
        updateFighterPhysics(state, p1, stage);
    }
    if (fighterCanUpdateDuringGlobalPause(state, 1)) {
        updateFighterPhysics(state, p2, stage);
    }
    updateCommonAirRecoveryState(state, p1);
    updateCommonAirRecoveryState(state, p2);
    updateCommonDizzyState(state, p1);
    updateCommonDizzyState(state, p2);
    const bool p1ChangedAfterPhysics = p1CanUpdate && !p1ChangedBeforePhysics && updateStateChangeStateControllers(state, p1, &p2, &stage);
    const bool p2ChangedAfterPhysics = p2CanUpdate && !p2ChangedBeforePhysics && updateStateChangeStateControllers(state, p2, &p1, &stage);
    if (p1ChangedAfterPhysics && p1.y >= 0.0f && p1.stateType != 'A') {
        p1.y = 0.0f;
        p1.vy = 0.0f;
        p1.onGround = true;
    }
    if (p2ChangedAfterPhysics && p2.y >= 0.0f && p2.stateType != 'A') {
        p2.y = 0.0f;
        p2.vy = 0.0f;
        p2.onGround = true;
    }
    if (p1CanUpdate
        && !p1ChangedAfterPhysics
        && shouldDeferCommonLandingToAuthoredAirChangeState(state, p1)
        && p1.y >= 0.0f
        && p1.vy >= 0.0f) {
        enterCommonLandingState(state, p1);
    }
    if (p2CanUpdate
        && !p2ChangedAfterPhysics
        && shouldDeferCommonLandingToAuthoredAirChangeState(state, p2)
        && p2.y >= 0.0f
        && p2.vy >= 0.0f) {
        enterCommonLandingState(state, p2);
    }
    if (p1CanUpdate) {
        updateNeutralAirLandingFallback(state, p1);
    }
    if (p2CanUpdate) {
        updateNeutralAirLandingFallback(state, p2);
    }
    applyPlayerPush(state, stage);
    updateFighterFacing(state);
    updateComboCounterBreaks(state);

    if (state.matchPhaseTicks <= state.fightRoundSettings.overHitTime) {
        updateRuntimeProjectiles(state, stage);
        applyHitIfNeeded(state);
    }

    if (p1CanUpdate) {
        updateStateChangeAnimControllers(state, p1, &p2, &stage);
        updateStatePosAddControllers(state, p1, &p2, &stage);
        updateStateCtrlControllers(state, p1);
        updateStateAudioControllers(state, p1, &p2, &stage);
    }
    if (p2CanUpdate) {
        updateStateChangeAnimControllers(state, p2, &p1, &stage);
        updateStatePosAddControllers(state, p2, &p1, &stage);
        updateStateCtrlControllers(state, p2);
        updateStateAudioControllers(state, p2, &p1, &stage);
    }
    applyTargetBindings(state);
    updateFightAssertSpecialControllers(state, stage);
    if (state.matchPhaseTicks >= state.fightRoundSettings.overWaitTime) {
        p1.ctrl = false;
        p2.ctrl = false;
    }
    updateCamera(state, stage);
    applyScreenBounds(state, stage);

    if (p1.hitPauseTicks <= 0 && fighterCanUpdateDuringGlobalPause(state, 0)) {
        ++p1.animTick;
        ++p1.stateTime;
        updateAfterImageEffect(p1);
    }
    if (p2.hitPauseTicks <= 0 && fighterCanUpdateDuringGlobalPause(state, 1)) {
        ++p2.animTick;
        ++p2.stateTime;
        updateAfterImageEffect(p2);
    }
    updateGlobalPauseTimers(state);
    finishStateIfAnimationEnded(state, p1);
    finishStateIfAnimationEnded(state, p2);
}

void updateSingleFightPhaseTimers(AppState& state) {
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
        finalizeSingleFightRoundAfterGrace(state);
        applySingleFightRoundPoses(state);
        if (state.matchPhaseTicks >= singleFightRoundFinishHoldTicks(state)) {
            applySingleFightRoundScore(state);
            state.matchPhase = MatchPhase::RoundResult;
            state.matchPhaseTicks = 0;
        }
        break;
    case MatchPhase::RoundResult:
        ++state.matchPhaseTicks;
        if (state.matchPhaseTicks >= singleFightRoundResultHoldTicks(state)) {
            if (state.matchComplete) {
                state.matchPhase = MatchPhase::MatchResult;
                state.matchPhaseTicks = 0;
                state.frontend.selectedMatchResultOption = 0;
                awardProgressionForMatchIfNeeded(state);
            } else {
                ++state.currentRound;
                resetFightRound(state);
            }
        }
        break;
    case MatchPhase::MatchResult:
        awardProgressionForMatchIfNeeded(state);
        ++state.matchPhaseTicks;
        break;
    case MatchPhase::Fight:
    default:
        break;
    }
}

#include "ArenaModeRuntime.h"
#include "StoryModeRuntime.h"
