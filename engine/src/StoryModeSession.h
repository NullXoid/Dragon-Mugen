#pragma once

// Internal App.cpp implementation header for Story Mode round/session setup.
// Include only from FightSessionRuntime.h after common reset helpers exist.

float storyScrollMinCamera(const StageSlot& stage) {
    return stage.openborScrolling
        ? std::max(stage.cameraBoundleft, std::min(stage.openborScrollStartx, stage.openborScrollEndx))
        : stage.cameraBoundleft;
}

float storyScrollMaxCamera(const StageSlot& stage) {
    return stage.openborScrolling
        ? std::min(stage.cameraBoundright, std::max(stage.openborScrollStartx, stage.openborScrollEndx))
        : stage.cameraBoundright;
}

float storyWaveCameraGate(const AppState& state, const StageSlot& stage) {
    const float minCamera = storyScrollMinCamera(stage);
    const float maxCamera = storyScrollMaxCamera(stage);
    if (minCamera >= maxCamera) {
        return maxCamera;
    }
    const int waves = storyWaveCount(state);
    const float progress = static_cast<float>(std::clamp(state.story.waveIndex + 1, 1, waves))
        / static_cast<float>(waves);
    return std::clamp(minCamera + (maxCamera - minCamera) * progress, minCamera, maxCamera);
}

void applyStoryDifficultyToEnemy(AppState& state, FighterState& fighter, size_t fighterIndex) {
    const StoryDifficultyTuning tuning = storyDifficultyTuning(state.story.difficulty);
    const int baseLife = characterMaxLifeForFighterIndex(state, fighterIndex);
    fighter.maxLifeOverride = scaleStoryDifficultyLife(baseLife, state.story.difficulty);
    fighter.life = fighter.maxLifeOverride;
    fighter.modeAttackMultiplier = tuning.attackMultiplier;
    fighter.modeDefenceMultiplier = tuning.defenceMultiplier;
}

void resetStoryFighterCommon(AppState& state, FighterState& fighter, size_t fighterIndex, const StageSlot& stage) {
    fighter = FighterState{};
    fighter.onGround = true;
    fighter.y = 0.0f;
    fighter.depthVz = 0.0f;
    fighter.arenaDepthModifierHeld = false;
    fighter.arenaDepthModifierLastTapFrame = -100000;
    fighter.arenaDepthSidestepTicks = 0;
    fighter.arenaDepthSidestepVelocity = 0.0f;
    fighter.arenaDepthSidestepDirection = 1;
    fighter.life = characterMaxLifeForFighterIndex(state, fighterIndex);
    fighter.power = 0;
    fighter.paletteNo = fighterPaletteNoForSlot(state, fighterIndex);
    fighter.attackDistanceOverride = -1;
    fighter.victoryQuote = -1;
    clearFighterHitRuntime(fighter);
    clearFighterVariables(fighter);
    clearFighterVisualRuntime(fighter);
    applyInitialFighterScale(state, fighter, fighterIndex);
    enterRoundInitialState(state, fighter);
    fighter.x = clampFighterOriginToStage(fighter.x, stage);
}

