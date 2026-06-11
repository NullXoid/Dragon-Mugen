#pragma once

// Internal App.cpp implementation header.
// This file depends on App.cpp-local state controller runtime helpers.
// Include only from App.cpp after required types/helpers are defined.
// Do not include from other translation units.

void updateStatePowerControllersForDefinition(
    AppState& state,
    FighterState& fighter,
    const StateDefinition& stateDef,
    const FighterState* opponent,
    const StageSlot* stage) {
    for (const auto& powerAdd : stateDef.powerAdds) {
        if (!shouldRunStateRuntimeController(state, fighter, powerAdd.id, powerAdd.trigger, opponent, stage)) {
            continue;
        }
        const auto value = evalMugenExpression(state, fighter, powerAdd.valueExpression, opponent, stage);
        if (!value) {
            continue;
        }
        fighter.power = std::clamp(
            fighter.power + static_cast<int>(std::lround(*value)),
            0,
            std::max(0, characterConstantsForActor(state, fighter).maxPower));
    }
}
