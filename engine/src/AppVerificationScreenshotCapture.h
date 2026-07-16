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

verification::ResolutionScreenProbe captureResolutionScreenProbe(
    SDL_Renderer* renderer,
    AppState& state,
    std::string name,
    const std::filesystem::path& proofPath) {
    verification::ResolutionScreenProbe out;
    out.name = std::move(name);
    out.proofPath = proofPath;
    if (!renderer) {
        return out;
    }

    state.suppressFpsCounter = true;
    renderVerificationActiveScreen(renderer, state);
    renderVerificationActiveScreen(renderer, state);

    SDL_Surface* surface = SDL_RenderReadPixels(renderer, nullptr);
    if (!surface) {
        return out;
    }

    out.readbackWidth = surface->w;
    out.readbackHeight = surface->h;
    const int bytesPerPixel = SDL_BYTESPERPIXEL(surface->format);
    if (surface->pixels && bytesPerPixel > 0 && surface->w > 0 && surface->h > 0) {
        constexpr std::uint64_t fnvOffset = 1469598103934665603ULL;
        constexpr std::uint64_t fnvPrime = 1099511628211ULL;
        std::array<bool, 256> distinct{};
        std::uint64_t hash = fnvOffset;
        const auto* pixels = static_cast<const Uint8*>(surface->pixels);
        for (int y = 0; y < surface->h; ++y) {
            const Uint8* row = pixels + static_cast<size_t>(y) * static_cast<size_t>(surface->pitch);
            for (int x = 0; x < surface->w; ++x) {
                const Uint8* pixel = row + static_cast<size_t>(x) * static_cast<size_t>(bytesPerPixel);
                for (int byte = 0; byte < bytesPerPixel; ++byte) {
                    const Uint8 value = pixel[byte];
                    distinct[value] = true;
                    hash ^= value;
                    hash *= fnvPrime;
                }
            }
        }
        out.frameHash = hash;
        out.distinctByteValues = static_cast<int>(std::count(distinct.begin(), distinct.end(), true));
        out.readbackOk = true;
    }

    std::error_code error;
    std::filesystem::create_directories(proofPath.parent_path(), error);
    if (!error) {
        out.proofSaved = SDL_SaveBMP(surface, proofPath.string().c_str());
    }
    SDL_DestroySurface(surface);
    return out;
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

    std::vector<OptionsMenuRowView> menuRows;
    const OptionsMenuView menuView = buildControlsOptionsView(controlsOptionsContext(state), menuRows);
    out.menuTitle = menuView.title;
    out.menuFooter = menuView.footer;
    out.menuRows.reserve(menuRows.size());
    for (const auto& row : menuRows) {
        out.menuRows.push_back(verification::PresentationMenuRowProbe{
            row.label,
            row.value,
            row.selected,
            row.adjustable,
            row.disabled,
        });
    }
    const UiMenuListGeometrySnapshot menuGeometry = optionsMenuGeometrySnapshot(
        uiRenderContext(renderer, state),
        menuView);

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
            const auto* pixels = static_cast<const Uint8*>(surface->pixels);
            const auto hashRegion = [&](SDL_Rect region, const SDL_Rect* excluded, std::uint64_t& hash, int& distinctCount) {
                region.x = std::clamp(region.x, 0, surface->w);
                region.y = std::clamp(region.y, 0, surface->h);
                region.w = std::clamp(region.w, 0, surface->w - region.x);
                region.h = std::clamp(region.h, 0, surface->h - region.y);
                std::array<bool, 256> distinct{};
                hash = fnvOffset;
                int pixelCount = 0;
                for (int y = region.y; y < region.y + region.h; ++y) {
                    const Uint8* row = pixels + static_cast<size_t>(y) * static_cast<size_t>(surface->pitch);
                    for (int x = region.x; x < region.x + region.w; ++x) {
                        if (excluded
                            && x >= excluded->x && x < excluded->x + excluded->w
                            && y >= excluded->y && y < excluded->y + excluded->h) {
                            continue;
                        }
                        const Uint8* pixel = row + static_cast<size_t>(x) * static_cast<size_t>(bytesPerPixel);
                        for (int byte = 0; byte < bytesPerPixel; ++byte) {
                            const Uint8 value = pixel[byte];
                            distinct[value] = true;
                            hash ^= value;
                            hash *= fnvPrime;
                        }
                        ++pixelCount;
                    }
                }
                distinctCount = static_cast<int>(std::count(distinct.begin(), distinct.end(), true));
                return pixelCount;
            };

            const int staticPixels = hashRegion(
                SDL_Rect{ 0, 0, std::min(surface->w, 900), std::min(surface->h, 72) },
                nullptr,
                out.staticUiHash,
                out.sampledDistinctByteValues);

            int menuPixels = 0;
            if (menuGeometry.valid && out.renderTargetWidth > 0 && out.renderTargetHeight > 0) {
                const float scaleX = static_cast<float>(surface->w) / static_cast<float>(out.renderTargetWidth);
                const float scaleY = static_cast<float>(surface->h) / static_cast<float>(out.renderTargetHeight);
                const SDL_Rect panelRect{
                    static_cast<int>(std::floor(menuGeometry.panel.x * scaleX)),
                    static_cast<int>(std::floor(menuGeometry.panel.y * scaleY)),
                    static_cast<int>(std::ceil(menuGeometry.panel.w * scaleX)),
                    static_cast<int>(std::ceil(menuGeometry.panel.h * scaleY)),
                };
                const int maskLeft = static_cast<int>(std::floor(menuGeometry.firstValueCell.x * scaleX)) - 1;
                const int maskTop = static_cast<int>(std::floor(menuGeometry.firstValueCell.y * scaleY)) - 1;
                const SDL_Rect resolutionValueMask{
                    maskLeft,
                    maskTop,
                    static_cast<int>(std::ceil(
                        (menuGeometry.firstValueCell.x + menuGeometry.firstValueCell.w) * scaleX)) - maskLeft + 1,
                    static_cast<int>(std::ceil(
                        (menuGeometry.firstValueCell.y + menuGeometry.firstValueCell.h) * scaleY)) - maskTop + 1,
                };
                menuPixels = hashRegion(
                    panelRect,
                    &resolutionValueMask,
                    out.menuBodyHash,
                    out.menuBodyDistinctByteValues);
            }
            out.readbackOk = staticPixels > 0 && menuPixels > 0;
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
