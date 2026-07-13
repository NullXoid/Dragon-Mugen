#pragma once

#include "AppVerificationProbeHelpers.h"
#include "AppVerificationPerformanceBridge.h"
#include "AppVerificationScreenshotCapture.h"

class AppVerificationRuntime final : public verification::RuntimeProbe {
public:
    explicit AppVerificationRuntime(std::filesystem::path gameRoot)
        : gameRoot_(std::move(gameRoot)) {}

    ~AppVerificationRuntime() override {
        resetSessionResources();
        if (sdlInitialized_) {
            SDL_Quit();
        }
    }

    bool setup(
        std::string_view p1Id,
        std::string_view stageHint,
        verification::ScenarioMode mode,
        std::ostream& out,
        int arenaCpuCount = 1) override {
        if (!prepareVerificationShell(out) || !loadVerificationContent(out)) {
            return false;
        }

        state_.selection.selectedCharacter = findCharacterIndex(p1Id);
        if (mode == verification::ScenarioMode::Arena) {
            state_.frontend.pendingMode = PendingMode::Arena;
            setArenaDefaultsFromConfig(state_);
            state_.selection.sessionSlots.arenaCpuCount = arenaCpuCount;
            setArenaCpuCount(state_, arenaCpuCount);
            state_.selection.sessionSlots.opponentType = OpponentType::Cpu;
            selectArenaDefaultStage(state_);
        } else if (mode == verification::ScenarioMode::Story) {
            state_.frontend.pendingMode = PendingMode::Story;
            state_.selection.sessionSlots.opponentType = OpponentType::Cpu;
            selectStoryDefaultBoardNode(state_);
        } else if (mode == verification::ScenarioMode::Versus) {
            state_.frontend.pendingMode = PendingMode::SingleFight;
            state_.selection.selectedP2Character =
                defaultP2CharacterIndex(state_.selection, state_.selection.selectedCharacter);
        } else {
            state_.frontend.pendingMode = mode == verification::ScenarioMode::SinglePlayer
                ? PendingMode::SinglePlayer
                : PendingMode::Training;
        }
        configureFightSessionSlotsFromSelection(state_);
        if (mode == verification::ScenarioMode::Arena) {
            selectArenaDefaultStage(state_);
        } else if (mode == verification::ScenarioMode::Story) {
            selectStoryDefaultBoardNode(state_);
        } else {
            selectPreferredStage(state_);
        }
        if (!stageHint.empty()) {
            state_.selection.selectedStage = findStageIndex(stageHint);
        }

        loadVisualAssets(renderer_, state_);
        openExistingGamepads(state_);
        if (!prepareFightSession(renderer_, state_)) {
            out << "prepareFightSession failed\n";
            return false;
        }
        beginFight(state_);
        return true;
    }

    bool setupStageSelect(std::string_view p1Id, verification::ScenarioMode mode, std::ostream& out) override {
        if (!prepareVerificationShell(out) || !loadVerificationContent(out)) {
            return false;
        }

        state_.selection.selectedCharacter = findCharacterIndex(p1Id);
        state_.frontend.pendingMode = mode == verification::ScenarioMode::Story
            ? PendingMode::Story
            : PendingMode::SinglePlayer;
        if (state_.frontend.pendingMode == PendingMode::Story) {
            state_.selection.sessionSlots.opponentType = OpponentType::Cpu;
            selectStoryDefaultBoardNode(state_);
        } else {
            selectPreferredStage(state_);
        }
        configureFightSessionSlotsFromSelection(state_);
        state_.frontend.screen = Screen::StageSelect;
        state_.frontend.screenFrame = 0;

        loadVisualAssets(renderer_, state_);
        openExistingGamepads(state_);
        return true;
    }

    bool setupArenaSetupScreen(std::string_view p1Id, std::ostream& out) override {
        if (!prepareVerificationShell(out) || !loadVerificationContent(out)) {
            return false;
        }

        state_.selection.selectedCharacter = findCharacterIndex(p1Id);
        state_.frontend.pendingMode = PendingMode::Arena;
        setArenaDefaultsFromConfig(state_);
        state_.selection.sessionSlots.arenaCpuCount = 1;
        setArenaCpuCount(state_, 1);
        state_.selection.sessionSlots.opponentType = OpponentType::Cpu;
        selectArenaDefaultStage(state_);
        configureFightSessionSlotsFromSelection(state_);
        state_.frontend.screen = Screen::ArenaSetup;
        state_.frontend.screenFrame = 0;

        loadVisualAssets(renderer_, state_);
        openExistingGamepads(state_);
        return true;
    }

    void step(const verification::SymbolicInput& p1Input, int frames) override {
        step(p1Input, verification::SymbolicInput{}, frames);
    }

