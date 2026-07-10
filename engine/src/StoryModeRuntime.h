#pragma once

// Internal App.cpp implementation header for Story Mode fight loop.
// Include only from App.cpp after ArenaModeRuntime.h and StoryModeSession.h helpers are available.

bool storyActorCanUpdate(const AppState& state, size_t fighterIndex) {
    if (fighterIndex >= state.fighters.size()) {
        return false;
    }
    if (fighterIndex == 0) {
        return state.fighters[0].life > 0;
    }
    return storyEnemySlotActive(state, fighterIndex) && state.fighters[fighterIndex].life > 0;
}
FighterState* storyTargetForFighter(AppState& state, size_t fighterIndex) {
    const int targetIndex = fighterIndex == 0
        ? storyNearestLivingEnemyIndex(state, 0)
        : storyNearestLivingEnemyIndex(state, static_cast<int>(fighterIndex));
    if (targetIndex < 0 || targetIndex >= static_cast<int>(state.fighters.size())) {
        return nullptr;
    }
    return &state.fighters[static_cast<size_t>(targetIndex)];
}

const FighterState* storyTargetForFighter(const AppState& state, size_t fighterIndex) {
    const int targetIndex = fighterIndex == 0
        ? storyNearestLivingEnemyIndex(state, 0)
        : storyNearestLivingEnemyIndex(state, static_cast<int>(fighterIndex));
    if (targetIndex < 0 || targetIndex >= static_cast<int>(state.fighters.size())) {
        return nullptr;
    }
    return &state.fighters[static_cast<size_t>(targetIndex)];
}

void updateStoryFighterFacing(AppState& state) {
    for (size_t i = 0; i < state.fighters.size(); ++i) {
        if (!storyActorCanUpdate(state, i)) {
            continue;
        }
        if (const FighterState* target = storyTargetForFighter(state, i)) {
            state.fighters[i].facing = state.fighters[i].x <= target->x ? 1 : -1;
        }
    }
}

int storyEnemyRewardPermille(const AppState& state) {
    switch (state.story.difficulty) {
    case StoryDifficulty::Easy:
        return 850;
    case StoryDifficulty::Hard:
        return 1250;
    case StoryDifficulty::Medium:
    default:
        return 1000;
    }
}

std::string storyEnemyRewardText(const DragonProgressionAwardResult& award, int goldBalance) {
    if (!award.applied) {
        return {};
    }
    std::ostringstream out;
    out << "ENEMY +" << award.xpGained << " XP";
    if (award.goldGained > 0) {
        out << " +" << award.goldGained << "G";
    }
    out << " LV " << award.newLevel;
    out << " BAL " << std::max(0, goldBalance) << "G";
    return out.str();
}

void awardStoryEnemyDefeat(AppState& state, const FighterState& defeatedEnemy) {
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
    const std::string profileId = dragonProgressionPlayerProfileId(state.progression.save, 0);
    if (isDragonProgressionGuestProfile(profileId)) {
        return;
    }
    const int permille = storyEnemyRewardPermille(state);
    const int xp = std::max(0, (state.progression.data.config.enemyDefeatXp * permille + 500) / 1000);
    const int gold = std::max(0, (state.progression.data.config.enemyDefeatGold * permille + 500) / 1000);
    const auto award = recordDragonProgressionRewardForProfile(
        state.progression.data,
        state.progression.save,
        profileId,
        p1->id,
        p1->displayName,
        xp,
        gold);
    if (!award.applied) {
        return;
    }
    const int goldBalance = dragonProgressionGoldForProfile(state.progression.save, profileId);
    state.progression.lastAwardText = storyEnemyRewardText(award, goldBalance);
    state.messages.lastHitText = state.progression.lastAwardText;
    state.messages.lastHitTextTicks = std::max(state.messages.lastHitTextTicks, 60);
    spawnStoryRewardFeedback(state, defeatedEnemy, award);
    try {
        saveDragonProgressionSave(
            state.progression.savePath.empty() ? dragonProgressionSavePath(state.gameRoot) : state.progression.savePath,
            state.progression.save);
    } catch (const std::exception& ex) {
        SDL_Log("Dragon story enemy reward save failed: %s", ex.what());
    }
}

