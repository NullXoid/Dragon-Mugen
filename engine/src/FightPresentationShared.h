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