    void step(
        const verification::SymbolicInput& p1Input,
        const verification::SymbolicInput& p2Input,
        int frames) override {
        const FighterInputState p1 = verificationProbeInput(p1Input);
        const FighterInputState p2 = verificationProbeInput(p2Input);
        for (int i = 0; i < frames; ++i) {
            ++state_.frame;
            ++state_.frontend.screenFrame;
            FightInputOverride inputOverride;
            inputOverride.p1 = &p1;
            inputOverride.p2 = &p2;
            const FightInputOverride* previous = gFightInputOverride;
            gFightInputOverride = &inputOverride;
            updateFight(state_);
            gFightInputOverride = previous;
            applyTrainingPowerMode(state_); consumeStoryShopDoorTransition(renderer_, state_); updateAudioMixer(state_);
        }
    }

    void pressKey(std::string_view key) override {
        const SDL_Keycode code = verificationProbeKeyCode(key);
        if (code == 0) {
            return;
        }
        handleKey(renderer_, state_, code);
    }

    bool preparePendingFight() override {
        if (state_.frontend.screen != Screen::VersusScreen) {
            return state_.frontend.screen == Screen::FightView
                && state_.matchPhase == MatchPhase::Fight;
        }
        if (!state_.fightSessionPrepared && !prepareFightSession(renderer_, state_)) {
            return false;
        }
        for (int i = 0; i < 140; ++i) {
            ++state_.frame;
            ++state_.frontend.screenFrame;
            if (state_.frontend.screen == Screen::VersusScreen
                && state_.fightSessionPrepared
                && state_.frontend.screenFrame > 120) {
                beginFight(state_);
            }
            applyTrainingPowerMode(state_);
            consumeStoryShopDoorTransition(renderer_, state_);
            updateAudioMixer(state_);
            if (state_.frontend.screen == Screen::FightView
                && state_.matchPhase == MatchPhase::Fight) {
                return true;
            }
        }
        return state_.frontend.screen == Screen::FightView
            && state_.matchPhase == MatchPhase::Fight;
    }

    void holdTrainingShowSelect(bool held, int frames) override {
        const FighterInputState p1;
        const FighterInputState p2;
        for (int i = 0; i < frames; ++i) {
            ++state_.frame;
            ++state_.frontend.screenFrame;
            updateTrainingShowSelectHold(state_, held);
            FightInputOverride inputOverride;
            inputOverride.p1 = &p1;
            inputOverride.p2 = &p2;
            const FightInputOverride* previous = gFightInputOverride;
            gFightInputOverride = &inputOverride;
            updateFight(state_);
            gFightInputOverride = previous;
            applyTrainingPowerMode(state_); consumeStoryShopDoorTransition(renderer_, state_); updateAudioMixer(state_);
        }
    }

    void positionFighters(float p1X, float p2X) override {
        resetTrainingPositions(state_);
        state_.fighters[0].x = p1X;
        state_.fighters[1].x = p2X;
        state_.fighters[0].y = 0.0f;
        state_.fighters[1].y = 0.0f;
        state_.fighters[0].triggerY = 0.0f;
        state_.fighters[1].triggerY = 0.0f;
        state_.fighters[0].depthZ = 0.0f;
        state_.fighters[1].depthZ = 0.0f;
        state_.fighters[0].depthVz = 0.0f;
        state_.fighters[1].depthVz = 0.0f;
        state_.fighters[0].arenaDepthModifierHeld = false;
        state_.fighters[1].arenaDepthModifierHeld = false;
        state_.fighters[0].arenaDepthModifierLastTapFrame = -100000;
        state_.fighters[1].arenaDepthModifierLastTapFrame = -100000;
        state_.fighters[0].arenaDepthSidestepTicks = 0;
        state_.fighters[1].arenaDepthSidestepTicks = 0;
        state_.fighters[0].arenaDepthSidestepVelocity = 0.0f;
        state_.fighters[1].arenaDepthSidestepVelocity = 0.0f;
        state_.fighters[0].arenaDepthSidestepDirection = 1;
        state_.fighters[1].arenaDepthSidestepDirection = 1;
        state_.fighters[0].facing = state_.fighters[0].x <= state_.fighters[1].x ? 1 : -1;
        state_.fighters[1].facing = -state_.fighters[0].facing;
    }

    void setFighterPosition(int fighterIndex, float x, float y) override {
        if (fighterIndex < 0 || fighterIndex >= static_cast<int>(state_.fighters.size())) {
            return;
        }
        auto& fighter = state_.fighters[static_cast<size_t>(fighterIndex)];
        fighter.x = x;
        fighter.y = y;
        fighter.triggerY = y;
        fighter.onGround = y >= 0.0f;
        if (!fighter.onGround && fighter.stateType != 'A') {
            fighter.stateType = 'A';
        }
    }

