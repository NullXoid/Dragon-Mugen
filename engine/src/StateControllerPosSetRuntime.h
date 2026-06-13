#pragma once

// Internal App.cpp implementation header.
// This file depends on App.cpp-local state controller runtime helpers.
// Include only from App.cpp after required types/helpers are defined.
// Do not include from other translation units.

void updateStatePosSetControllersForDefinition(
    AppState& state,
    FighterState& fighter,
    const StateDefinition& stateDef,
    const FighterState* opponent,
    const StageSlot* stage) {
    for (const auto& posSet : stateDef.posSets) {
        if (!shouldRunStateRuntimeController(state, fighter, posSet.id, posSet.trigger, opponent, stage)) {
            continue;
        }
        if (posSet.hasX) {
            fighter.x = posSet.x;
        }
        if (posSet.hasY) {
            fighter.y = posSet.y;
            fighter.onGround = fighter.stateType != 'A' && fighter.y >= 0.0f;
        }
    }
}
