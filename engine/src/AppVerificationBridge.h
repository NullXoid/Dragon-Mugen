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

    bool setup(std::string_view p1Id, std::string_view stageHint, verification::ScenarioMode mode, std::ostream& out) override {
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
        initAudio(state_);
        state_.fightRoundSettings = loadFightRoundSettings(gameRoot_);
        state_.selection.characters = loadCharacters(gameRoot_);
        state_.selection.stages = loadStages(gameRoot_);
        if (state_.selection.characters.empty() || state_.selection.stages.empty()) {
            out << "runtime content missing characters or stages\n";
            return false;
        }

        state_.selection.selectedCharacter = findCharacterIndex(p1Id);
        state_.frontend.pendingMode = mode == verification::ScenarioMode::SinglePlayer
            ? PendingMode::SinglePlayer
            : PendingMode::Training;
        configureFightSessionSlotsFromSelection(state_);
        selectPreferredStage(state_);
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
        for (int i = 0; i < frames; ++i) {
            ++state_.frame;
            ++state_.frontend.screenFrame;
            FightInputOverride inputOverride;
            inputOverride.p1 = &p1;
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
        state_.fighters[0].facing = state_.fighters[0].x <= state_.fighters[1].x ? 1 : -1;
        state_.fighters[1].facing = -state_.fighters[0].facing;
    }

    verification::RuntimeSnapshot snapshot() const override {
        verification::RuntimeSnapshot out;
        out.frame = state_.frame;
        out.matchTimerTicks = state_.matchTimerTicks;
        out.matchPhase = static_cast<int>(state_.matchPhase);
        out.activeEffects = static_cast<int>(state_.runtimeEffects.size());
        out.activeSounds = static_cast<int>(state_.audio.activeVoices.size());
        out.comboHits = state_.display.comboCounters[0].displayHits;
        out.lastHitText = state_.messages.lastHitText;
        out.p1 = fighterSnapshot(state_.fighters[0]);
        out.p2 = fighterSnapshot(state_.fighters[1]);
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
        };
    }

    static verification::FighterSnapshot fighterSnapshot(const FighterState& fighter) {
        return verification::FighterSnapshot{
            fighter.x,
            fighter.y,
            fighter.vx,
            fighter.vy,
            fighter.stateNo,
            fighter.action,
            fighter.stateTime,
            fighter.animTick,
            fighter.life,
            fighter.power,
            fighter.hitCount,
            fighter.hitPauseTicks,
            fighter.stateType,
            fighter.moveType,
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