    void setFighterDepth(int fighterIndex, float depthZ) override {
        if (fighterIndex < 0 || fighterIndex >= static_cast<int>(state_.fighters.size())) {
            return;
        }
        auto& fighter = state_.fighters[static_cast<size_t>(fighterIndex)];
        fighter.depthZ = arenaDepthActive(state_)
            ? std::clamp(depthZ, state_.arenaConfig.depthMin, state_.arenaConfig.depthMax)
            : 0.0f;
        fighter.depthVz = 0.0f;
        fighter.arenaDepthSidestepTicks = 0;
        fighter.arenaDepthSidestepVelocity = 0.0f;
    }

    void setFighterLife(int fighterIndex, int life) override {
        if (fighterIndex < 0 || fighterIndex >= static_cast<int>(state_.fighters.size())) {
            return;
        }
        auto& fighter = state_.fighters[static_cast<size_t>(fighterIndex)];
        fighter.life = life;
        if (life <= 0) {
            fighter.ctrl = false;
            fighter.vx = 0.0f;
            fighter.vy = 0.0f;
            fighter.hitPauseTicks = 0;
            fighter.hitStunTicks = 0;
        }
    }

    void setFighterPower(int fighterIndex, int power) override {
        if (fighterIndex < 0 || fighterIndex >= static_cast<int>(state_.fighters.size())) {
            return;
        }
        auto& fighter = state_.fighters[static_cast<size_t>(fighterIndex)];
        fighter.power = std::clamp(power, 0, std::max(0, characterConstantsForActor(state_, fighter).maxPower));
    }

    void setFighterVar(int fighterIndex, int varIndex, int value) override {
        if (fighterIndex < 0 || fighterIndex >= static_cast<int>(state_.fighters.size()) || varIndex < 0 || varIndex >= 60) {
            return;
        }
        state_.fighters[static_cast<size_t>(fighterIndex)].vars[static_cast<size_t>(varIndex)] = value;
    }

    int fighterVar(int fighterIndex, int varIndex) const override {
        if (fighterIndex < 0 || fighterIndex >= static_cast<int>(state_.fighters.size()) || varIndex < 0 || varIndex >= 60) {
            return 0;
        }
        return state_.fighters[static_cast<size_t>(fighterIndex)].vars[static_cast<size_t>(varIndex)];
    }

    void setFighterMoveContact(int fighterIndex, bool hit, bool guarded) override {
        if (fighterIndex < 0 || fighterIndex >= static_cast<int>(state_.fighters.size())) {
            return;
        }
        auto& fighter = state_.fighters[static_cast<size_t>(fighterIndex)];
        fighter.moveContact = hit || guarded;
        fighter.moveHit = hit;
        fighter.moveGuarded = guarded;
    }

    void setFighterControl(int fighterIndex, bool enabled) override {
        if (fighterIndex < 0 || fighterIndex >= static_cast<int>(state_.fighters.size())) {
            return;
        }
        state_.fighters[static_cast<size_t>(fighterIndex)].ctrl = enabled;
    }

    void setMatchTimerTicks(int ticks) override {
        state_.matchTimerTicks = std::max(0, ticks);
    }

    void setTrainingDummyGuardMode(std::string_view mode) override {
        if (mode == "stand") {
            state_.training.options.dummyGuardMode = DummyGuardMode::Stand;
        } else if (mode == "crouch") {
            state_.training.options.dummyGuardMode = DummyGuardMode::Crouch;
        } else if (mode == "auto") {
            state_.training.options.dummyGuardMode = DummyGuardMode::Auto;
        } else {
            state_.training.options.dummyGuardMode = DummyGuardMode::Off;
        }
    }

    void setArenaZAxisEnabled(bool enabled) override {
        state_.selection.sessionSlots.arenaZAxisEnabled = enabled;
        updateArenaCameraRotation(state_);
    }

    void setArenaCameraRotationEnabled(bool enabled) override {
        state_.selection.sessionSlots.arenaCameraRotationEnabled = enabled;
        updateArenaCameraRotation(state_);
    }

    void setArenaCpuFrozen(bool frozen) override {
        state_.suppressArenaCpu = frozen;
    }

    void setStoryWave(int waveIndex) override {
        if (state_.frontend.pendingMode != PendingMode::Story || state_.fighters.empty()) {
            return;
        }
        const StageSlot fallbackStage;
        const StageSlot& stage = selectedStageSlot(state_.selection) ? *selectedStageSlot(state_.selection) : fallbackStage;
        state_.story.waveIndex = std::clamp(waveIndex, 0, storyWaveCount(state_) - 1);
        startStoryWave(state_, stage, false);
        updateStoryFighterFacing(state_);
    }

    void setFightPaused(bool paused) override {
        state_.frontend.fightPauseOpen = paused;
        if (paused) {
            state_.frontend.singleFightPauseOpen = false;
            state_.training.options.menuOpen = false;
        }
    }

