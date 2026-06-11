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
        const int controllerKey = posAdd.trigger.hasTrigger ? posAdd.id : stateControllerDomainKey(3, posAdd.id);
        if (posAdd.trigger.hasTrigger) {
            if (!shouldRunStateRuntimeController(state, fighter, controllerKey, posAdd.trigger, opponent, stage)) {
                continue;
            }
            fighter.x += posAdd.x * static_cast<float>(fighter.facing);
            fighter.y += posAdd.y;
            continue;
        }
        if (!shouldRunSimpleStateRuntimeController(
            state,
            fighter,
            controllerKey,
            posAdd.trigger,
            posAdd.triggerTime,
            posAdd.triggerAnimElem)) {
            continue;
        }
        fighter.x += posAdd.x * static_cast<float>(fighter.facing);
        fighter.y += posAdd.y;
    }
}
