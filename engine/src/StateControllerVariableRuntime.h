#pragma once

// Internal App.cpp implementation header.
// This file depends on App.cpp-local state controller runtime helpers.
// Include only from App.cpp after required types/helpers are defined.
// Do not include from other translation units.

void updateStateVariableControllers(
    AppState& state,
    FighterState& fighter,
    const FighterState* opponent = nullptr,
    const StageSlot* stage = nullptr) {
    forEachRuntimeControllerStateDefinition(state, fighter, [&](const StateDefinition& stateDef) {
        for (const auto& variable : stateDef.variableControllers) {
            if (!shouldRunStateRuntimeController(state, fighter, variable.id, variable.trigger, opponent, stage)) {
                continue;
            }

            if (variable.operation == StateVariableOperation::Random) {
                const auto minValue = evalMugenExpression(state, fighter, variable.rangeMinExpression, opponent, stage);
                const auto maxValue = evalMugenExpression(state, fighter, variable.rangeMaxExpression, opponent, stage);
                if (!minValue || !maxValue) {
                    continue;
                }
                int low = static_cast<int>(std::lround(*minValue));
                int high = static_cast<int>(std::lround(*maxValue));
                if (high < low) {
                    std::swap(low, high);
                }
                const int span = std::max(1, high - low + 1);
                const int value = low + (pseudoMugenRandom(state, fighter, variable.id) % span);
                setFighterVariableValue(fighter, variable.target, static_cast<float>(value));
                continue;
            }

            const auto value = evalMugenExpression(state, fighter, variable.valueExpression, opponent, stage);
            if (!value) {
                continue;
            }

            if (variable.operation == StateVariableOperation::Add) {
                setFighterVariableValue(fighter, variable.target, fighterVariableValue(fighter, variable.target) + *value);
            } else {
                setFighterVariableValue(fighter, variable.target, *value);
            }
        }

        for (const auto& rangeSet : stateDef.varRangeSets) {
            if (!shouldRunStateRuntimeController(state, fighter, rangeSet.id, rangeSet.trigger, opponent, stage)) {
                continue;
            }
            const auto value = evalMugenExpression(state, fighter, rangeSet.valueExpression, opponent, stage);
            if (!value) {
                continue;
            }
            const int maxIndex = rangeSet.floatBank ? 39 : 59;
            const int first = std::clamp(std::min(rangeSet.first, rangeSet.last), 0, maxIndex);
            const int last = std::clamp(std::max(rangeSet.first, rangeSet.last), 0, maxIndex);
            for (int index = first; index <= last; ++index) {
                if (rangeSet.floatBank) {
                    fighter.fvars[static_cast<size_t>(index)] = *value;
                } else {
                    fighter.vars[static_cast<size_t>(index)] = static_cast<int>(std::lround(*value));
                }
            }
        }
        return true;
    });
}
