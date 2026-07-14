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
