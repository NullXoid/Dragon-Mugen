#pragma once

void renderVerificationActiveScreen(SDL_Renderer* renderer, AppState& state) {
    beginPresentationFrame(renderer, state);
    switch (state.frontend.screen) {
    case Screen::ModeSelect:
        drawModeSelect(renderer, state);
        break;
    case Screen::MainSettings:
        drawMainSettings(renderer, state);
        break;
    case Screen::CharacterSelect:
        drawCharacterSelect(renderer, state);
        break;
    case Screen::ArenaSetup:
        drawArenaSetup(renderer, state);
        break;
    case Screen::StageSelect:
        drawStageSelect(renderer, state);
        break;
    case Screen::VersusScreen:
        drawVersusScreen(renderer, state);
        break;
    case Screen::ShopDemo:
        drawShopDemo(renderer, state);
        break;
    case Screen::FightView:
    default:
        drawFightViewFrame(renderer, state, true);
        break;
    }
}

bool captureVerificationScreenshot(SDL_Renderer* renderer, AppState& state, const std::filesystem::path& path) {
    if (!renderer) {
        return false;
    }

    std::error_code error;
    if (path.has_parent_path()) {
        std::filesystem::create_directories(path.parent_path(), error);
        if (error) {
            return false;
        }
    }

    const bool oldSuppressFps = state.suppressFpsCounter;
    state.suppressFpsCounter = true;
    renderVerificationActiveScreen(renderer, state);
    renderVerificationActiveScreen(renderer, state);
    state.suppressFpsCounter = oldSuppressFps;

    SDL_Surface* surface = SDL_RenderReadPixels(renderer, nullptr);
    if (!surface) {
        return false;
    }
    const bool saved = SDL_SaveBMP(surface, path.string().c_str());
    SDL_DestroySurface(surface);
    return saved;
}

verification::PresentationFrameProbe captureVideoOptionsPresentationProbe(
    SDL_Renderer* renderer,
    AppState& state,
    CanvasPreset preset) {
    verification::PresentationFrameProbe out;
    if (!renderer) {
        return out;
    }

    const Screen oldScreen = state.frontend.screen;
    const int oldScreenFrame = state.frontend.screenFrame;
    const CanvasPreset oldPreset = state.mainSettings.canvasPreset;
    const OptionsMenuScreen oldOptionsScreen = state.mainSettings.optionsScreen;
    const int oldSelectedVideoOption = state.mainSettings.selectedVideoOption;
    const bool oldSuppressFps = state.suppressFpsCounter;

    state.frontend.screen = Screen::MainSettings;
    state.frontend.screenFrame = 0;
    state.mainSettings.canvasPreset = preset;
    state.mainSettings.optionsScreen = OptionsMenuScreen::Video;
    state.mainSettings.selectedVideoOption = 0;
    state.suppressFpsCounter = true;

    const CanvasDimensions selectedOutput = outputDimensionsForPreset(preset);
    out.selectedOutputWidth = selectedOutput.width;
    out.selectedOutputHeight = selectedOutput.height;

    renderVerificationActiveScreen(renderer, state);
    renderVerificationActiveScreen(renderer, state);

    out.renderTargetWidth = gPresentationFrameTargetWidth;
    out.renderTargetHeight = gPresentationFrameTargetHeight;
    SDL_GetCurrentRenderOutputSize(renderer, &out.physicalWidth, &out.physicalHeight);

    SDL_Surface* surface = SDL_RenderReadPixels(renderer, nullptr);
    if (surface) {
        out.readbackWidth = surface->w;
        out.readbackHeight = surface->h;
        const int bytesPerPixel = SDL_BYTESPERPIXEL(surface->format);
        if (surface->pixels && bytesPerPixel > 0) {
            constexpr std::uint64_t fnvOffset = 1469598103934665603ULL;
            constexpr std::uint64_t fnvPrime = 1099511628211ULL;
            std::array<bool, 256> distinct{};
            std::uint64_t hash = fnvOffset;
            const int sampleWidth = std::min(surface->w, 900);
            const int sampleHeight = std::min(surface->h, 72);
            const auto* pixels = static_cast<const Uint8*>(surface->pixels);
            for (int y = 0; y < sampleHeight; ++y) {
                const Uint8* row = pixels + static_cast<size_t>(y) * static_cast<size_t>(surface->pitch);
                for (int x = 0; x < sampleWidth; ++x) {
                    const Uint8* pixel = row + static_cast<size_t>(x) * static_cast<size_t>(bytesPerPixel);
                    for (int byte = 0; byte < bytesPerPixel; ++byte) {
                        const Uint8 value = pixel[byte];
                        distinct[value] = true;
                        hash ^= value;
                        hash *= fnvPrime;
                    }
                }
            }
            out.staticUiHash = hash;
            out.sampledDistinctByteValues = static_cast<int>(std::count(distinct.begin(), distinct.end(), true));
            out.readbackOk = sampleWidth > 0 && sampleHeight > 0;
        }
        SDL_DestroySurface(surface);
    }

    state.frontend.screen = oldScreen;
    state.frontend.screenFrame = oldScreenFrame;
    state.mainSettings.canvasPreset = oldPreset;
    state.mainSettings.optionsScreen = oldOptionsScreen;
    state.mainSettings.selectedVideoOption = oldSelectedVideoOption;
    state.suppressFpsCounter = oldSuppressFps;
    return out;
}
