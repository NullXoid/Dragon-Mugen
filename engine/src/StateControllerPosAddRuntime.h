#pragma once

// Internal App.cpp implementation header.
// This file depends on App.cpp-local state controller runtime helpers.
// Include only from App.cpp after required types/helpers are defined.
// Do not include from other translation units.

void updateStatePosAddControllersForDefinition(
    AppState& state,
    FighterState& fighter,
    const StateDefinition& stateDef,
    const FighterState* opponent,
    const StageSlot* stage) {
    for (const auto& posAdd : stateDef.posAdds) {
        if (posAdd.trigger.hasTrigger) {
            if (!shouldRunStateRuntimeController(state, fighter, posAdd.id, posAdd.trigger, opponent, stage)) {
                continue;
            }
            fighter.x += posAdd.x * static_cast<float>(fighter.facing);
            fighter.y += posAdd.y;
            continue;
        }
        if (statePosAddAlreadyFired(fighter, posAdd.id)
            || !simpleControllerTriggerActive(state, fighter, posAdd.triggerTime, posAdd.triggerAnimElem)) {
            continue;
        }
        markStatePosAddFired(fighter, posAdd.id);
        fighter.x += posAdd.x * static_cast<float>(fighter.facing);
        fighter.y += posAdd.y;
    }
}