std::string storyConfiguredRewardText(
    std::string_view label,
    const DragonProgressionAwardResult& award,
    int goldBalance) {
    if (!award.applied) {
        return {};
    }
    std::ostringstream out;
    out << label << " +" << award.xpGained << " XP";
    if (award.goldGained > 0) {
        out << " +" << award.goldGained << "G";
    }
    out << " BAL " << std::max(0, goldBalance) << "G";
    return out.str();
}

void awardStoryConfiguredReward(AppState& state, int xp, int gold, std::string_view label) {
    xp = std::max(0, xp);
    gold = std::max(0, gold);
    if (xp <= 0 && gold <= 0) {
        return;
    }
    if (!state.progression.loaded) {
        loadProgressionState(state);
    }
    if (!state.progression.data.config.enabled || state.fighters.empty()) {
        return;
    }
    const CharacterSlot* p1 = sessionP1CharacterSlot(state.selection);
    if (!p1) {
        return;
    }
    const std::string profileId = dragonProgressionPlayerProfileId(state.progression.save, 0);
    if (isDragonProgressionGuestProfile(profileId)) {
        return;
    }
    const auto award = recordDragonProgressionRewardForProfile(
        state.progression.data,
        state.progression.save,
        profileId,
        p1->id,
        p1->displayName,
        xp,
        gold);
    if (!award.applied) {
        return;
    }
    const int goldBalance = dragonProgressionGoldForProfile(state.progression.save, profileId);
    state.progression.lastAwardText = storyConfiguredRewardText(label, award, goldBalance);
    state.messages.lastHitText = state.progression.lastAwardText;
    state.messages.lastHitTextTicks = std::max(state.messages.lastHitTextTicks, 90);
    spawnStoryRewardFeedback(state, state.fighters[0], award);
    try {
        saveDragonProgressionSave(
            state.progression.savePath.empty() ? dragonProgressionSavePath(state.gameRoot) : state.progression.savePath,
            state.progression.save);
    } catch (const std::exception& ex) {
        SDL_Log("Dragon story configured reward save failed: %s", ex.what());
    }
}

void awardStoryWaveAndBoardRewards(AppState& state, bool finalWave) {
    if (const StoryBoardWaveSpec* wave = activeStoryWaveSpec(state)) {
        awardStoryConfiguredReward(state, wave->rewardXp, wave->rewardGold, "WAVE");
    }
    if (finalWave) {
        if (const StoryBoardNode* node = activeStoryBoardNode(state)) {
            awardStoryConfiguredReward(state, node->rewardXp, node->rewardGold, "BOARD");
        }
    }
}

void markStoryDefeatedEnemies(AppState& state) {
    const int active = std::clamp(state.story.activeWaveEnemyCount, 0, kStoryMaxEnemies);
    for (size_t i = 1; i < state.fighters.size(); ++i) {
        auto& fighter = state.fighters[i];
        if (fighter.life > 0) {
            continue;
        }
        const size_t enemySlot = i - 1;
        if (static_cast<int>(i) <= active
            && enemySlot < state.story.enemyRewarded.size()
            && !state.story.enemyRewarded[enemySlot]) {
            state.story.enemyRewarded[enemySlot] = true;
            awardStoryEnemyDefeat(state, fighter);
        }
        fighter.life = 0;
        fighter.ctrl = false;
        fighter.targetIndex = -1;
        fighter.targetTicks = 0;
        enterArenaFallbackDefeatPose(state, fighter, arenaFighterNeedsForcedDefeatPose(fighter));
        holdArenaDefeatedRecoveryPose(state, fighter);
    }
}

void startStoryRoundFinish(AppState& state, bool playerWon) {
    state.matchPhase = MatchPhase::RoundFinish;
    state.matchPhaseTicks = 0;
    state.roundWinner = playerWon ? 1 : 2;
    state.roundEndReason = RoundEndReason::Ko;
    state.matchComplete = true;
    state.story.stageClear = playerWon;
    state.story.stageFailed = !playerWon;
    state.frontend.selectedMatchResultOption = 0;

    for (size_t i = 0; i < state.fighters.size(); ++i) {
        auto& fighter = state.fighters[i];
        fighter.ctrl = false;
        fighter.targetIndex = -1;
        fighter.targetTicks = 0;
        fighter.vx = 0.0f;
        fighter.depthVz = 0.0f;
        if ((playerWon && i == 0) || (!playerWon && i > 0 && fighter.life > 0)) {
            enterStateIfAvailable(state, fighter, 181);
        } else if (fighter.life <= 0 || (!playerWon && i == 0)) {
            enterArenaFallbackDefeatPose(state, fighter, true);
        }
        holdArenaDefeatedRecoveryPose(state, fighter);
    }
    state.messages.lastHitText = playerWon ? "STAGE CLEAR" : "MISSION FAILED";
    state.messages.lastHitTextTicks = singleFightRoundFinishHoldTicks(state);
}

