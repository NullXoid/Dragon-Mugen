#pragma once

// Internal App.cpp extraction file.
// This file depends on App.cpp anonymous-namespace types and render helpers.
// Include only from App.cpp after the required types/helpers are defined.
// Do not include from other translation units.

void drawModeSelect(SDL_Renderer* renderer, const AppState& state) {
    static constexpr std::array<std::string_view, 5> modes{ {
        "TRAINING",
        "SINGLE PLAYER",
        "VS MODE",
        "OPTIONS",
        "EXIT",
    } };

    drawTitleBackground(renderer, state);

    const float centerX = screenCenterX(state);
    const float menuX = centerX - 64.0f;
    setColor(renderer, 230, 230, 238);
    debugTextCentered(renderer, centerX, 16, "DRAGON MUGEN CORE");
    drawSpriteAtAxis(renderer, state.systemScreens.titleLogo, centerX, 40);

    setColor(renderer, 10, 10, 12);
    fillRect(renderer, menuX, 138, 128, 70);
    setColor(renderer, 96, 106, 122);
    drawRect(renderer, menuX, 138, 128, 70);

    for (int i = 0; i < static_cast<int>(modes.size()); ++i) {
        const float y = 150.0f + static_cast<float>(i * 13);
        const std::string label(modes[static_cast<size_t>(i)]);
        const float textX = centerX - static_cast<float>(label.size()) * 4.0f;

        if (i == state.frontend.selectedMode) {
            setColor(renderer, 226, 210, 78);
            drawRect(renderer, menuX + 8.0f, y - 9, 112, 13);
            setColor(renderer, 255, 238, 96);
            debugText(renderer, textX, y - 6, label);
        } else {
            setColor(renderer, 220, 224, 232);
            debugText(renderer, textX, y - 6, label);
        }
    }

    setColor(renderer, 220, 224, 232);
    switch (state.frontend.selectedMode) {
    case 0:
        debugTextCentered(renderer, centerX, 214, "TRAINING SANDBOX");
        break;
    case 1:
        debugTextCentered(renderer, centerX, 214, "ARCADE STYLE MATCH");
        break;
    case 2:
        debugTextCentered(renderer, centerX, 214, "LOCAL COUCH MATCH");
        break;
    case 3:
        debugTextCentered(renderer, centerX, 214, "OPTIONS");
        break;
    case 4:
        debugTextCentered(renderer, centerX, 214, "EXIT");
        break;
    default:
        break;
    }

    setColor(renderer, 150, 156, 166);
    debugTextCentered(renderer, centerX, 226, "UP/DOWN  ENTER  ESC");

    SDL_RenderPresent(renderer);
}
