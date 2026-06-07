#pragma once

// Internal App.cpp implementation header.
// This file depends on App.cpp-local state controller runtime helpers.
// Include only from App.cpp after required types/helpers are defined.
// Do not include from other translation units.

void updateStateVelocityControllersForDefinition(
    AppState& state,
    FighterState& fighter,
    const StateDefinition& stateDef,
    const FighterState* opponent,
    const StageSlot* stage) {
    for (const auto& velocity : stateDef.velocityControllers) {
        if (!shouldRunStateRuntimeController(state, fighter, velocity.id, velocity.trigger, opponent, stage)) {
            continue;
        }
        switch (velocity.operation) {
        case StateVelocityOperation::Set:
            if (velocity.hasX) {
                fighter.vx = velocity.x * static_cast<float>(fighter.facing);
            }
            if (velocity.hasY) {
                fighter.vy = velocity.y;
            }
            break;
        case StateVelocityOperation::Add:
            if (velocity.hasX) {
                fighter.vx += velocity.x * static_cast<float>(fighter.facing);
            }
            if (velocity.hasY) {
                fighter.vy += velocity.y;
            }
            break;
        case StateVelocityOperation::Mul:
            if (velocity.hasX) {
                fighter.vx *= velocity.x;
            }
            if (velocity.hasY) {
                fighter.vy *= velocity.y;
            }
            break;
        }
    }
}
