    bool setupVideoOptions(std::ostream& out) override {
        if (!prepareVerificationShell(out) || !loadVerificationContent(out)) {
            return false;
        }

        state_.frontend.screen = Screen::MainSettings;
        state_.frontend.screenFrame = 0;
        state_.mainSettings.optionsScreen = OptionsMenuScreen::Video;
        state_.mainSettings.selectedVideoOption = 0;
        loadVisualAssets(renderer_, state_);
        return true;
    }

    verification::PresentationFrameProbe videoOptionsPresentation(int outputProfileIndex) override {
        static constexpr std::array<CanvasPreset, 5> presets{
            CanvasPreset::Classic320x240,
            CanvasPreset::Wide426x240,
            CanvasPreset::Extra480x240,
            CanvasPreset::Sd854x480,
            CanvasPreset::Hd1280x720,
        };
        if (outputProfileIndex < 0 || outputProfileIndex >= static_cast<int>(presets.size())) {
            return {};
        }
        return captureVideoOptionsPresentationProbe(
            renderer_,
            state_,
            presets[static_cast<size_t>(outputProfileIndex)]);
    }

    std::vector<verification::ResolutionScreenProbe> resolutionScreenPresentation(int outputProfileIndex) override {
        static constexpr std::array<const char*, 5> profileNames{
            "classic_320x240",
            "wide_426x240",
            "extra_480x240",
            "sd_854x480",
            "hd_1280x720",
        };
        if (outputProfileIndex < 0 || outputProfileIndex >= static_cast<int>(profileNames.size())) {
            return {};
        }

        state_.frontend.screen = Screen::MainSettings;
        state_.frontend.screenFrame = 0;
        state_.mainSettings.optionsScreen = OptionsMenuScreen::Video;
        state_.mainSettings.selectedVideoOption = 0;
        state_.mainSettings.canvasPreset = kStandardCanvasPreset;
        for (int step = 0; step <= outputProfileIndex; ++step) {
            handleKey(renderer_, state_, SDLK_RIGHT);
        }

        const std::filesystem::path proofRoot = gameRoot_.parent_path()
            / "build" / "verification-proof" / "video-resolution" / profileNames[static_cast<size_t>(outputProfileIndex)];
        std::vector<verification::ResolutionScreenProbe> probes;
        const auto capture = [&](std::string_view name) {
            probes.push_back(captureResolutionScreenProbe(
                renderer_,
                state_,
                std::string(name),
                proofRoot / (std::string(name) + ".bmp")));
        };

        state_.frontend.screen = Screen::ModeSelect;
        capture("mode_select");

        struct OptionsCase {
            OptionsMenuScreen screen;
            const char* name;
        };
        constexpr std::array<OptionsCase, 9> optionsCases{ {
            { OptionsMenuScreen::Root, "options_root" },
            { OptionsMenuScreen::Gameplay, "options_gameplay" },
            { OptionsMenuScreen::Video, "options_video" },
            { OptionsMenuScreen::Controls, "options_controls" },
            { OptionsMenuScreen::PlayerControls, "options_player_controls" },
            { OptionsMenuScreen::KeyboardSetup, "options_keyboard_setup" },
            { OptionsMenuScreen::ControllerSetup, "options_controller_setup" },
            { OptionsMenuScreen::InputTest, "options_input_test" },
            { OptionsMenuScreen::RestoreDefaults, "options_restore_defaults" },
        } };
        state_.frontend.screen = Screen::MainSettings;
        for (const OptionsCase& item : optionsCases) {
            state_.mainSettings.optionsScreen = item.screen;
            setCurrentOptionsSelection(state_.mainSettings, 0);
            capture(item.name);
        }

        state_.frontend.pendingMode = PendingMode::SinglePlayer;
        state_.frontend.screen = Screen::CharacterSelect;
        state_.frontend.screenFrame = 0;
        capture("character_select");

        state_.frontend.pendingMode = PendingMode::Arena;
        setArenaDefaultsFromConfig(state_);
        state_.selection.sessionSlots.arenaCpuCount = 1;
        setArenaCpuCount(state_, 1);
        state_.selection.sessionSlots.opponentType = OpponentType::Cpu;
        selectArenaDefaultStage(state_);
        configureFightSessionSlotsFromSelection(state_);
        state_.frontend.screen = Screen::ArenaSetup;
        capture("arena_setup");

        state_.frontend.pendingMode = PendingMode::SinglePlayer;
        selectPreferredStage(state_);
        configureFightSessionSlotsFromSelection(state_);
        state_.frontend.screen = Screen::StageSelect;
        capture("stage_select");

        state_.frontend.pendingMode = PendingMode::Story;
        state_.selection.sessionSlots.opponentType = OpponentType::Cpu;
        selectStoryDefaultBoardNode(state_);
        configureFightSessionSlotsFromSelection(state_);
        state_.frontend.screen = Screen::StageSelect;
        capture("story_stage_select");

        state_.frontend.pendingMode = PendingMode::SingleFight;
        state_.selection.selectedP2Character = defaultP2CharacterIndex(
            state_.selection,
            state_.selection.selectedCharacter);
        state_.selection.sessionSlots.opponentType = OpponentType::Cpu;
        selectPreferredStage(state_);
        configureFightSessionSlotsFromSelection(state_);
        state_.frontend.screen = Screen::VersusScreen;
        state_.frontend.screenFrame = 0;
        state_.fightSessionPrepared = false;
        if (prepareFightSession(renderer_, state_)) {
            capture("versus_screen");
            beginFight(state_);
            capture("fight_view");

            state_.frontend.pendingMode = PendingMode::Training;
            capture("training_fight");
            state_.training.options.menuOpen = true;
            capture("training_options");
            state_.training.options.menuOpen = false;

            state_.frontend.fightPauseOpen = true;
            capture("fight_pause");
            state_.frontend.fightPauseOpen = false;

            state_.frontend.pendingMode = PendingMode::SingleFight;
            state_.matchPhase = MatchPhase::MatchResult;
            capture("fight_result");
            state_.matchPhase = MatchPhase::Fight;

            state_.frontend.pendingMode = PendingMode::Arena;
            capture("arena_fight");
            state_.frontend.pendingMode = PendingMode::Story;
            state_.story.activeWaveEnemyCount = 1;
            capture("story_fight");
        } else {
            verification::ResolutionScreenProbe versusProbe;
            versusProbe.name = "versus_screen";
            probes.push_back(std::move(versusProbe));
            verification::ResolutionScreenProbe fightProbe;
            fightProbe.name = "fight_view";
            probes.push_back(std::move(fightProbe));
            constexpr std::array<const char*, 6> unavailableFightViews{
                "training_fight",
                "training_options",
                "fight_pause",
                "fight_result",
                "arena_fight",
                "story_fight",
            };
            for (const char* name : unavailableFightViews) {
                verification::ResolutionScreenProbe unavailable;
                unavailable.name = name;
                probes.push_back(std::move(unavailable));
            }
        }

        enterShopDemo(renderer_, state_);
        capture("shop_demo");
        shopDemoBeginShopkeeperGreeting(state_);
        shopDemoUpdateCamera(state_, true);
        state_.shopDemo.layeredSceneV2Enabled = false;
        capture("shop_greeting_v1");
        state_.shopDemo.layeredSceneV2Enabled = true;
        capture("shop_greeting_v2");
        shopDemoOpenServicePanel(state_);
        shopDemoUpdateCamera(state_, true);
        state_.shopDemo.layeredSceneV2Enabled = false;
        capture("shop_overlay_v1");
        state_.shopDemo.layeredSceneV2Enabled = true;
        capture("shop_overlay_v2");
        return probes;
    }
