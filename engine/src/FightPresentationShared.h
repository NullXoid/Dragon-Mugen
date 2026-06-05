#pragma once

// Internal App.cpp extraction file.
// This file depends on App.cpp anonymous-namespace types and render helpers.
// Include only from App.cpp after the required types/helpers are defined.
// Do not include from other translation units.

std::string singleFightStatusLine(const AppState& state) {
    if (state.matchPhase == MatchPhase::RoundStart) {
        return roundStartCalloutText(state);
    }
    if (state.matchPhase == MatchPhase::RoundFinish) {
        if (state.matchPhaseTicks >= state.fightRoundSettings.winTime) {
            return roundResultText(state);
        }
        return roundFinishCalloutText(state);
    }
    if (state.matchPhase == MatchPhase::RoundResult) {
        return roundResultText(state);
    }
    if (state.matchPhase == MatchPhase::MatchResult) {
        return "MATCH COMPLETE";
    }
    if (!state.gamepads.empty()) {
        return "Pads " + gamepadActionLayoutText(state, 0) + "  Start pause";
    }
    if (state.frontend.pendingMode == PendingMode::SinglePlayer) {
        return "P1 arrows A/S/D Z/X/C  CPU opponent";
    }
    return "P1 arrows A/S/D Z/X/C  P2 I/J/K/L U/O/P N/M/,";
}

void drawRoundWinPips(
    SDL_Renderer* renderer,
    float x,
    float y,
    int wins,
    int required,
    bool rightAligned = false,
    float size = 5.0f) {
    required = std::clamp(required, 1, 5);
    wins = std::clamp(wins, 0, required);
    const float gap = 3.0f;
    const float totalWidth = static_cast<float>(required) * size + static_cast<float>(required - 1) * gap;
    float startX = rightAligned ? x - totalWidth : x;
    for (int i = 0; i < required; ++i) {
        const float pipX = startX + static_cast<float>(i) * (size + gap);
        if (i < wins) {
            setColor(renderer, 230, 190, 105);
            fillRect(renderer, pipX, y, size, size);
            setColor(renderer, 42, 32, 12);
            drawRect(renderer, pipX, y, size, size);
        } else {
            setColor(renderer, 42, 48, 58, 210);
            fillRect(renderer, pipX, y, size, size);
            setColor(renderer, 118, 130, 148);
            drawRect(renderer, pipX, y, size, size);
        }
    }
}