    void setFighterHitPause(int fighterIndex, int ticks) override {
        if (fighterIndex < 0 || fighterIndex >= static_cast<int>(state_.fighters.size())) {
            return;
        }
        auto& fighter = state_.fighters[static_cast<size_t>(fighterIndex)];
        fighter.hitPauseTicks = std::max(0, ticks);
        if (fighter.hitPauseTicks <= 0) {
            fighter.hitPauseChangeStateControllerId = 0;
        }
    }

    void setFighterHitStun(int fighterIndex, int ticks) override {
        if (fighterIndex < 0 || fighterIndex >= static_cast<int>(state_.fighters.size())) {
            return;
        }
        state_.fighters[static_cast<size_t>(fighterIndex)].hitStunTicks = std::max(0, ticks);
    }

    void forceFighterState(int fighterIndex, int stateNo) override {
        if (fighterIndex < 0 || fighterIndex >= static_cast<int>(state_.fighters.size())) {
            return;
        }
        enterState(state_, state_.fighters[static_cast<size_t>(fighterIndex)], stateNo);
    }

    void forceFighterLiedown(int fighterIndex, int hitStunTicks) override {
        if (fighterIndex < 0 || fighterIndex >= static_cast<int>(state_.fighters.size())) {
            return;
        }
        auto& fighter = state_.fighters[static_cast<size_t>(fighterIndex)];
        enterState(state_, fighter, 5110);
        fighter.stateType = 'L';
        fighter.moveType = 'H';
        fighter.physics = 'N';
        fighter.ctrl = false;
        fighter.onGround = true;
        fighter.y = 0.0f;
        fighter.vx = 0.0f;
        fighter.vy = 0.0f;
        fighter.hitFall = true;
        fighter.hitFallRecover = false;
        fighter.hitDowned = false;
        fighter.hitStunTicks = std::max(1, hitStunTicks);
        fighter.hitPauseTicks = 0;
        setFighterAction(fighter, firstExistingAction(state_, { 5110, 5170, 5080, 0 }));
    }

    void spawnHelper(int ownerIndex, int helperId, int stateNo, int pauseMoveTime = 0, int superMoveTime = 0) override {
        if (ownerIndex < 0 || ownerIndex >= static_cast<int>(state_.fighters.size())) {
            return;
        }
        const auto& owner = state_.fighters[static_cast<size_t>(ownerIndex)];
        if (!findStateDefinitionForActor(state_, owner, stateNo)) {
            return;
        }
        FighterState helper;
        helper.helper = true;
        helper.ownerIndex = ownerIndex;
        helper.helperId = helperId;
        helper.x = owner.x;
        helper.y = owner.y;
        helper.depthZ = owner.depthZ;
        helper.facing = owner.facing;
        helper.onGround = true;
        helper.life = 1000;
        helper.pauseMoveTime = pauseMoveTime;
        helper.superMoveTime = superMoveTime;
        if (!enterState(state_, helper, stateNo)) {
            return;
        }
        state_.helpers.push_back(std::move(helper));
    }

#include "AppVerificationRuntimeTrainingMethods.inl"

    verification::RuntimePerformanceResult measurePerformance(
        int warmupFrames,
        int measuredFrames,
        bool renderEachFrame,
        bool stressInputs) override {
        return measureVerificationPerformance(state_, renderer_, warmupFrames, measuredFrames, renderEachFrame, stressInputs);
    }

    bool captureScreenshot(const std::filesystem::path& path) override {
        return captureVerificationScreenshot(renderer_, state_, path);
    }

