#pragma once

class AppVerificationRuntime final : public verification::RuntimeProbe {
public:
    explicit AppVerificationRuntime(std::filesystem::path gameRoot)
        : gameRoot_(std::move(gameRoot)) {}

    ~AppVerificationRuntime() override {
        closeAllGamepads(state_);
        destroyVisualAssets(state_);
        destroyAudioAssets(state_);
        if (renderer_) {
            SDL_DestroyRenderer(renderer_);
        }
        if (window_) {
            SDL_DestroyWindow(window_);
        }
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
        if (!sdlInitialized_) {
            if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
                out << "SDL_Init failed: " << SDL_GetError() << "\n";
                return false;
            }
            sdlInitialized_ = true;
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
        SDL_SetRenderLogicalPresentation(renderer_, kLogicalWidth, kLogicalHeight, SDL_LOGICAL_PRESENTATION_LETTERBOX);
        SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);

        state_.gameRoot = gameRoot_;
        state_.arenaConfig = loadArenaConfig(gameRoot_);
        initAudio(state_);
        state_.fightRoundSettings = loadFightRoundSettings(gameRoot_);
        state_.selection.characters = loadCharacters(gameRoot_);
        state_.selection.stages = loadStages(gameRoot_);
        if (state_.selection.characters.empty() || state_.selection.stages.empty()) {
            out << "runtime content missing characters or stages\n";
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

    void step(const verification::SymbolicInput& p1Input, int frames) override {
        const FighterInputState p1 = toFighterInput(p1Input);
        const FighterInputState p2;
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
            applyTrainingPowerMode(state_);
            updateAudioMixer(state_);
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

    void setArenaZAxisEnabled(bool enabled) override {
        state_.selection.sessionSlots.arenaZAxisEnabled = enabled;
        updateArenaCameraRotation(state_);
    }

    void setArenaCameraRotationEnabled(bool enabled) override {
        state_.selection.sessionSlots.arenaCameraRotationEnabled = enabled;
        updateArenaCameraRotation(state_);
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

    std::vector<verification::TrainingMoveInfo> trainingMoves() const override {
        std::vector<verification::TrainingMoveInfo> moves;
        const auto entries = displayableMoveListEntries(state_);
        moves.reserve(entries.size());
        for (const auto* entry : entries) {
            if (!entry) {
                continue;
            }
            int targetState = -1;
            std::vector<std::string> commandNames = entry->requiredCommands;
            for (const auto& optionGroup : entry->commandOptionGroups) {
                for (const auto& command : optionGroup) {
                    if (!commandListContains(commandNames, command)) {
                        commandNames.push_back(command);
                    }
                }
            }
            if (const auto literalTarget = parsePlainIntValue(entry->targetStateExpression)) {
                targetState = *literalTarget;
            } else if (!state_.fighters.empty()) {
                std::vector<std::string> commands = entry->requiredCommands;
                for (const auto& optionGroup : entry->commandOptionGroups) {
                    if (!optionGroup.empty()) {
                        commands.push_back(optionGroup.front());
                    }
                }
                const FighterState& demoFighter = state_.fighters.size() > 1 ? state_.fighters[1] : state_.fighters[0];
                const FighterState* opponent = state_.fighters.size() > 1 ? &state_.fighters[0] : nullptr;
                if (const auto resolvedTarget = resolveCommandTargetState(state_, demoFighter, opponent, *entry, commands)) {
                    targetState = *resolvedTarget;
                }
            }
            moves.push_back(verification::TrainingMoveInfo{
                moveListEntryName(*entry),
                moveListInputText(*entry),
                targetState,
                entry->requiredStateType,
                commandEntryRequiredPower(*entry),
                std::move(commandNames),
            });
        }
        return moves;
    }

    bool selectTrainingMoveIndex(int index) override {
        const auto entries = displayableMoveListEntries(state_);
        if (index < 0 || index >= static_cast<int>(entries.size())) {
            return false;
        }
        state_.training.options.selectedMoveListEntry = index;
        state_.training.options.moveListScroll = std::max(0, index - 2);
        return true;
    }

    bool selectTrainingMove(std::string_view label) override {
        const std::string wanted = lowercaseCopy(label);
        const auto entries = displayableMoveListEntries(state_);
        for (int i = 0; i < static_cast<int>(entries.size()); ++i) {
            if (lowercaseCopy(moveListEntryName(*entries[static_cast<size_t>(i)])) != wanted) {
                continue;
            }
            state_.training.options.selectedMoveListEntry = i;
            state_.training.options.moveListScroll = std::max(0, i - 2);
            return true;
        }
        return false;
    }

    void startTrainingCommandDemo() override {
        beginTrainingCommandDemo(state_);
    }

    verification::RuntimeSnapshot snapshot() const override {
        verification::RuntimeSnapshot out;
        out.frame = state_.frame;
        out.cameraX = state_.cameraX;
        out.cameraY = state_.cameraY;
        out.arenaCameraYawDeg = state_.arenaCameraYawDeg;
        out.arenaCameraTargetYawDeg = state_.arenaCameraTargetYawDeg;
        out.matchTimerTicks = state_.matchTimerTicks;
        out.matchPhase = static_cast<int>(state_.matchPhase);
        out.activeEffects = static_cast<int>(state_.runtimeEffects.size());
        out.activeSounds = static_cast<int>(state_.audio.activeVoices.size());
        out.comboHits = state_.display.comboCounters[0].displayHits;
        out.fighterCount = static_cast<int>(state_.fighters.size());
        out.arenaRuntimeCount = static_cast<int>(state_.arenaRuntimes.size());
        out.globalPauseTicks = state_.globalPauseTicks;
        out.globalPauseOwnerMoveTicks = state_.globalPauseOwnerMoveTicks;
        out.globalPauseIsSuper = state_.globalPauseIsSuper;
        out.p1RuntimeStates = static_cast<int>(stateDefinitionsForActor(state_, state_.fighters[0]).size());
        out.p2RuntimeStates = static_cast<int>(stateDefinitionsForActor(state_, state_.fighters[1]).size());
        out.p1RuntimeHitDefs = static_cast<int>(hitDefinitionsForActor(state_, state_.fighters[0]).size());
        out.p2RuntimeHitDefs = static_cast<int>(hitDefinitionsForActor(state_, state_.fighters[1]).size());
        out.p1RuntimeCommandEntries = static_cast<int>(commandEntriesForActor(state_, state_.fighters[0]).size());
        out.p2RuntimeCommandEntries = static_cast<int>(commandEntriesForActor(state_, state_.fighters[1]).size());
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
        out.arenaZAxisEnabled = arenaZAxisEnabled(state_);
        out.arenaCameraRotationSelected = arenaCameraRotationSelected(state_);
        out.arenaCameraRotationActive = arenaCameraRotationActive(state_);
        out.lastHitText = state_.messages.lastHitText;
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
        out.p1 = fighterSnapshot(state_.fighters[0]);
        out.p2 = fighterSnapshot(state_.fighters[1]);
        out.p1AnimElem = currentAnimElemForFighter(state_, state_.fighters[0]);
        out.p2AnimElem = currentAnimElemForFighter(state_, state_.fighters[1]);
        out.p1P2BodyDistX = p2BodyDistXValue(state_, state_.fighters[0], state_.fighters[1]);
        if (const AnimationFrame* p1Frame = currentFrameForFighter(state_, state_.fighters[0])) {
            out.p1Clsn1Count = static_cast<int>(p1Frame->clsn1.size());
            if (const AnimationFrame* p2Frame = currentFrameForFighter(state_, state_.fighters[1])) {
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
        const StageSlot fallbackStage;
        const StageSlot& stage = selectedStageSlot(state_.selection) ? *selectedStageSlot(state_.selection) : fallbackStage;
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
    void applyProjectedSnapshot(const StageSlot& stage, const FighterState& fighter, verification::FighterSnapshot& snapshot) const {
        const ArenaProjectedPoint projected = projectArenaWorldPoint(state_, stage, fighter.x, fighter.y, fighter.depthZ);
        snapshot.screenX = projected.screenX;
        snapshot.screenY = projected.screenY;
        snapshot.viewDepth = projected.viewZ;
    }

    static FighterInputState toFighterInput(const verification::SymbolicInput& input) {
        return FighterInputState{
            input.left,
            input.right,
            input.up,
            input.down,
            input.s,
            input.x,
            input.y,
            input.z,
            input.a,
            input.b,
            input.c,
            input.depthModifier,
        };
    }

    static verification::FighterSnapshot fighterSnapshot(const FighterState& fighter) {
        return verification::FighterSnapshot{
            fighter.x,
            fighter.y,
            fighter.depthZ,
            fighter.vx,
            fighter.vy,
            fighter.depthVz,
            fighter.stateNo,
            fighter.action,
            fighter.stateTime,
            fighter.animTick,
            fighter.life,
            fighter.power,
            fighter.targetIndex,
            fighter.targetTicks,
            fighter.targetHitId,
            fighter.hitCount,
            fighter.hitPauseTicks,
            fighter.hitStunTicks,
            fighter.hitDownVelocityX,
            fighter.hitDownVelocityY,
            fighter.displayOffsetX,
            fighter.displayOffsetY,
            0.0f,
            0.0f,
            0.0f,
            fighter.stateType,
            fighter.moveType,
            fighter.physics,
            fighter.ctrl,
            fighter.onGround,
            fighter.moveContact,
            fighter.moveHit,
            fighter.moveGuarded,
        };
    }

    int findCharacterIndex(std::string_view hint) const {
        const std::string wanted = lowercaseCopy(hint);
        for (int i = 0; i < static_cast<int>(state_.selection.characters.size()); ++i) {
            const auto& character = state_.selection.characters[static_cast<size_t>(i)];
            if (lowercaseCopy(character.id) == wanted
                || lowercaseCopy(character.displayName) == wanted
                || lowercaseCopy(character.folder.filename().string()) == wanted) {
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
