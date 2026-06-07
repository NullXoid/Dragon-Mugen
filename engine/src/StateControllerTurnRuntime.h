#pragma once

// Internal App.cpp implementation header.
// This file depends on App.cpp-local state controller runtime helpers.
// Include only from App.cpp after required types/helpers are defined.
// Do not include from other translation units.

void updateStateTurnControllersForDefinition(
    AppState& state,
    FighterState& fighter,
    const StateDefinition& stateDef,
    const FighterState* opponent,
    const StageSlot* stage) {
    for (const auto& turn : stateDef.turns) {
        if (!shouldRunStateRuntimeController(state, fighter, turn.id, turn.trigger, opponent, stage)) {
            continue;
        }
        fighter.facing *= -1;
        fighter.jumpBaseAction = 0;
        fighter.jumpPeakActionApplied = false;
    }
}
