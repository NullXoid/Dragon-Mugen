#pragma once

// Internal App.cpp implementation header.
// This file depends on App.cpp-local state controller runtime helpers.
// Include only from App.cpp after required types/helpers are defined.
// Do not include from other translation units.

float resolveStateVelocityControllerValue(
    const AppState& state,
    const FighterState& fighter,
    const StateVelocityController& velocity,
    bool xAxis,
    const FighterState* opponent,
    const StageSlot* stage) {
    const std::string& expression = xAxis ? velocity.xExpression : velocity.yExpression;
    if (!expression.empty()) {
        if (const auto value = evalMugenExpression(state, fighter, expression, opponent, stage)) {
            return *value;
        }
    }
    return xAxis ? velocity.x : velocity.y;
}

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
                fighter.vx = resolveStateVelocityControllerValue(state, fighter, velocity, true, opponent, stage)
                    * static_cast<float>(fighter.facing);
            }
            if (velocity.hasY) {
                fighter.vy = resolveStateVelocityControllerValue(state, fighter, velocity, false, opponent, stage);
            }
            break;
        case StateVelocityOperation::Add:
            if (velocity.hasX) {
                fighter.vx += resolveStateVelocityControllerValue(state, fighter, velocity, true, opponent, stage)
                    * static_cast<float>(fighter.facing);
            }
            if (velocity.hasY) {
                fighter.vy += resolveStateVelocityControllerValue(state, fighter, velocity, false, opponent, stage);
            }
            break;
        case StateVelocityOperation::Mul:
            if (velocity.hasX) {
                fighter.vx *= resolveStateVelocityControllerValue(state, fighter, velocity, true, opponent, stage);
            }
            if (velocity.hasY) {
                fighter.vy *= resolveStateVelocityControllerValue(state, fighter, velocity, false, opponent, stage);
            }
            break;
        }
    }
}
