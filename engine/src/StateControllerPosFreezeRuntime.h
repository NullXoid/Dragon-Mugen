#pragma once

// Internal App.cpp implementation header.
// This file depends on App.cpp-local state controller runtime helpers.
// Include only from App.cpp after required types/helpers are defined.
// Do not include from other translation units.

void updateStatePosFreezeControllersForDefinition(
    AppState& state,
    FighterState& fighter,
    const StateDefinition& stateDef,
    const FighterState* opponent,
    const StageSlot* stage) {
    for (const auto& posFreeze : stateDef.posFreezes) {
        if (!shouldRunStateRuntimeController(state, fighter, posFreeze.id, posFreeze.trigger, opponent, stage)) {
            continue;
        }
        fighter.posFreezeX = fighter.posFreezeX || posFreeze.freezeX;
        fighter.posFreezeY = fighter.posFreezeY || posFreeze.freezeY;
    }
}
