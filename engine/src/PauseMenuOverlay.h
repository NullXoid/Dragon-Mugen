#pragma once

// Internal App.cpp extraction file.
// This file depends on App.cpp anonymous-namespace types and render helpers.
// Include only from App.cpp after the required types/helpers are defined.
// Do not include from other translation units.

std::string_view singleFightPauseLabel(int option) {
    static constexpr std::array<std::string_view, kSingleFightPauseOptionCount> labels{
        "RESUME",
        "RESTART MATCH",
        "FIGHTER SELECT",
        "STAGE SELECT",
        "MODE SELECT",
    };
    return labels[static_cast<size_t>(std::clamp(option, 0, kSingleFightPauseOptionCount - 1))];
}

void drawSingleFightPauseMenu(SDL_Renderer* renderer, const AppState& state) {
    ScopedUiScale scaledUi(renderer, state, 320.0f, 240.0f);

    setColor(renderer, 6, 8, 14, 238);
    fillRect(renderer, 68, 42, 184, 156);
    setColor(renderer, 72, 82, 104);
    drawRect(renderer, 68, 42, 184, 156);
    setColor(renderer, 30, 38, 54);
    fillRect(renderer, 70, 44, 180, 28);
    setColor(renderer, 158, 64, 58);
    fillRect(renderer, 70, 72, 180, 2);

    setColor(renderer, 230, 220, 172);
    debugText(renderer, 84, 52, std::string(pendingModeTitle(state.frontend.pendingMode)));
    setColor(renderer, 128, 171, 225);
    debugText(renderer, 204, 52, "PAUSE");

    for (int i = 0; i < kSingleFightPauseOptionCount; ++i) {
        const float y = 88.0f + static_cast<float>(i * 18);
        if (i == state.frontend.selectedSingleFightPauseOption) {
            setColor(renderer, 74, 170, 134);
            fillRect(renderer, 84, y - 3.0f, 132, 14);
            setColor(renderer, 8, 12, 16);
        } else {
            setColor(renderer, 174, 184, 196);
        }
        debugText(renderer, 94, y, std::string(singleFightPauseLabel(i)));
    }

    setColor(renderer, 130, 142, 156);
    debugText(renderer, 86, 178, "ENTER select  ESC resume");
}