void updateStoryPhaseTimers(AppState& state) {
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
        markStoryDefeatedEnemies(state);
        if (state.matchPhaseTicks >= singleFightRoundFinishHoldTicks(state)) {
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
            awardProgressionForMatchIfNeeded(state);
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

void updateStoryCamera(AppState& state, const StageSlot& stage) {
    if (!stage.openborScrolling) {
        updateArenaCamera(state, stage);
        return;
    }

    const float minCamera = storyScrollMinCamera(stage);
    const float maxCamera = (state.story.stageClear || state.story.shopDoorAvailable)
        ? storyScrollMaxCamera(stage)
        : storyWaveCameraGate(state, stage);
    if (minCamera > maxCamera) {
        updateArenaCamera(state, stage);
        return;
    }

    state.cameraX = std::clamp(state.cameraX, minCamera, maxCamera);
    if (!state.fighters.empty() && state.fighters[0].life > 0) {
        const float halfWidth = storyGameplayHalfWidth(state, stage);
        const float leadEdge = state.cameraX + halfWidth - stage.openborScrollLead;
        float targetX = state.cameraX;
        if (state.fighters[0].x > leadEdge) {
            targetX += state.fighters[0].x - leadEdge;
        }
        targetX = std::clamp(targetX, minCamera, maxCamera);
        if (targetX > state.cameraX) {
            state.cameraX = std::min(targetX, state.cameraX + stage.openborScrollSpeed);
        }
    }
    state.cameraY = std::clamp(stage.cameraStarty, stage.cameraBoundhigh, stage.cameraBoundlow);
    updateArenaCameraRotation(state);
}

void applyStoryScreenBounds(AppState& state, const StageSlot& stage) {
    applyScreenBounds(state, stage);
    if (!stage.openborScrolling || state.fighters.empty()) {
        return;
    }

    const float halfWidth = storyGameplayHalfWidth(state, stage);
    const float waveGate = state.story.shopDoorAvailable ? storyScrollMaxCamera(stage) : storyWaveCameraGate(state, stage);
    const float maxPlayableX = std::min(stage.rightbound, waveGate + halfWidth - stage.screenright);
    state.fighters[0].x = std::min(state.fighters[0].x, maxPlayableX);
}

void applyStoryHitIfNeeded(AppState& state) {
    FramePerfScope scope(state.framePerf, FramePerfSection::CollisionHitRouting);
    if (state.fighters.empty() || state.fighters[0].life <= 0) {
        return;
    }

    const int active = std::clamp(state.story.activeWaveEnemyCount, 0, kStoryMaxEnemies);
    for (int enemy = 1; enemy <= active && enemy < static_cast<int>(state.fighters.size()); ++enemy) {
        if (!arenaFighterCanReceiveHit(state.fighters[static_cast<size_t>(enemy)])) {
            continue;
        }
        applyHitBetween(state, 0, static_cast<size_t>(enemy));
    }
    for (int enemy = 1; enemy <= active && enemy < static_cast<int>(state.fighters.size()); ++enemy) {
        if (state.fighters[static_cast<size_t>(enemy)].life <= 0 || !arenaFighterCanReceiveHit(state.fighters[0])) {
            continue;
        }
        applyHitBetween(state, static_cast<size_t>(enemy), 0);
    }

    const size_t helperBase = state.fighters.size();
    for (size_t i = 0; i < state.helpers.size(); ++i) {
        const auto& helper = state.helpers[i];
        if (helper.destroyRequested || helper.ownerIndex < 0 || helper.ownerIndex >= static_cast<int>(state.fighters.size())) {
            continue;
        }
        const int defender = storyProjectileDefenderIndex(state, helper.ownerIndex);
        if (defender >= 0 && arenaFighterCanReceiveHit(state.fighters[static_cast<size_t>(defender)])) {
            applyHitBetween(state, helperBase + i, static_cast<size_t>(defender));
        }
    }
}

bool storyShopDoorRouteAvailable(const AppState& state) {
    return nextStoryShopBoardNode(state) != nullptr;
}

const StoryBoardNode* storyShopDoorConfigNode(const AppState& state) {
    return nextStoryShopBoardNode(state);
}

float storyShopDoorOffsetX(const AppState& state) {
    if (const StoryBoardNode* node = storyShopDoorConfigNode(state)) {
        return node->shopDoorOffsetX;
    }
    return 160.0f;
}

float storyShopDoorRadiusX(const AppState& state) {
    if (const StoryBoardNode* node = storyShopDoorConfigNode(state)) {
        return node->shopDoorRadiusX;
    }
    return 56.0f;
}

float storyShopDoorRadiusZ(const AppState& state) {
    if (const StoryBoardNode* node = storyShopDoorConfigNode(state)) {
        return node->shopDoorRadiusZ;
    }
    return 44.0f;
}

std::string storyShopDoorPromptText(const AppState& state) {
    if (const StoryBoardNode* node = storyShopDoorConfigNode(state); node && !node->shopDoorPrompt.empty()) {
        return node->shopDoorPrompt;
    }
    return "LK / X SHOP";
}

std::string storyShopDoorEnterText(const AppState& state) {
    if (const StoryBoardNode* node = storyShopDoorConfigNode(state); node && !node->shopDoorEnterText.empty()) {
        return node->shopDoorEnterText;
    }
    return "ENTERING SHOP";
}

std::string storyShopDoorOpenText(const AppState& state) {
    if (const StoryBoardNode* node = storyShopDoorConfigNode(state); node && !node->shopDoorOpenText.empty()) {
        return node->shopDoorOpenText;
    }
    return "SHOP DOOR OPEN";
}

std::string storyWaveClearText(const AppState& state) {
    if (const StoryBoardWaveSpec* wave = activeStoryWaveSpec(state); wave && !wave->clearText.empty()) {
        return wave->clearText;
    }
    if (const StoryBoardNode* node = activeStoryBoardNode(state); node && !node->waveClearText.empty()) {
        return node->waveClearText;
    }
    switch (storyWaveRole(state, state.story.waveIndex)) {
    case StoryWaveRole::MidBoss:
        return "MID BOSS CLEAR";
    case StoryWaveRole::Boss:
        return "BOSS CLEAR";
    case StoryWaveRole::Normal:
    default:
        break;
    }
    return "WAVE CLEAR";
}

float storyShopDoorX(const AppState& state, const StageSlot& stage) {
    const float halfWidth = storyGameplayHalfWidth(state, stage);
    const float maxCamera = storyScrollMaxCamera(stage);
    const float maxPlayableX = std::min(stage.rightbound, maxCamera + halfWidth - stage.screenright);
    return std::clamp(maxPlayableX - storyShopDoorOffsetX(state), stage.leftbound + 32.0f, stage.rightbound - 18.0f);
}

float storyShopDoorDepthZ(const AppState& state) {
    return arenaDepthActive(state)
        ? std::clamp(0.0f, state.arenaConfig.depthMin, state.arenaConfig.depthMax)
        : 0.0f;
}

bool storyPlayerInsideShopDoor(const AppState& state, const StageSlot& stage) {
    if (!state.story.shopDoorAvailable || state.fighters.empty() || state.fighters[0].life <= 0) {
        return false;
    }
    const FighterState& player = state.fighters[0];
    return std::abs(player.x - storyShopDoorX(state, stage)) <= storyShopDoorRadiusX(state)
        && std::abs(player.depthZ - storyShopDoorDepthZ(state)) <= storyShopDoorRadiusZ(state);
}

bool storyShopDoorPromptVisible(const AppState& state, const StageSlot& stage) {
    return state.frontend.pendingMode == PendingMode::Story
        && state.matchPhase == MatchPhase::Fight
        && !state.story.stageClear
        && !state.story.stageFailed
        && !state.story.pendingShopDoorTransition
        && storyPlayerInsideShopDoor(state, stage);
}

FighterInputState storyShopDoorLiveInput(AppState& state) {
    const bool* keys = gFightInputOverride ? nullptr : SDL_GetKeyboardState(nullptr);
    return gFightInputOverride && gFightInputOverride->p1
        ? *gFightInputOverride->p1
        : collectMappedFighterInput(keys, controlProfileForPlayer(state, 0), assignedGamepad(state, 0));
}

bool storyShopDoorActionPressed(AppState& state) {
    const bool now = storyShopDoorLiveInput(state).a;
    const bool pressed = now && !state.story.shopDoorActionHeld;
    state.story.shopDoorActionHeld = now;
    return pressed;
}

void updateStoryShopDoorTrigger(AppState& state, const StageSlot& stage) {
    const bool visible = storyShopDoorPromptVisible(state, stage);
    const bool actionPressed = storyShopDoorActionPressed(state);
    if (!visible) {
        if (state.story.shopDoorPromptWasVisible && state.story.shopDoorAvailable && actionPressed) {
            state.story.pendingShopDoorTransition = true;
            state.story.shopDoorPromptWasVisible = false;
            state.messages.lastHitText = storyShopDoorEnterText(state);
            state.messages.lastHitTextTicks = 30;
            return;
        }
        state.story.shopDoorPromptWasVisible = false;
        return;
    }
    state.story.shopDoorPromptWasVisible = true;
    state.messages.lastHitText = storyShopDoorPromptText(state);
    state.messages.lastHitTextTicks = std::max(state.messages.lastHitTextTicks, 6);
    if (actionPressed) {
        state.story.pendingShopDoorTransition = true;
        state.story.shopDoorPromptWasVisible = false;
        state.messages.lastHitText = storyShopDoorEnterText(state);
        state.messages.lastHitTextTicks = 30;
    }
}

void updateStoryWaveRules(AppState& state, const StageSlot& stage) {
    if (state.matchPhase != MatchPhase::Fight) {
        return;
    }
    if (state.fighters.empty() || state.fighters[0].life <= 0) {
        startStoryRoundFinish(state, false);
        return;
    }

    if (livingStoryEnemyCount(state) > 0) {
        state.story.waveTransitionTicks = 0;
        return;
    }

    if (state.story.waveTransitionTicks == 0) {
        state.story.enemiesDefeated = std::min(
            state.story.totalEnemies,
            state.story.enemiesDefeated + state.story.activeWaveEnemyCount);
        const int waves = storyWaveCount(state);
        awardStoryWaveAndBoardRewards(state, state.story.waveIndex + 1 >= waves);
        if (state.story.waveIndex + 1 >= waves && storyShopDoorRouteAvailable(state)) {
            state.messages.lastHitText = storyShopDoorOpenText(state);
        } else {
            state.messages.lastHitText = state.story.waveIndex + 1 >= waves ? "STAGE CLEAR" : storyWaveClearText(state);
        }
        state.messages.lastHitTextTicks = 90;
    }

    ++state.story.waveTransitionTicks;
    if (state.story.waveIndex + 1 >= storyWaveCount(state)) {
        if (storyShopDoorRouteAvailable(state)) {
            state.story.shopDoorAvailable = true;
            return;
        }
        if (state.story.waveTransitionTicks >= 45) {
            startStoryRoundFinish(state, true);
        }
        return;
    }

    if (state.story.waveTransitionTicks >= 45) {
        ++state.story.waveIndex;
        startStoryWave(state, stage, false);
        updateStoryFighterFacing(state);
        const StoryWaveRole role = storyWaveRole(state, state.story.waveIndex);
        state.messages.lastHitText = role == StoryWaveRole::Normal
            ? "WAVE " + std::to_string(state.story.waveIndex + 1)
            : std::string(storyWaveRoleLabel(role));
        state.messages.lastHitTextTicks = 90;
    }
}

void updateStoryRoundFinishWorld(AppState& state, const StageSlot& stage) {
    markStoryDefeatedEnemies(state);
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

void updateStoryFight(AppState& state) {
    const StageSlot fallbackStage;
    const StageSlot& stage = selectedStageSlot(state.selection) ? *selectedStageSlot(state.selection) : fallbackStage;

    updateRuntimeEffects(state);
    updateStoryRewardFeedback(state);
    clearFightAssertSpecialFlags(state);
    updateFightAssertSpecialControllers(state, stage);
    resetFighterOneTickBounds(state);

    if (state.matchPhase == MatchPhase::RoundStart) {
        updateStoryPhaseTimers(state);
        updateStoryCamera(state, stage);
        return;
    }
    if (state.matchPhase == MatchPhase::RoundFinish) {
        updateStoryRoundFinishWorld(state, stage);
        updateStoryPhaseTimers(state);
        updateStoryCamera(state, stage);
        applyStoryScreenBounds(state, stage);
        return;
    }
    if (state.matchPhase == MatchPhase::RoundResult || state.matchPhase == MatchPhase::MatchResult) {
        updateStoryPhaseTimers(state);
        updateStoryCamera(state, stage);
        return;
    }

    const bool* keys = gFightInputOverride ? nullptr : SDL_GetKeyboardState(nullptr);
    updateStoryFighterFacing(state);
    markStoryDefeatedEnemies(state);

    std::vector<int> stateNoAtFrameStart;
    stateNoAtFrameStart.reserve(state.fighters.size());
    for (const auto& fighter : state.fighters) {
        stateNoAtFrameStart.push_back(fighter.stateNo);
    }

    if (!state.fighters.empty() && state.fighters[0].life > 0 && fighterCanUpdateDuringGlobalPause(state, 0)) {
        FighterState* target = storyTargetForFighter(state, 0);
        const FighterInputState p1Input = gFightInputOverride && gFightInputOverride->p1
            ? *gFightInputOverride->p1
            : collectMappedFighterInput(keys, controlProfileForPlayer(state, 0), assignedGamepad(state, 0));
        updateControlledFighter(state, state.fighters[0], target, p1Input);
    }

    for (size_t i = 1; i < state.fighters.size(); ++i) {
        auto& fighter = state.fighters[i];
        FighterState* target = storyTargetForFighter(state, i);
        if (!storyActorCanUpdate(state, i)
            || !target
            || state.suppressArenaCpu
            || !fighterCanUpdateDuringGlobalPause(state, static_cast<int>(i))) {
            if (fighter.life > 0) {
                fighter.vx = 0.0f;
                fighter.vy = 0.0f;
            }
            continue;
        }
        updateCpuOpponent(state, fighter, *target);
    }

    for (auto& fighter : state.fighters) {
        updateStateZeroFromMovement(state, fighter);
    }

    std::vector<bool> canUpdate(state.fighters.size(), false);
    for (size_t i = 0; i < state.fighters.size(); ++i) {
        canUpdate[i] = storyActorCanUpdate(state, i) && fighterCanUpdateDuringGlobalPause(state, static_cast<int>(i));
        if (!canUpdate[i]) {
            continue;
        }
        FighterState* target = storyTargetForFighter(state, i);
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
        FighterState* target = storyTargetForFighter(state, i);
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
    updateStoryFighterFacing(state);
    for (auto& fighter : state.fighters) {
        updateStateZeroFromMovement(state, fighter);
    }
    updateComboCounterBreaks(state);
    updateRuntimeProjectiles(state, stage);
    if (!globalPauseActive(state) || state.globalPauseOwnerMoveTicks > 0) {
        applyStoryHitIfNeeded(state);
    }
    markStoryDefeatedEnemies(state);

    for (size_t i = 0; i < state.fighters.size(); ++i) {
        if (!canUpdate[i] || state.fighters[i].life <= 0) {
            continue;
        }
        FighterState* target = storyTargetForFighter(state, i);
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
    updateStoryWaveRules(state, stage);

    for (size_t i = 0; i < state.fighters.size(); ++i) {
        if (i == 0 && state.fighters[i].life <= 0) {
            enterArenaFallbackDefeatPose(state, state.fighters[i], true);
            holdArenaDefeatedRecoveryPose(state, state.fighters[i]);
        } else {
            updateArenaDefeatedFighterPose(state, i, stage);
        }
    }

    updateStoryCamera(state, stage);
    applyStoryScreenBounds(state, stage);
    updateStoryShopDoorTrigger(state, stage);
    updateStoryPhaseTimers(state);

    for (size_t i = 0; i < state.fighters.size(); ++i) {
        const bool enteredNewState = i < stateNoAtFrameStart.size()
            && state.fighters[i].stateNo != stateNoAtFrameStart[i]
            && state.fighters[i].stateTime == 0;
        if (!enteredNewState
            && state.fighters[i].hitPauseTicks <= 0
            && storyActorCanUpdate(state, i)
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
