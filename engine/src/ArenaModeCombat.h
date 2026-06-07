#pragma once

// Internal App.cpp implementation header for Arena DLC combat targeting.
// Include only from App.cpp after applyHitBetween is available.

int nearestLivingEnemyIndex(const AppState& state, int ownerIndex) {
    if (ownerIndex < 0 || ownerIndex >= static_cast<int>(state.fighters.size())) {
        return -1;
    }
    const FighterState& owner = state.fighters[static_cast<size_t>(ownerIndex)];
    int nearest = -1;
    float nearestDistance = 0.0f;
    for (size_t i = 0; i < state.fighters.size(); ++i) {
        if (static_cast<int>(i) == ownerIndex || state.fighters[i].life <= 0) {
            continue;
        }
        const float distance = std::fabs(state.fighters[i].x - owner.x);
        if (nearest < 0 || distance < nearestDistance) {
            nearest = static_cast<int>(i);
            nearestDistance = distance;
        }
    }
    return nearest;
}

void applyArenaHitIfNeeded(AppState& state) {
    for (size_t attacker = 0; attacker < state.fighters.size(); ++attacker) {
        if (state.fighters[attacker].life <= 0) {
            continue;
        }
        for (size_t defender = 0; defender < state.fighters.size(); ++defender) {
            if (attacker == defender || state.fighters[defender].life <= 0) {
                continue;
            }
            applyHitBetween(state, attacker, defender);
        }
    }

    const size_t helperBase = state.fighters.size();
    for (size_t i = 0; i < state.helpers.size(); ++i) {
        const auto& helper = state.helpers[i];
        if (helper.destroyRequested || helper.ownerIndex < 0 || helper.ownerIndex >= static_cast<int>(state.fighters.size())) {
            continue;
        }
        const int defender = nearestLivingEnemyIndex(state, helper.ownerIndex);
        if (defender >= 0) {
            applyHitBetween(state, helperBase + i, static_cast<size_t>(defender));
        }
    }
}