    verification::RuntimeSnapshot snapshot() const override {
        verification::RuntimeSnapshot out;
        out.frame = state_.frame;
        out.logicalWidth = logicalWidth(state_);
        out.cameraX = state_.cameraX;
        out.cameraY = state_.cameraY;
        out.arenaCameraYawDeg = state_.arenaCameraYawDeg;
        out.arenaCameraTargetYawDeg = state_.arenaCameraTargetYawDeg;
        out.matchTimerTicks = state_.matchTimerTicks;
        out.screen = static_cast<int>(state_.frontend.screen);
        out.pendingMode = static_cast<int>(state_.frontend.pendingMode);
        out.selectedStageIndex = state_.selection.selectedStage;
        out.stageCount = static_cast<int>(state_.selection.stages.size());
        out.stageBackgroundCount = static_cast<int>(state_.stageBackground.size());
        out.matchPhase = static_cast<int>(state_.matchPhase);
        out.activeEffects = static_cast<int>(state_.runtimeEffects.size());
        out.activeSounds = static_cast<int>(state_.audio.activeVoices.size());
        const FramePerfSummary perfSummary = state_.framePerf.summary(true);
        out.perfFps = perfSummary.fps;
        out.perfAvgFrameMs = perfSummary.avgFrameMs;
        out.perfP95FrameMs = perfSummary.p95FrameMs;
        out.perfWorstFrameMs = perfSummary.worstFrameMs;
        out.perfFpsEquivalent = perfSummary.fpsEquivalent;
        out.perfDrawCalls = perfSummary.latestCounters.drawCalls;
        out.perfSkippedDraws = perfSummary.latestCounters.skippedDraws;
        out.perfFixedSteps = perfSummary.latestCounters.fixedSteps;
        out.perfDominantSection = std::string(framePerfSectionLabel(perfSummary.dominantSection));
        out.comboHits = state_.display.comboCounters[0].displayHits;
        out.fighterCount = static_cast<int>(state_.fighters.size());
        out.arenaRuntimeCount = static_cast<int>(state_.arenaRuntimes.size());
        const StageSlot fallbackStage;
        const StageSlot& stage = selectedStageSlot(state_.selection) ? *selectedStageSlot(state_.selection) : fallbackStage;
        out.selectedStageDragonSidecarAvailable = stage.dragonSidecarAvailable; out.selectedStageLegacyOpenBorSection = stage.legacyOpenBorSection;
        out.selectedStageHasMusic = !stage.bgMusicPath.empty(); out.selectedStageMusicPath = stage.bgMusicPath.generic_string();
        out.storyDifficulty = storyDifficultyIndex(state_.story.difficulty);
        out.storyBoardNodeCount = static_cast<int>(state_.story.boardRoute.nodes.size());
        out.storySelectableBoardNodeCount = storySelectableBoardNodeCount(state_);
        out.storySelectedBoardNode = state_.story.selectedBoardNode;
        out.storyActiveBoardNode = state_.story.activeBoardNode;
        out.storySelectedBoardWaves = storySelectedBoardWaveCount(state_);
        if (const StoryBoardNode* node = selectedStoryBoardNode(state_)) {
            out.storySelectedBoardKind = storyBoardNodeKindTag(node->kind);
            out.storySelectedBoardTitle = node->title;
            out.storySelectedBoardTarget = !node->shopRef.empty() ? node->shopRef : node->enemyRef;
            out.storySelectedBoardShop = node->kind == StoryBoardNodeKind::Shop;
        }
        const auto assignCharacterInfo = [&](size_t fighterIndex, std::string& outId, std::string& outName) {
            int characterIndex = -1;
            if (isStoryMode(state_)) {
                characterIndex = storyFighterCharacterIndex(state_, fighterIndex);
            } else if (fighterIndex == 0) {
                characterIndex = sessionP1CharacterIndex(state_.selection);
            } else {
                characterIndex = state_.selection.sessionSlots.opponentCharacter;
            }
            if (const CharacterSlot* character = characterSlotAt(state_.selection, characterIndex)) {
                outId = character->id;
                outName = character->displayName;
            }
        };
        assignCharacterInfo(0, out.p1CharacterId, out.p1CharacterName);
        assignCharacterInfo(1, out.p2CharacterId, out.p2CharacterName);
        out.storyForwardCueVisible = storyForwardCueVisible(state_); out.storyForwardCueImageLoaded = state_.storyForwardCueImage.texture != nullptr;
        out.storyShopDoorAvailable = state_.story.shopDoorAvailable; out.storyShopDoorPromptVisible = storyShopDoorPromptVisible(state_, stage); out.storyShopDoorTransitionPending = state_.story.pendingShopDoorTransition; out.storyResumeRouteAfterShop = state_.story.resumeRouteAfterShop; out.storyResumeBoardNodeAfterShop = state_.story.resumeBoardNodeAfterShop; out.storyShopDoorX = storyShopDoorX(state_, stage);
        out.loadingProgressActive = state_.loadingProgress.active;
        out.loadingProgressFailed = state_.loadingProgress.failed;
        out.loadingProgressFraction = loadingProgressFraction(state_.loadingProgress);
        out.loadingProgressPhase = state_.loadingProgress.phase;
        if (state_.fighters.size() < 2) {
            return out;
        }
        out.globalPauseTicks = state_.globalPauseTicks;
        out.globalPauseOwnerMoveTicks = state_.globalPauseOwnerMoveTicks;
        out.globalPauseIsSuper = state_.globalPauseIsSuper;
        out.p1RuntimeStates = static_cast<int>(stateDefinitionsForActor(state_, state_.fighters[0]).size());
        out.p2RuntimeStates = static_cast<int>(stateDefinitionsForActor(state_, state_.fighters[1]).size());
        out.p1RuntimeHitDefs = static_cast<int>(hitDefinitionsForActor(state_, state_.fighters[0]).size());
        out.p2RuntimeHitDefs = static_cast<int>(hitDefinitionsForActor(state_, state_.fighters[1]).size());
        out.p1RuntimeCommandEntries = static_cast<int>(commandEntriesForActor(state_, state_.fighters[0]).size());
        out.p2RuntimeCommandEntries = static_cast<int>(commandEntriesForActor(state_, state_.fighters[1]).size());
        out.runtimeMode = dragonRuntimeModeName(state_.runtimeMode);
        out.p1CompatibilityProfile = compatibilityProfileName(state_.characterCompatibility.contentProfile);
        out.p1LocalCoordWidth = state_.characterCompatibility.localCoord.width;
        out.p1LocalCoordHeight = state_.characterCompatibility.localCoord.height;
        out.p1UsesMugenSemantics = usesMugenSemantics(state_.characterCompatibility);
        out.p1AllowsDragonExtensions = allowsDragonExtensions(state_.characterCompatibility);
        out.p1AllowsArenaExtensions = allowsArenaExtensions(state_.characterCompatibility);
        const CompatibilityContext& p2Compatibility = state_.fighters.size() > 1
            ? compatibilityContextForActor(state_, state_.fighters[1])
            : state_.opponentRuntime.compatibility;
        out.p2CompatibilityProfile = compatibilityProfileName(p2Compatibility.contentProfile);
        out.p2LocalCoordWidth = p2Compatibility.localCoord.width;
        out.p2LocalCoordHeight = p2Compatibility.localCoord.height;
        out.p2UsesMugenSemantics = usesMugenSemantics(p2Compatibility);
        for (const auto& fighter : state_.fighters) {
            if (fighter.life > 0) {
                ++out.livingFighters;
            }
        }
        for (const auto& helper : state_.helpers) {
            if (helper.destroyRequested) {
                continue;
            }
            if (out.activeHelpers == 0) {
                out.firstHelperState = helper.stateNo;
                out.firstHelperAction = helper.action;
                out.firstHelperAnimTick = helper.animTick;
            }
            ++out.activeHelpers;
            if (helper.stateNo == 0) {
                ++out.idleHelpers;
            }
        }
        out.roundWinner = state_.roundWinner;
        out.roundEndReason = static_cast<int>(state_.roundEndReason);
        out.roundWinsP1 = state_.roundWins[0];
        out.roundWinsP2 = state_.roundWins[1];
        out.matchComplete = state_.matchComplete;
        out.matchWinner = matchWinner(state_);
        out.storyWaveIndex = state_.story.waveIndex;
        out.storyActiveEnemies = state_.story.activeWaveEnemyCount;
        out.storyLivingEnemies = livingStoryEnemyCount(state_);
        out.storyEnemiesDefeated = state_.story.enemiesDefeated;
        out.storyTotalEnemies = state_.story.totalEnemies;
        out.storyStageClear = state_.story.stageClear;
        out.storyStageFailed = state_.story.stageFailed;
        out.storyNextRouteBoardNode = nextStoryRouteBoardNodeIndex(state_, state_.story.activeBoardNode);
        out.storyCanContinueRoute = storyCanContinueRoute(state_);
        out.fightPauseOpen = state_.frontend.fightPauseOpen;
        out.singleFightPauseOpen = state_.frontend.singleFightPauseOpen;
        out.trainingOptionsOpen = state_.training.options.menuOpen;
        out.selectedSingleFightPauseOption = state_.frontend.selectedSingleFightPauseOption;
        out.selectedMatchResultOption = state_.frontend.selectedMatchResultOption;
        out.arenaZAxisEnabled = arenaDepthActive(state_);
        out.arenaCameraRotationSelected = arenaCameraRotationSelected(state_);
        out.arenaCameraRotationActive = arenaCameraRotationActive(state_);
        out.lastHitText = state_.messages.lastHitText;
        out.progressionAwardText = state_.progression.lastAwardText;
        out.storyRewardPopups = static_cast<int>(state_.story.rewardPopups.size());
        out.storyRewardCoins = static_cast<int>(state_.story.rewardCoins.size());
        if (state_.progression.loaded && state_.progression.data.config.enabled) {
            const std::string p1ProfileId = dragonProgressionPlayerProfileId(state_.progression.save, 0);
            out.progressionGoldBalance = dragonProgressionGoldForProfile(state_.progression.save, p1ProfileId);
        }
        const auto& trainingEntries = activeDisplayableMoveListEntries(state_);
        if (!trainingEntries.empty()) {
            const int selected = std::clamp(
                state_.training.options.selectedMoveListEntry,
                0,
                static_cast<int>(trainingEntries.size()) - 1);
            out.selectedTrainingMoveLabel = moveListEntryName(*trainingEntries[static_cast<size_t>(selected)]);
        }
        const auto appendCommands = [](std::string& text, const std::vector<std::string>& commands) {
            for (size_t i = 0; i < commands.size(); ++i) {
                if (i > 0) {
                    text += ",";
                }
                text += commands[i];
            }
        };
        appendCommands(out.p1Commands, collectCurrentFighterCommands(state_, state_.fighters[0]));
        appendCommands(out.p2Commands, collectCurrentFighterCommands(state_, state_.fighters[1]));
        out.p1 = verificationProbeFighterSnapshot(state_.fighters[0], characterMaxLifeForFighterIndex(state_, 0));
        out.p2 = verificationProbeFighterSnapshot(state_.fighters[1], characterMaxLifeForFighterIndex(state_, 1));
        out.p1AnimElem = currentAnimElemForFighter(state_, state_.fighters[0]);
        out.p2AnimElem = currentAnimElemForFighter(state_, state_.fighters[1]);
        out.p1P2BodyDistX = p2BodyDistXValue(state_, state_.fighters[0], state_.fighters[1]);
        if (const AnimationFrame* p1Frame = currentFrameForFighter(state_, state_.fighters[0])) {
            out.p1Clsn1Count = static_cast<int>(p1Frame->clsn1.size());
            out.p1Clsn2Count = static_cast<int>(p1Frame->clsn2.size());
            if (const AnimationFrame* p2Frame = currentFrameForFighter(state_, state_.fighters[1])) {
                out.p2Clsn1Count = static_cast<int>(p2Frame->clsn1.size());
                out.p2Clsn2Count = static_cast<int>(p2Frame->clsn2.size());
                out.p1P2BoxesOverlap = !p1Frame->clsn1.empty()
                    && !p2Frame->clsn2.empty()
                    && fighterBoxesOverlap(state_.fighters[0], *p1Frame, state_.fighters[1], *p2Frame);
            }
        }
        if (const HitDefinition* activeHitDef = findActiveHitDefinition(state_, state_.fighters[0], state_.fighters[1], 1)) {
            out.p1ActiveHitDef = true;
            out.p1HitFlagAllowsP2 = hitFlagAllowsDefender(*activeHitDef, state_.fighters[1]);
            out.p2HittableByP1 = defenderCanBeHitBy(state_.fighters[1], *activeHitDef);
        }
        applyProjectedSnapshot(stage, state_.fighters[0], out.p1);
        applyProjectedSnapshot(stage, state_.fighters[1], out.p2);
        std::vector<int> drawOrder;
        drawOrder.reserve(state_.fighters.size());
        for (int i = 0; i < static_cast<int>(state_.fighters.size()); ++i) {
            drawOrder.push_back(i);
        }
        std::stable_sort(drawOrder.begin(), drawOrder.end(), [this](int lhs, int rhs) {
            const auto& left = state_.fighters[static_cast<size_t>(lhs)];
            const auto& right = state_.fighters[static_cast<size_t>(rhs)];
            if (left.sprPriority != right.sprPriority) {
                return left.sprPriority < right.sprPriority;
            }
            const float leftDepth = arenaProjectedViewDepth(state_, left.x, arenaActorDepth(state_, left));
            const float rightDepth = arenaProjectedViewDepth(state_, right.x, arenaActorDepth(state_, right));
            if (arenaDepthActive(state_) && std::fabs(leftDepth - rightDepth) > 0.001f) {
                return leftDepth < rightDepth;
            }
            return lhs < rhs;
        });
        for (size_t i = 0; i < drawOrder.size(); ++i) {
            if (i > 0) {
                out.arenaDrawOrder += ",";
            }
            out.arenaDrawOrder += std::to_string(drawOrder[i]);
        }
        return out;
    }