void startStoryWave(AppState& state, const StageSlot& stage, bool resetPlayer) {
    state.story.activeWaveEnemyCount = storyWaveEnemyCount(state, state.story.waveIndex);
    state.story.waveTransitionTicks = 0;
    state.story.enemyRewarded = {};
    state.story.shopDoorAvailable = false;
    state.story.pendingShopDoorTransition = false;
    const float halfWidth = logicalWidthF(state) * 0.5f;
    const float minCamera = storyScrollMinCamera(stage);
    const float maxCamera = storyWaveCameraGate(state, stage);

    if (!state.fighters.empty() && resetPlayer) {
        auto& player = state.fighters[0];
        resetStoryFighterCommon(state, player, 0, stage);
        player.x = clampFighterOriginToStage(stage.openborScrolling ? std::max(stage.p1startx, minCamera - halfWidth + 70.0f) : stage.p1startx, stage);
        player.y = stage.p1starty;
        player.depthZ = 0.0f;
        player.facing = 1;
    }

    const float enemyBaseX = std::clamp(
        std::max(stage.p2startx, state.cameraX + halfWidth - 42.0f),
        stage.leftbound + 24.0f,
        stage.rightbound - 24.0f);
    static constexpr std::array<float, kStoryMaxEnemies> depths{ 0.0f, -22.0f, 22.0f };
    const StoryBoardWaveSpec* waveSpec = activeStoryWaveSpec(state);
    for (int i = 1; i < static_cast<int>(state.fighters.size()); ++i) {
        auto& enemy = state.fighters[static_cast<size_t>(i)];
        if (i <= state.story.activeWaveEnemyCount) {
            resetStoryFighterCommon(state, enemy, static_cast<size_t>(i), stage);
            applyStoryDifficultyToEnemy(state, enemy, static_cast<size_t>(i));
            const StoryBoardWaveEnemy* waveEnemy =
                waveSpec && i - 1 < static_cast<int>(waveSpec->enemies.size())
                    ? &waveSpec->enemies[static_cast<size_t>(i - 1)]
                    : nullptr;
            const float xOffset = waveEnemy && waveEnemy->hasXOffset
                ? waveEnemy->xOffset
                : static_cast<float>((i - 1) * 34);
            enemy.x = clampFighterOriginToStage(std::min(enemyBaseX + xOffset, maxCamera + halfWidth - 24.0f), stage);
            enemy.y = i % 2 == 0 ? stage.p1starty : stage.p2starty;
            enemy.depthZ = arenaDepthActive(state)
                ? std::clamp(
                    waveEnemy && waveEnemy->hasDepthZ ? waveEnemy->depthZ : depths[static_cast<size_t>(i - 1)],
                    state.arenaConfig.depthMin,
                    state.arenaConfig.depthMax)
                : 0.0f;
            enemy.facing = state.fighters.empty() || enemy.x >= state.fighters[0].x ? -1 : 1;
        } else {
            enemy = FighterState{};
            enemy.life = 0;
            enemy.ctrl = false;
            enemy.x = clampFighterOriginToStage(enemyBaseX + 220.0f + static_cast<float>(i * 28), stage);
            enemy.y = stage.p2starty;
            enemy.onGround = true;
            enemy.depthZ = 0.0f;
            enemy.facing = -1;
        }
    }
}

void resetStoryFightRound(AppState& state) {
    const StageSlot fallbackStage;
    const StageSlot& stage = selectedStageSlot(state.selection) ? *selectedStageSlot(state.selection) : fallbackStage;

    commitSelectedStoryBoardNode(state);
    chooseStoryEnemyCharacters(state);
    state.fighters.assign(static_cast<size_t>(storyFighterCount()), FighterState{});
    state.helpers.clear();
    state.projectiles.clear();

    state.story.waveIndex = 0;
    state.story.activeWaveEnemyCount = storyWaveEnemyCount(state, 0);
    state.story.enemiesDefeated = 0;
    state.story.totalEnemies = storyTotalEnemyCount(state);
    state.story.waveTransitionTicks = 0;
    state.story.shopDoorAvailable = false;
    state.story.pendingShopDoorTransition = false;
    state.story.stageClear = false;
    state.story.stageFailed = false;
    state.story.enemyRewarded = {};
    state.story.rewardPopups.clear();
    state.story.rewardCoins.clear();

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
    state.frontend.fightPauseOpen = false;
    state.frontend.singleFightPauseOpen = false;
    state.frontend.selectedSingleFightPauseOption = 0;
    state.frontend.selectedMatchResultOption = 0;
    state.matchPhase = MatchPhase::RoundStart;
    state.roundEndReason = RoundEndReason::None;
    state.matchTimerTicks = 0;
    state.matchPhaseTicks = 0;
    state.roundWinner = 0;
    state.roundScoreApplied = false;
    state.roundPoseApplied = false;
    state.matchComplete = false;
    state.cameraX = stage.openborScrolling ? storyScrollMinCamera(stage) : stage.cameraStartx;
    state.cameraY = stage.cameraStarty;
    state.arenaCameraYawDeg = 0.0f;
    state.arenaCameraTargetYawDeg = 0.0f;

    startStoryWave(state, stage, true);
}
