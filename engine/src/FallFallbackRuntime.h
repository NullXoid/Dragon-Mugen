#pragma once

// Internal App.cpp implementation header for common fall/down safety behavior.
// Include only from App.cpp after the common recovery helpers are defined.

bool fallFallbacksEnabled(const AppState& state) {
    return state.mainSettings.fallFallbacksEnabled;
}

int animationVariantMod(int action) {
    const int mod = action % 10;
    return mod < 0 ? mod + 10 : mod;
}

int variantActionOrBase(const AppState& state, const FighterState& fighter, int base, int sourceAction) {
    const int candidate = base + animationVariantMod(sourceAction);
    if (candidate != base && findExactClipForActor(state, fighter, candidate)) {
        return candidate;
    }
    return findExactClipForActor(state, fighter, base) ? base : 0;
}

int fallBounceActionForFighter(const AppState& state, const FighterState& fighter) {
    const int action = variantActionOrBase(state, fighter, 5160, fighter.action);
    return action != 0 ? action : fallLandActionForFighter(state, fighter);
}

int liedownImpactActionForFighter(const AppState& state, const FighterState& fighter) {
    const int action = variantActionOrBase(state, fighter, 5170, fighter.action);
    if (action != 0) {
        return action;
    }
    return firstExistingActionForActor(state, fighter, { 5110, fallLandActionForFighter(state, fighter), 0 });
}

int liedownRestActionForFighter(const AppState& state, const FighterState& fighter) {
    const int action = variantActionOrBase(state, fighter, 5110, fighter.action);
    return action != 0 ? action : firstExistingActionForActor(state, fighter, { 5110, fallLandActionForFighter(state, fighter), 0 });
}

int getUpActionForFighter(const AppState& state, const FighterState& fighter) {
    const int action = variantActionOrBase(state, fighter, 5120, fighter.action);
    return action != 0 ? action : (findExactClipForActor(state, fighter, 5120) ? 5120 : 0);
}

void enterGroundImpactState(const AppState& state, FighterState& fighter, int stateNo, int action) {
    fighter.prevStateNo = fighter.stateNo;
    fighter.stateNo = stateNo;
    fighter.stateTime = 0;
    fighter.stateType = 'L';
    fighter.moveType = 'H';
    fighter.physics = 'N';
    fighter.ctrl = false;
    fighter.onGround = true;
    fighter.y = 0.0f;
    fighter.vy = 0.0f;
    fighter.vx *= 0.75f;
    setFighterAction(fighter, action);
}

struct FallGroundImpactChoice {
    int stateNo = 0;
    int action = 0;
};

FallGroundImpactChoice fallGroundImpactForFighter(const AppState& state, const FighterState& fighter) {
    if (fighter.hitFallTrip || fighter.stateNo == 5070 || fighter.stateNo == 5071) {
        return { 5110, liedownImpactActionForFighter(state, fighter) };
    }
    if (fighter.stateNo == 5160 || fighter.stateNo == 5101) {
        return { 5110, liedownImpactActionForFighter(state, fighter) };
    }
    const int action = fallLandActionForFighter(state, fighter);
    return { action, action };
}

bool enterFallGroundImpactIfAvailable(AppState& state, FighterState& fighter) {
    const FallGroundImpactChoice impact = fallGroundImpactForFighter(state, fighter);
    if (impact.action == 0 || impact.stateNo == 0) {
        enterState(state, fighter, 0);
        return true;
    }
    enterGroundImpactState(state, fighter, impact.stateNo, impact.action);
    triggerFallEnvShakeIfNeeded(state, fighter);
    if (fighter.hitFallDamage > 0) {
        fighter.life = std::max(0, fighter.life - fighter.hitFallDamage);
        fighter.hitFallDamage = 0;
    }
    startFallGroundLiedownRecovery(state, fighter);
    return true;
}

bool resolveTripFallGrounding(AppState& state, FighterState& fighter) {
    if (!fallFallbacksEnabled(state) || !fighter.hitFall || !fighter.hitFallTrip || fighter.hitPauseTicks > 0) {
        return false;
    }
    if (fighter.stateNo != 5071) {
        return false;
    }
    if (fighter.vy <= 0.0f || fighter.y < 15.0f) {
        return false;
    }

    fighter.y = 0.0f;
    fighter.vy = 0.0f;
    fighter.onGround = true;
    return enterFallGroundImpactIfAvailable(state, fighter);
}

bool resolveArenaFallGrounding(AppState& state, FighterState& fighter) {
    if (!fallFallbacksEnabled(state) || !isArenaMode(state) || !fighter.hitFall || fighter.hitPauseTicks > 0) {
        return false;
    }

    clampArenaHitFallRuntime(state, fighter);
    const bool floorContact = fighter.onGround || fighter.y >= 0.0f;
    if (fighter.hitFallTrip
        && fighter.stateNo == 5071
        && fighter.stateTime >= 8
        && fighter.vy > 0.0f
        && fighter.y >= 15.0f) {
        fighter.y = 0.0f;
        fighter.vy = 0.0f;
        fighter.onGround = true;
        return enterFallGroundImpactIfAvailable(state, fighter);
    }
    if (fighter.stateNo == 5101
        && fighter.vy > 0.0f
        && fighter.y >= characterConstantsForActor(state, fighter).movementDownBounceGroundLevel) {
        fighter.y = 0.0f;
        fighter.vy = 0.0f;
        fighter.onGround = true;
        return enterFallGroundImpactIfAvailable(state, fighter);
    }
    if ((fighter.stateNo == 5050 || fighter.stateNo == 5160) && floorContact) {
        fighter.y = 0.0f;
        fighter.vy = 0.0f;
        fighter.onGround = true;
        return enterFallGroundImpactIfAvailable(state, fighter);
    }
    if (fighter.stateNo == 5050 && fighter.stateTime >= 18 && fighter.vy > 0.0f && fighter.y >= -8.0f) {
        fighter.y = 0.0f;
        fighter.vy = 0.0f;
        fighter.onGround = true;
        return enterFallGroundImpactIfAvailable(state, fighter);
    }
    return false;
}