    std::string rootText() const override { return gameRoot_.string(); }
    std::string stageName() const override { return selectedStageName(state_.selection); }
    std::string p1Name() const override {
        if (const CharacterSlot* character = sessionP1CharacterSlot(state_.selection)) {
            return character->displayName;
        }
        return {};
    }

private:
    bool prepareVerificationShell(std::ostream& out) {
        if (!sdlInitialized_) {
            if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
                out << "SDL_Init failed: " << SDL_GetError() << "\n";
                return false;
            }
            sdlInitialized_ = true;
        }
        if (renderer_ || window_) {
            resetSessionResources();
        }
        window_ = SDL_CreateWindow("Dragon MUGEN Verify", kWindowWidth, kWindowHeight, SDL_WINDOW_HIDDEN);
        if (!window_) {
            out << "SDL_CreateWindow failed: " << SDL_GetError() << "\n";
            return false;
        }
        renderer_ = SDL_CreateRenderer(window_, nullptr);
        if (!renderer_) {
            out << "SDL_CreateRenderer failed: " << SDL_GetError() << "\n";
            return false;
        }
        state_.mainSettings.canvasPreset = kStandardCanvasPreset;
        const CanvasDimensions defaultCanvas = dimensionsForPreset(kStandardCanvasPreset);
        SDL_SetRenderLogicalPresentation(renderer_, defaultCanvas.width, defaultCanvas.height, SDL_LOGICAL_PRESENTATION_LETTERBOX);
        SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
        return true;
    }
    bool loadVerificationContent(std::ostream& out) {
        state_.gameRoot = gameRoot_;
        state_.arenaConfig = loadArenaConfig(gameRoot_);
        loadProgressionState(state_);
        loadControlsState(state_);
        initAudio(state_);
        state_.fightRoundSettings = loadFightRoundSettings(gameRoot_);
        state_.selection.characters = loadCharacters(gameRoot_);
        state_.selection.stages = loadStages(gameRoot_);
        const auto compatibilitySelect = gameRoot_ / "data" / "compatibility_select.def";
        appendUniqueCharacters(state_.selection.characters, loadCharactersFromSelectFile(gameRoot_, compatibilitySelect));
        appendUniqueStages(state_.selection.stages, loadStagesFromSelectFile(gameRoot_, compatibilitySelect));
        if (state_.selection.characters.empty() || state_.selection.stages.empty()) {
            out << "runtime content missing characters or stages\n";
            return false;
        }
        return true;
    }

    static void appendUniqueCharacters(std::vector<CharacterSlot>& target, std::vector<CharacterSlot> source) {
        for (auto& character : source) {
            const auto duplicate = std::any_of(target.begin(), target.end(), [&](const CharacterSlot& existing) {
                return existing.id == character.id || existing.defPath == character.defPath;
            });
            if (!duplicate) {
                target.push_back(std::move(character));
            }
        }
    }

    static void appendUniqueStages(std::vector<StageSlot>& target, std::vector<StageSlot> source) {
        for (auto& stage : source) {
            const auto duplicate = std::any_of(target.begin(), target.end(), [&](const StageSlot& existing) {
                return existing.defPath == stage.defPath;
            });
            if (!duplicate) {
                target.push_back(std::move(stage));
            }
        }
    }

    void applyProjectedSnapshot(const StageSlot& stage, const FighterState& fighter, verification::FighterSnapshot& snapshot) const {
        const ArenaProjectedPoint projected = projectArenaWorldPoint(state_, stage, fighter.x, fighter.y, fighter.depthZ);
        snapshot.screenX = projected.screenX;
        snapshot.screenY = projected.screenY;
        snapshot.viewDepth = projected.viewZ;
        const FighterVisualScreenBounds visual = fighterVisualScreenBounds(state_, stage, fighter);
        if (visual.valid) {
            snapshot.visualScreenLeft = visual.left;
            snapshot.visualScreenRight = visual.right;
        } else {
            snapshot.visualScreenLeft = projected.screenX;
            snapshot.visualScreenRight = projected.screenX;
        }
    }

    void resetSessionResources() {
        closeAllGamepads(state_);
        destroyVisualAssets(state_);
        destroyAudioAssets(state_);
        if (renderer_) {
            SDL_DestroyRenderer(renderer_);
            renderer_ = nullptr;
        }
        if (window_) {
            SDL_DestroyWindow(window_);
            window_ = nullptr;
        }
        state_ = AppState{};
    }

    int findCharacterIndex(std::string_view hint) const {
        const std::string wanted = lowercaseCopy(hint);
        for (int i = 0; i < static_cast<int>(state_.selection.characters.size()); ++i) {
            const auto& character = state_.selection.characters[static_cast<size_t>(i)];
            const std::string defPath = lowercaseCopy(character.defPath.generic_string());
            if (lowercaseCopy(character.id) == wanted
                || lowercaseCopy(character.displayName) == wanted
                || lowercaseCopy(character.folder.filename().string()) == wanted
                || defPath == wanted
                || defPath.find(wanted) != std::string::npos) {
                return i;
            }
        }
        return 0;
    }

    int findStageIndex(std::string_view hint) const {
        const std::string wanted = lowercaseCopy(hint);
        for (int i = 0; i < static_cast<int>(state_.selection.stages.size()); ++i) {
            const auto& stage = state_.selection.stages[static_cast<size_t>(i)];
            if (lowercaseCopy(stage.id).find(wanted) != std::string::npos
                || lowercaseCopy(stage.displayName).find(wanted) != std::string::npos
                || lowercaseCopy(stage.defPath.filename().string()).find(wanted) != std::string::npos) {
                return i;
            }
        }
        return state_.selection.selectedStage;
    }

    std::filesystem::path gameRoot_;
    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
    bool sdlInitialized_ = false;
    AppState state_;
};

int runVerificationScenarioInternal(
    const std::filesystem::path& gameRoot,
    std::string_view scenarioName,
    std::ostream& out) {
    AppVerificationRuntime runtime(gameRoot);
    return verification::runNamedScenario(runtime, scenarioName, out);
}
