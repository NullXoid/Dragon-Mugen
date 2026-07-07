#pragma once

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
    const auto renderActiveScreen = [&]() {
        clearPhysicalFrame(renderer);
        applyLogicalPresentation(renderer, state);
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
            drawFightViewFrame(renderer, state, false);
            break;
        }
    };

    renderActiveScreen();
    renderActiveScreen();
    state.suppressFpsCounter = oldSuppressFps;

    SDL_Surface* surface = SDL_RenderReadPixels(renderer, nullptr);
    if (!surface) {
        return false;
    }
    const bool saved = SDL_SaveBMP(surface, path.string().c_str());
    SDL_DestroySurface(surface);
    return saved;
}
