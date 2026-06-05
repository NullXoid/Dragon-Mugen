#pragma once

// Internal App.cpp extraction file.
// This file depends on App.cpp anonymous-namespace types and render helpers.
// Include only from App.cpp after the required types/helpers are defined.
// Do not include from other translation units.

std::string_view mainSettingLabel(int option) {
    static constexpr std::array<std::string_view, kMainSettingsCount> labels{
        "MATCH TIMER",
        "CANVAS SIZE",
        "UI SCALE",
        "PAD LABELS",
        "P1 GAMEPAD",
        "P2 GAMEPAD",
        "BACK",
    };
    return labels[static_cast<size_t>(std::clamp(option, 0, kMainSettingsCount - 1))];
}

std::string mainSettingStatus(const AppState& state, int option) {
    switch (option) {
    case 0:
        return matchTimerSettingText(state.mainSettings);
    case 1:
        return canvasSizeSettingText(state.mainSettings);
    case 2:
        return uiScaleSettingText(state.mainSettings);
    case 3:
        return gamepadPromptStyleText(state.mainSettings.gamepadPromptStyle);
    case 4:
        return gamepadAssignmentText(state, 0);
    case 5:
        return gamepadAssignmentText(state, 1);
    default:
        return "";
    }
}

void drawMainSettings(SDL_Renderer* renderer, const AppState& state) {
    drawTitleBackground(renderer, state);

    ScopedUiScale scaledUi(renderer, state, 320.0f, 240.0f);

    constexpr float centerX = 160.0f;
    constexpr float menuTop = 76.0f;
    constexpr float rowSpacing = 13.0f;
    constexpr float labelX = 62.0f;
    constexpr float valueX = 204.0f;

    setColor(renderer, 238, 238, 244);
    debugTextCentered(renderer, centerX, 28, "DRAGON MUGEN CORE");
    setColor(renderer, 246, 214, 92);
    debugTextCentered(renderer, centerX, 46, "OPTIONS");

    setColor(renderer, 0, 0, 0, 178);
    fillRect(renderer, 36, menuTop - 14.0f, 248, 112);
    setColor(renderer, 78, 88, 108);
    drawRect(renderer, 36, menuTop - 14.0f, 248, 112);

    for (int i = 0; i < kMainSettingsCount; ++i) {
        const float y = menuTop + static_cast<float>(i) * rowSpacing;
        if (i == state.mainSettings.selectedOption) {
            setColor(renderer, 238, 222, 92);
            drawRect(renderer, 48, y - 9.0f, 224, 13);
            setColor(renderer, 246, 214, 92);
        } else {
            setColor(renderer, 222, 226, 232);
        }

        debugText(renderer, labelX, y - 6.0f, std::string(mainSettingLabel(i)));
        if (i < kMainSettingsCount - 1) {
            const std::string status = compactSettingText(mainSettingStatus(state, i), 16);
            const bool selected = i == state.mainSettings.selectedOption;
            setColor(renderer, selected ? 246 : 172, selected ? 214 : 182, selected ? 92 : 194);
            debugText(renderer, valueX - 12.0f, y - 6.0f, "<");
            debugText(renderer, valueX, y - 6.0f, status);
            debugText(renderer, valueX + 8.0f + static_cast<float>(status.size()) * 8.0f, y - 6.0f, ">");
        }
    }

    setColor(renderer, 0, 0, 0, 170);
    fillRect(renderer, 0, 204, 320, 24);

    setColor(renderer, 172, 182, 194);
    const std::string padText = state.gamepads.empty()
        ? "PADS NONE"
        : "PADS " + std::to_string(state.gamepads.size()) + "  " + compactSettingText(state.gamepads.front().name, 18);
    debugTextCentered(renderer, centerX, 208, padText);
    debugTextCentered(renderer, centerX, 220, "UP/DOWN   LEFT/RIGHT   ENTER   ESC");
    SDL_RenderPresent(renderer);
}
