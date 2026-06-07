#pragma once

// Internal App.cpp implementation header.
// This file depends on App.cpp-local state controller runtime helpers.
// Include only from App.cpp after required types/helpers are defined.
// Do not include from other translation units.

void updateStateSprPriorityControllersForDefinition(
    AppState& state,
    FighterState& fighter,
    const StateDefinition& stateDef,
    const FighterState* opponent,
    const StageSlot* stage) {
    for (const auto& sprPriority : stateDef.sprPriorities) {
        if (!shouldRunStateRuntimeController(state, fighter, sprPriority.id, sprPriority.trigger, opponent, stage)) {
            continue;
        }
        fighter.sprPriority = sprPriority.value;
    }
}
