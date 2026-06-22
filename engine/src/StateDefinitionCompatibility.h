#pragma once

// Internal App.cpp state-definition compatibility helpers.
// Depends on App.cpp-local AppState, FighterState, StateDefinition, expression,
// clip, and compatibility helpers. Include only after those declarations exist.

const FighterState* opponentForActor(const AppState& state, const FighterState& actor);

std::optional<float> evalMugenExpression(
    const AppState& state,
    const FighterState& fighter,
    const std::string& expression,
    const FighterState* opponent,
    const StageSlot* stage);

int resolveStateDefinitionAnimAction(const AppState& state, const FighterState& fighter, int requestedAction) {
    const auto resolved = resolveCompatibleStateAnimAction(
        compatibilityContextForActor(state, fighter),
        requestedAction,
        [&state, &fighter](int action) {
            return findExactClipForActor(state, fighter, action) != nullptr;
        });
    return resolved ? *resolved : -1;
}

int resolveStateDefinitionAnimAction(const AppState& state, const FighterState& fighter, const StateDefinition& stateDef) {
    int requestedAction = stateDef.anim;
    if (!trim(stateDef.animExpression).empty()) {
        if (const auto value = evalMugenExpression(state, fighter, stateDef.animExpression, opponentForActor(state, fighter), nullptr)) {
            requestedAction = static_cast<int>(std::lround(*value));
        }
    }
    return resolveStateDefinitionAnimAction(state, fighter, requestedAction);
}
