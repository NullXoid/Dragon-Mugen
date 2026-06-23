#pragma once

// Internal App.cpp implementation shard.
// Hit, guard, fall, scaling, and fight-session compatibility runtime.

bool enterState(const AppState& state, FighterState& fighter, int stateNo) {
    fighter.guarding = false;
    if (stateNo == 0) {
        fighter.prevStateNo = fighter.stateNo;
        fighter.stateNo = stateNo;
        fighter.stateTime = 0;
        fighter.appliedHitDefIds.clear();
        clearStateRuntimeControllerTracking(fighter);
        fighter.attackDistanceOverride = -1;
        fighter.drawAngle = 0.0f;
        fighter.angleDrawActive = false;
        fighter.displayOffsetX = 0.0f;
        fighter.displayOffsetY = 0.0f;
        fighter.actionClipOwnerIndex = -1;
        fighter.moveContact = false;
        fighter.moveHit = false;
        fighter.moveGuarded = false;
        fighter.hitCount = 0;
        fighter.customHitState = false;
        fighter.ctrl = true;
        fighter.moveType = 'I';
        fighter.physics = fighter.onGround ? 'S' : 'A';
        fighter.stateType = fighter.onGround ? 'S' : 'A';
        fighter.sprPriority = 0;
        setFighterAction(fighter, chooseMovementAction(state, fighter));
        return true;
    }

    const StateDefinition* stateDef = findStateDefinitionForActor(state, fighter, stateNo);
    const int resolvedStateAnim = stateDef && stateDef->hasAnim
        ? resolveStateDefinitionAnimAction(state, fighter, *stateDef)
        : -1;
    if (!stateDef || (stateDef->hasAnim && resolvedStateAnim < 0)) {
        return false;
    }

    fighter.prevStateNo = fighter.stateNo;
    fighter.stateNo = stateNo;
    fighter.stateTime = 0;
    fighter.appliedHitDefIds.clear();
    clearStateRuntimeControllerTracking(fighter);
    fighter.attackDistanceOverride = -1;
    fighter.drawAngle = 0.0f;
    fighter.angleDrawActive = false;
    fighter.displayOffsetX = 0.0f;
    fighter.displayOffsetY = 0.0f;
    fighter.actionClipOwnerIndex = -1;
    fighter.moveContact = false;
    fighter.moveHit = false;
    fighter.moveGuarded = false;
    fighter.hitCount = 0;
    fighter.customHitState = false;
    fighter.ctrl = stateDef->ctrl;
    fighter.stateType = stateDef->stateType;
    fighter.moveType = stateDef->moveType;
    fighter.physics = stateDef->physics;
    fighter.onGround = fighter.stateType != 'A';
    fighter.sprPriority = stateDef->sprPriority;
    if (stateDef->hasVelset) {
        fighter.vx = stateDef->velsetX * static_cast<float>(fighter.facing);
        fighter.vy = stateDef->velsetY;
    }
    if (stateDef->hasAnim) {
        setFighterAction(fighter, resolvedStateAnim);
    }
    applyStateDefinitionPowerAdd(state, fighter, *stateDef);
    return true;
}

int hitAnimTypeIndex(std::string_view animtype) {
    if (startsWithNoCase(animtype, "Med") || startsWithNoCase(animtype, "Medium")) {
        return 1;
    }
    if (startsWithNoCase(animtype, "Hard") || startsWithNoCase(animtype, "Heavy")) {
        return 2;
    }
    if (startsWithNoCase(animtype, "Back")) {
        return 3;
    }
    if (startsWithNoCase(animtype, "Up")) {
        return 4;
    }
    if (startsWithNoCase(animtype, "DiagUp")) {
        return 5;
    }
    return 0;
}

bool hitGroundTypeIsLow(std::string_view groundType) {
    return startsWithNoCase(trim(groundType), "Low");
}

int hitGroundTypeValue(std::string_view groundType) {
    const std::string value = trim(groundType);
    if (startsWithNoCase(value, "Low")) {
        return 2;
    }
    if (startsWithNoCase(value, "Trip")) {
        return 3;
    }
    return 1;
}

int hitAirTypeValue(std::string_view animtype) {
    const int type = hitAnimTypeIndex(animtype);
    if (type >= 3) {
        return type;
    }
    return 1;
}

bool containsFlagNoCase(std::string_view flags, char flag) {
    const auto wanted = static_cast<char>(std::toupper(static_cast<unsigned char>(flag)));
    return std::any_of(flags.begin(), flags.end(), [wanted](char value) {
        return static_cast<char>(std::toupper(static_cast<unsigned char>(value))) == wanted;
    });
}

bool canStandingGuard(const HitDefinition& hitDef) {
    return containsFlagNoCase(hitDef.guardflag, 'M')
        || containsFlagNoCase(hitDef.guardflag, 'H');
}

bool canCrouchingGuard(const HitDefinition& hitDef) {
    return containsFlagNoCase(hitDef.guardflag, 'M')
        || containsFlagNoCase(hitDef.guardflag, 'L');
}

bool hitFlagContainsExpanded(std::string_view flags, char flag) {
    if (flag == 'H' || flag == 'L') {
        return containsFlagNoCase(flags, 'M') || containsFlagNoCase(flags, flag);
    }
    return containsFlagNoCase(flags, flag);
}

bool fighterIsLyingDown(const FighterState& fighter) {
    return fighter.onGround
        && (fighter.stateNo == 5080
            || fighter.stateNo == 5090
            || fighter.stateNo == 5100
            || fighter.stateNo == 5110
            || fighter.stateNo == 5120
            || fighter.stateNo == 5150
            || fighter.stateNo == 5160
            || fighter.stateNo == 5170);
}

bool fighterIsFallingInAir(const FighterState& fighter) {
    return !fighter.onGround && (fighter.hitFall || fighter.stateNo >= 5050);
}

bool hitFlagAllowsDefender(const HitDefinition& hitDef, const FighterState& defender) {
    const std::string rawFlags = trim(hitDef.hitflag);
    if (rawFlags.empty()) {
        return false;
    }
    const std::string flags = uppercaseCopy(rawFlags);
    const bool defenderInGetHit = defender.moveType == 'H';

    if (containsFlagNoCase(flags, '+') && !defenderInGetHit) {
        return false;
    }
    if (containsFlagNoCase(flags, '-') && defenderInGetHit) {
        return false;
    }
    if (fighterIsLyingDown(defender)) {
        return hitFlagContainsExpanded(flags, 'D');
    }
    if (!defender.onGround || defender.stateType == 'A') {
        if (fighterIsFallingInAir(defender) && !hitFlagContainsExpanded(flags, 'F')) {
            return false;
        }
        return hitFlagContainsExpanded(flags, 'A');
    }
    if (defender.stateType == 'C' || defender.hitCrouch) {
        return hitFlagContainsExpanded(flags, 'L');
    }
    return hitFlagContainsExpanded(flags, 'H');
}

GuardStance chooseDummyGuardStance(const TrainingOptions& options, const HitDefinition& hitDef, const FighterState& defender) {
    if (options.dummyGuardMode == DummyGuardMode::Off
        || !defender.onGround
        || (defender.moveType == 'H' && !defender.guarding)) {
        return GuardStance::None;
    }

    switch (options.dummyGuardMode) {
    case DummyGuardMode::Stand:
        return canStandingGuard(hitDef) ? GuardStance::Stand : GuardStance::None;
    case DummyGuardMode::Crouch:
        return canCrouchingGuard(hitDef) ? GuardStance::Crouch : GuardStance::None;
    case DummyGuardMode::Auto:
        if (containsFlagNoCase(hitDef.guardflag, 'L') && canCrouchingGuard(hitDef)) {
            return GuardStance::Crouch;
        }
        if (canStandingGuard(hitDef)) {
            return GuardStance::Stand;
        }
        return canCrouchingGuard(hitDef) ? GuardStance::Crouch : GuardStance::None;
    case DummyGuardMode::Off:
    default:
        return GuardStance::None;
    }
}

const FighterInputState* latestFighterInput(const FighterState& fighter) {
    return fighter.inputHistory.empty() ? nullptr : &fighter.inputHistory.back().input;
}

bool fighterInputHoldingBack(const FighterInputState& input, const FighterState& fighter) {
    return fighter.facing >= 0 ? input.left : input.right;
}

GuardStance choosePlayerGuardStance(const HitDefinition& hitDef, const FighterState& defender) {
    if (!defender.onGround || (defender.moveType == 'H' && !defender.guarding) || (!defender.ctrl && !defender.guarding)) {
        return GuardStance::None;
    }
    const FighterInputState* input = latestFighterInput(defender);
    if (!input || !fighterInputHoldingBack(*input, defender)) {
        return GuardStance::None;
    }
    if (input->down) {
        return canCrouchingGuard(hitDef) ? GuardStance::Crouch : GuardStance::None;
    }
    return canStandingGuard(hitDef) ? GuardStance::Stand : GuardStance::None;
}

GuardStance dummyGuardIdleStance(DummyGuardMode mode) {
    switch (mode) {
    case DummyGuardMode::Stand:
        return GuardStance::Stand;
    case DummyGuardMode::Crouch:
        return GuardStance::Crouch;
    case DummyGuardMode::Auto:
        return GuardStance::Stand;
    case DummyGuardMode::Off:
    default:
        return GuardStance::None;
    }
}

int fallbackGroundHitShakeAction(const HitDefinition& hitDef) {
    const int base = hitGroundTypeIsLow(hitDef.groundType) ? 5010 : 5000;
    return base + std::min(hitAnimTypeIndex(hitDef.animtype), 2);
}

int fallbackGroundHitRecoverAction(const HitDefinition& hitDef) {
    const int base = hitGroundTypeIsLow(hitDef.groundType) ? 5015 : 5005;
    return base + std::min(hitAnimTypeIndex(hitDef.animtype), 2);
}

int hitShakeActionForFighter(const AppState& state, const FighterState& target, const HitDefinition& hitDef, bool crouching) {
    const int variant = std::min(hitAnimTypeIndex(hitDef.animtype), 2);
    const int fallback = fallbackGroundHitShakeAction(hitDef);
    if (crouching) {
        return firstExistingActionForActor(state, target, { 5020 + variant, fallback, 5010 + variant, 5000 + variant, 0 });
    }
    return firstExistingActionForActor(state, target, { fallback, 5000 + variant, 5010 + variant, 0 });
}

int hitRecoverActionForFighter(const AppState& state, const FighterState& target, const HitDefinition& hitDef, bool crouching) {
    const int variant = std::min(hitAnimTypeIndex(hitDef.animtype), 2);
    const int fallback = fallbackGroundHitRecoverAction(hitDef);
    if (crouching) {
        return firstExistingActionForActor(state, target, { 5025 + variant, fallback, 5015 + variant, 5005 + variant, 0 });
    }
    return firstExistingActionForActor(state, target, { fallback, 5005 + variant, 5015 + variant, 0 });
}

bool hitGroundTypeIsTrip(std::string_view groundType) {
    return startsWithNoCase(trim(groundType), "Trip");
}

bool hitDefCausesFall(const HitDefinition& hitDef, bool wasAirborne) {
    return hitDef.fall
        || (wasAirborne && hitDef.airFall)
        || (!wasAirborne && hitGroundTypeIsTrip(hitDef.groundType));
}

bool hitDefCausesFall(const HitDefinition& hitDef, const FighterState& target) {
    return hitDefCausesFall(hitDef, !target.onGround || target.stateType == 'A');
}

int hitTimeForGetHitVars(const HitDefinition& hitDef, bool wasAirborne, bool wasLyingDown, float hitVelocityY) {
    if (wasLyingDown) {
        return std::max(0, hitDef.downHitTime);
    }

    // M.U.G.E.N/Ikemen use air.hittime for grounded hits that launch with a Y velocity.
    if (wasAirborne || std::abs(hitVelocityY) > 0.001f) {
        return std::max(0, hitDef.airHitTime);
    }

    return std::max(0, hitDef.groundHitTime);
}

void setGetHitVarsFromHitDef(
    FighterState& target,
    const HitDefinition& hitDef,
    bool wasAirborne,
    bool wasLyingDown,
    bool fallHit,
    float hitVelocityX,
    float hitVelocityY,
    float downVelocityX,
    float downVelocityY) {
    target.getHitAnimType = hitAnimTypeIndex(hitDef.animtype);
    target.getHitGroundType = hitGroundTypeValue(hitDef.groundType);
    target.getHitAirType = hitAirTypeValue(hitDef.animtype);
    target.getHitSlideTime = wasAirborne ? 0 : std::max(0, hitDef.groundSlideTime);
    target.getHitHitTime = hitTimeForGetHitVars(hitDef, wasAirborne, wasLyingDown, hitVelocityY);
    target.getHitCtrlTime = target.getHitHitTime;
    target.getHitHitCount = std::max(0, target.getHitHitCount + 1);
    target.hitVelocityX = hitVelocityX;
    target.hitVelocityY = hitVelocityY;
    target.hitDownVelocityX = downVelocityX;
    target.hitDownVelocityY = downVelocityY;
    if (fallHit && hitDef.hasFallYVelocity) {
        target.hitVelocityY = hitDef.fallYVelocity;
    }
}

bool fighterIsCrouchingForHit(const FighterState& fighter) {
    return fighter.stateType == 'C'
        || fighter.crouchGuard
        || fighter.stateNo == 10
        || fighter.stateNo == 11
        || fighter.stateNo == 12
        || fighter.stateNo == 131;
}

bool fighterIsLyingDownForHit(const FighterState& fighter) {
    return fighter.stateType == 'L'
        || fighterIsLyingDown(fighter)
        || fighter.stateNo == 5080
        || fighter.stateNo == 5090
        || fighter.stateNo == 5100
        || fighter.stateNo == 5110
        || fighter.stateNo == 5120
        || fighter.stateNo == 5150
        || fighter.stateNo == 5160
        || fighter.stateNo == 5170;
}

int fallAirActionForHit(const AppState& state, const FighterState& target, const HitDefinition& hitDef) {
    const std::string_view animType = hitDef.fallAnimtype.empty() ? std::string_view(hitDef.animtype) : std::string_view(hitDef.fallAnimtype);
    const int candidate = 5050 + hitAnimTypeIndex(animType);
    if (findExactClipForActor(state, target, candidate)) {
        return candidate;
    }
    return findExactClipForActor(state, target, 5050) ? 5050 : 0;
}

int fallLandActionForFighter(const AppState& state, const FighterState& target) {
    const int variant = target.hitFallAirAction % 10;
    const int candidate = 5100 + variant;
    if (findExactClipForActor(state, target, candidate)) {
        return candidate;
    }
    return findExactClipForActor(state, target, 5100) ? 5100 : 0;
}

void applyHitDefP1Facing(FighterState& attacker, const HitDefinition& hitDef) {
    if (!hitDef.hasP1Facing || hitDef.p1Facing == 0) {
        return;
    }
    if (hitDef.p1Facing < 0) {
        attacker.facing *= -1;
    }
}

void applyHitDefP2Facing(FighterState& target, const HitDefinition& hitDef, int attackerFacing) {
    if (!hitDef.hasP2Facing || hitDef.p2Facing == 0) {
        return;
    }
    target.facing = hitDef.p2Facing > 0 ? -attackerFacing : attackerFacing;
}

int guardStartStateNo() {
    return 120;
}

int guardIdleStateNo(GuardStance stance) {
    return stance == GuardStance::Crouch ? 131 : 130;
}

int guardEndStateNo() {
    return 140;
}

int guardStartAction(GuardStance stance) {
    return stance == GuardStance::Crouch ? 121 : 120;
}

int guardIdleAction(GuardStance stance) {
    return stance == GuardStance::Crouch ? 131 : 130;
}

int guardEndAction(GuardStance stance) {
    return stance == GuardStance::Crouch ? 141 : 140;
}

bool isGroundGuardCommonState(int stateNo) {
    return stateNo == 120
        || stateNo == 121
        || stateNo == 122
        || stateNo == 130
        || stateNo == 131
        || stateNo == 140;
}

GuardStance guardStanceFromCommonState(const FighterState& target) {
    return target.crouchGuard
        || target.stateType == 'C'
        || target.stateNo == guardIdleStateNo(GuardStance::Crouch)
        || target.action == guardStartAction(GuardStance::Crouch)
        || target.action == guardEndAction(GuardStance::Crouch)
        ? GuardStance::Crouch
        : GuardStance::Stand;
}

void enterGroundGuardReadyState(const AppState& state, FighterState& target, GuardStance stance) {
    const bool crouch = stance == GuardStance::Crouch;
    const int startAction = guardStartAction(stance);
    const int idleAction = guardIdleAction(stance);
    const int action = firstExistingActionForActor(state, target, { startAction, idleAction, 0 });
    target.guarding = false;
    target.crouchGuard = crouch;
    target.stateNo = action == idleAction ? guardIdleStateNo(stance) : guardStartStateNo();
    target.stateTime = 0;
    clearStateRuntimeControllerTracking(target);
    target.moveContact = false;
    target.moveHit = false;
    target.moveGuarded = false;
    target.stateType = crouch ? 'C' : 'S';
    target.moveType = 'I';
    target.physics = crouch ? 'C' : 'S';
    target.ctrl = true;
    target.onGround = true;
    target.vx = 0.0f;
    target.vy = 0.0f;
    target.hitPauseTicks = 0;
    target.hitStunTicks = 0;
    target.hitSlideTicks = 0;
    target.hitVelocityX = 0.0f;
    target.hitVelocityY = 0.0f;
    target.getHitAnimType = 0;
    target.getHitGroundType = 0;
    target.getHitAirType = 0;
    target.getHitSlideTime = 0;
    target.getHitHitTime = 0;
    target.getHitCtrlTime = 0;
    target.getHitHitCount = 0;
    target.hitRecoverAnim = 5005;
    target.hitCrouch = false;
    target.hitAirborne = false;
    setFighterAction(target, action);
}

void enterGroundGuardEndState(const AppState& state, FighterState& target) {
    const GuardStance stance = guardStanceFromCommonState(target);
    const int action = findExactClipForActor(state, target, guardEndAction(stance)) ? guardEndAction(stance) : 0;
    if (action == 0) {
        enterState(state, target, 0);
        return;
    }

    target.guarding = false;
    target.crouchGuard = stance == GuardStance::Crouch;
    target.stateNo = guardEndStateNo();
    target.stateTime = 0;
    target.moveType = 'I';
    target.stateType = target.crouchGuard ? 'C' : 'S';
    target.physics = target.crouchGuard ? 'C' : 'S';
    target.ctrl = false;
    target.onGround = true;
    target.vx = 0.0f;
    target.vy = 0.0f;
    setFighterAction(target, action);
}

void updateGroundGuardReadyState(const AppState& state, FighterState& target) {
    if (!isGroundGuardCommonState(target.stateNo)) {
        return;
    }

    const GuardStance stance = guardStanceFromCommonState(target);
    const int startAction = guardStartAction(stance);
    const int idleAction = guardIdleAction(stance);

    target.vx = 0.0f;
    target.vy = 0.0f;
    target.onGround = true;
    if (target.stateNo == guardStartStateNo() && fighterAnimationEnded(state, target)) {
        target.stateNo = findExactClipForActor(state, target, idleAction) ? guardIdleStateNo(stance) : guardStartStateNo();
        target.stateTime = 0;
        setFighterAction(target, findExactClipForActor(state, target, idleAction) ? idleAction : startAction);
        return;
    }
    if (target.stateNo == guardEndStateNo() && fighterAnimationEnded(state, target)) {
        enterState(state, target, 0);
    }
}

bool updateGroundGuardInputState(const AppState& state, FighterState& target, const FighterInputState& input) {
    if (!isGroundGuardCommonState(target.stateNo)) {
        return false;
    }

    if (target.stateNo == guardEndStateNo()) {
        updateGroundGuardReadyState(state, target);
        return true;
    }

    if (!fighterInputHoldingBack(input, target)) {
        enterGroundGuardEndState(state, target);
        return true;
    }

    const GuardStance desiredStance = input.down && target.onGround ? GuardStance::Crouch : GuardStance::Stand;
    if (target.stateNo != guardStartStateNo() && guardStanceFromCommonState(target) != desiredStance) {
        enterGroundGuardReadyState(state, target, desiredStance);
        return true;
    }

    updateGroundGuardReadyState(state, target);
    return true;
}

int fightHitPauseTicks(const AppState& state, int ticks, int minimum) {
    const int resolved = std::max(minimum, ticks);
    return state.frontend.pendingMode == PendingMode::Arena
        ? std::min(resolved, 12)
        : resolved;
}

float arenaFallInitialYVelocityCap(const FighterState&) {
    return -2.2f;
}

float arenaFallBounceYVelocityCap(const FighterState&) {
    return -1.0f;
}

void clampArenaHitFallRuntime(const AppState& state, FighterState& fighter) {
    if (!isArenaMode(state) || !fighter.hitFall) {
        return;
    }
    fighter.hitFallRecover = false;
    fighter.hitVelocityY = std::max(fighter.hitVelocityY, arenaFallInitialYVelocityCap(fighter));
    fighter.hitFallBounceYVelocity = std::max(fighter.hitFallBounceYVelocity, arenaFallBounceYVelocityCap(fighter));
    fighter.hitFallYAccel = std::max(fighter.hitFallYAccel, fighter.hitFallTrip ? 0.72f : 0.70f);
}

void clampArenaAppliedFallVelocity(const AppState& state, FighterState& fighter, bool bounceVelocity) {
    if (!isArenaMode(state) || !fighter.hitFall) {
        return;
    }
    const float cap = bounceVelocity
        ? arenaFallBounceYVelocityCap(fighter)
        : arenaFallInitialYVelocityCap(fighter);
    fighter.vy = std::max(fighter.vy, cap);
    if (fighter.hitFallTrip && fighter.onGround) {
        fighter.vy = std::max(fighter.vy, 0.0f);
    }
}

void enterGroundGetHitState(
    const AppState& state,
    FighterState& target,
    const HitDefinition& hitDef,
    float sourceX,
    float sourceY,
    int attackerFacing) {
    const bool wasAirborne = !target.onGround || target.stateType == 'A';
    const bool wasCrouching = fighterIsCrouchingForHit(target);
    const bool wasLyingDown = fighterIsLyingDownForHit(target);
    const bool wasLyingOnFloor = wasLyingDown && target.onGround && std::abs(target.y) <= 0.5f;
    const bool downedHit = wasLyingOnFloor;
    const bool useAirVelocity = wasAirborne && hitDef.hasAirVelocity;
    const float authoredDownVelocityX = hitDef.hasDownVelocity ? hitDef.downVelocityX : hitDef.airVelocityX;
    const float authoredDownVelocityY = hitDef.hasDownVelocity ? hitDef.downVelocityY : hitDef.airVelocityY;
    const float hitVelocityX = wasLyingOnFloor
        ? authoredDownVelocityX
        : (useAirVelocity ? hitDef.airVelocityX : hitDef.groundVelocityX);
    const float hitVelocityY = wasLyingOnFloor
        ? authoredDownVelocityY
        : (useAirVelocity ? hitDef.airVelocityY : hitDef.groundVelocityY);
    const float signedDownVelocityX = -authoredDownVelocityX * static_cast<float>(attackerFacing);
    const float signedDownVelocityY = authoredDownVelocityY;
    const bool standingTripHit = !wasAirborne && !downedHit && hitGroundTypeIsTrip(hitDef.groundType);
    const bool fallHit = hitDef.fall || (!target.onGround && hitDef.airFall) || standingTripHit;
    const bool arenaGroundLaunchFall = isArenaMode(state) && !wasAirborne && !downedHit && !fallHit && hitVelocityY < -0.05f;
    const bool downedAirHit = downedHit && std::abs(hitVelocityY) > 0.05f;
    const bool liedownBounce = downedHit && (hitDef.downBounce || fallHit) && hitVelocityY < -0.05f;
    const bool resolvedFallHit = downedHit ? (liedownBounce || (downedAirHit && fallHit)) : (fallHit || arenaGroundLaunchFall);

    target.guarding = false;
    target.stateNo = wasAirborne ? 5030 : (wasLyingDown ? (liedownBounce ? 5090 : 5080) : 5000);
    target.stateTime = 0;
    clearStateRuntimeControllerTracking(target);
    target.moveContact = false;
    target.moveHit = false;
    target.moveGuarded = false;
    target.stateType = wasAirborne || liedownBounce ? 'A' : (wasLyingDown ? 'L' : (wasCrouching ? 'C' : 'S'));
    target.moveType = 'H';
    target.physics = wasAirborne || liedownBounce ? 'A' : 'N';
    target.ctrl = false;
    target.onGround = !(wasAirborne || liedownBounce);
    if (hitDef.hasSnap) {
        target.x = sourceX + hitDef.snapX * static_cast<float>(attackerFacing);
        target.y = sourceY + hitDef.snapY;
        target.triggerY = target.y;
    }
    target.vx = 0.0f;
    target.vy = 0.0f;
    target.hitPauseTicks = fightHitPauseTicks(state, hitDef.pausetimeP2, 1);
    const int hitTime = hitTimeForGetHitVars(hitDef, wasAirborne, wasLyingDown, hitVelocityY);
    target.hitStunTicks = std::max(hitTime, target.hitPauseTicks);
    target.hitSlideTicks = wasAirborne ? 0 : std::max(0, hitDef.groundSlideTime);
    setGetHitVarsFromHitDef(
        target,
        hitDef,
        wasAirborne,
        wasLyingDown,
        resolvedFallHit,
        -hitVelocityX * static_cast<float>(attackerFacing),
        hitVelocityY,
        signedDownVelocityX,
        signedDownVelocityY);
    target.hitRecoverAnim = hitRecoverActionForFighter(state, target, hitDef, wasCrouching);
    target.hitFall = resolvedFallHit || (wasLyingDown && !downedHit);
    target.hitFallTrip = standingTripHit;
    target.hitDowned = downedHit;
    target.hitCrouch = wasCrouching;
    target.hitAirborne = wasAirborne || liedownBounce || downedAirHit;
    target.hitFallRecover = hitDef.fallRecover;
    target.hitFallRecoverTime = hitDef.fallRecoverTime;
    target.hitDownRecover = hitDef.downRecover;
    target.hitDownRecoverTime = hitDef.downRecoverTime;
    target.hitFallDamage = hitDef.fallDamage;
    target.hitFallYAccel = hitDef.hasYAccel ? hitDef.yAccel : characterConstantsForActor(state, target).movementYAccel;
    target.hitFallAirAction = fallAirActionForHit(state, target, hitDef);
    target.hitFallBounceXVelocity = hitDef.hasFallXVelocity
        ? -hitDef.fallXVelocity * static_cast<float>(attackerFacing)
        : target.hitVelocityX;
    target.hitFallBounceYVelocity = hitDef.fallYVelocity;
    if (wasLyingOnFloor && !hitDef.downBounce && std::abs(hitVelocityY) > 0.05f) {
        target.hitFallBounceYVelocity = 0.0f;
    }
    clampArenaHitFallRuntime(state, target);
    target.hitFallEnvShake = hitDef.fallEnvShake;
    target.hitFallEnvShakePlayed = false;
    if (downedHit) {
        const int downHitAction = std::abs(target.hitVelocityY) <= 0.05f ? 5080 : 5090;
        target.sysVars[2] = (downHitAction == 5090 && !findExactClipForActor(state, target, 5090)) ? 5030 : downHitAction;
    }

    const int action = wasAirborne
        ? firstExistingActionForActor(state, target, { 5030, fallAirActionForHit(state, target, hitDef), hitShakeActionForFighter(state, target, hitDef, false), 0 })
        : (wasLyingDown
            ? firstExistingActionForActor(state, target, { liedownBounce ? 5090 : 5080, fallLandActionForFighter(state, target), 5110, 0 })
            : hitShakeActionForFighter(state, target, hitDef, wasCrouching));
    const int tripAction = target.hitFallTrip && !wasLyingDown && findExactClipForActor(state, target, 5070) ? 5070 : 0;
    const int shakeAction = tripAction != 0 && !wasAirborne ? tripAction : firstExistingActionForActor(state, target, { action, 5000, 0 });
    setFighterAction(target, shakeAction);
    applyHitDefP2Facing(target, hitDef, attackerFacing);
}

bool enterCustomHitState(
    const AppState& state,
    FighterState& target,
    const HitDefinition& hitDef,
    int attackerFacing,
    int attackerStateOwnerIndex) {
    const int previousCustomOwnerIndex = target.customStateOwnerIndex;
    if (hitDef.p2GetP1State
        && (attackerStateOwnerIndex < 0 || attackerStateOwnerIndex >= static_cast<int>(state.fighters.size()))) {
        return false;
    }
    target.customStateOwnerIndex = hitDef.p2GetP1State ? attackerStateOwnerIndex : -1;
    if (hitDef.p2StateNo < 0 || !findStateDefinitionForActor(state, target, hitDef.p2StateNo)) {
        target.customStateOwnerIndex = previousCustomOwnerIndex;
        return false;
    }

    const bool wasAirborne = !target.onGround || target.stateType == 'A';
    const bool wasLyingDown = fighterIsLyingDownForHit(target);
    const float hitVelocityX = hitDef.hasAirVelocity ? hitDef.airVelocityX : hitDef.groundVelocityX;
    const float hitVelocityY = hitDef.hasAirVelocity ? hitDef.airVelocityY : hitDef.groundVelocityY;
    const float downVelocityX = (hitDef.hasDownVelocity ? hitDef.downVelocityX : hitDef.airVelocityX)
        * -static_cast<float>(attackerFacing);
    const float downVelocityY = hitDef.hasDownVelocity ? hitDef.downVelocityY : hitDef.airVelocityY;
    if (!enterState(state, target, hitDef.p2StateNo)) {
        target.customStateOwnerIndex = previousCustomOwnerIndex;
        return false;
    }
    target.customStateOwnerIndex = hitDef.p2GetP1State ? attackerStateOwnerIndex : -1;
    target.customHitState = true;
    target.moveType = 'H';
    target.ctrl = false;
    target.hitPauseTicks = fightHitPauseTicks(state, hitDef.pausetimeP2, 0);
    target.hitStunTicks = std::max(hitDef.groundHitTime, target.hitPauseTicks);
    target.hitSlideTicks = std::max(0, hitDef.groundSlideTime);
    setGetHitVarsFromHitDef(
        target,
        hitDef,
        wasAirborne,
        wasLyingDown,
        hitDefCausesFall(hitDef, wasAirborne),
        -hitVelocityX * static_cast<float>(attackerFacing),
        hitVelocityY,
        downVelocityX,
        downVelocityY);
    applyHitDefP2Facing(target, hitDef, attackerFacing);
    return true;
}

void enterGroundGuardHitState(const AppState& state, FighterState& target, const HitDefinition& hitDef, int attackerFacing, GuardStance stance) {
    const bool crouch = stance == GuardStance::Crouch;
    target.guarding = true;
    target.crouchGuard = crouch;
    target.stateNo = crouch ? 152 : 150;
    target.stateTime = 0;
    clearStateRuntimeControllerTracking(target);
    target.moveContact = false;
    target.stateType = crouch ? 'C' : 'S';
    target.moveType = 'H';
    target.physics = 'N';
    target.ctrl = false;
    target.onGround = true;
    target.vx = 0.0f;
    target.vy = 0.0f;
    target.hitPauseTicks = fightHitPauseTicks(state, hitDef.pausetimeP2, 1);
    target.hitStunTicks = std::max(1, hitDef.groundHitTime);
    target.hitSlideTicks = std::max(0, hitDef.groundSlideTime);
    target.hitVelocityX = -hitDef.guardVelocityX * static_cast<float>(attackerFacing);
    target.hitVelocityY = hitDef.guardVelocityY;
    target.getHitAnimType = hitAnimTypeIndex(hitDef.animtype);
    target.getHitGroundType = hitGroundTypeValue(hitDef.groundType);
    target.getHitAirType = hitAirTypeValue(hitDef.animtype);
    target.getHitSlideTime = target.hitSlideTicks;
    target.getHitHitTime = target.hitStunTicks;
    target.getHitCtrlTime = target.hitStunTicks;
    target.getHitHitCount = std::max(0, target.getHitHitCount + 1);
    target.hitRecoverAnim = crouch ? 131 : 130;

    const int action = crouch ? 151 : 150;
    const int guardAction = findExactClipForActor(state, target, action) ? action : 0;
    setFighterAction(target, guardAction);
}

void clearFighterHitRuntime(FighterState& fighter) {
    fighter.vx = 0.0f;
    fighter.vy = 0.0f;
    fighter.hitPauseTicks = 0;
    fighter.hitStunTicks = 0;
    fighter.hitSlideTicks = 0;
    fighter.hitVelocityX = 0.0f;
    fighter.hitVelocityY = 0.0f;
    fighter.getHitAnimType = 0;
    fighter.getHitGroundType = 0;
    fighter.getHitAirType = 0;
    fighter.getHitSlideTime = 0;
    fighter.getHitHitTime = 0;
    fighter.getHitCtrlTime = 0;
    fighter.getHitHitCount = 0;
    fighter.hitRecoverAnim = 5005;
    fighter.hitFall = false;
    fighter.hitFallTrip = false;
    fighter.hitDowned = false;
    fighter.hitCrouch = false;
    fighter.hitAirborne = false;
    fighter.hitFallRecover = true;
    fighter.hitFallRecoverTime = 0;
    fighter.hitDownRecover = true;
    fighter.hitDownRecoverTime = -1;
    fighter.hitDownVelocityX = 0.0f;
    fighter.hitDownVelocityY = 0.0f;
    fighter.hitFallDamage = 0;
    fighter.hitFallYAccel = 0.0f;
    fighter.hitFallAirAction = 5050;
    fighter.hitFallBounceXVelocity = 0.0f;
    fighter.hitFallBounceYVelocity = -4.5f;
    fighter.hitFallEnvShake = {};
    fighter.hitFallEnvShakePlayed = false;
    fighter.notHitByTicks = 0;
    fighter.notHitByValue.clear();
    fighter.hitByTicks = 0;
    fighter.hitByValue.clear();
    fighter.customHitState = false;
    fighter.customStateOwnerIndex = -1;
    fighter.actionClipOwnerIndex = -1;
    fighter.targetIndex = -1;
    fighter.targetHitId = -1;
    fighter.targetTicks = 0;
    fighter.boundByIndex = -1;
    fighter.bindTicks = 0;
    fighter.targetBindPositionActive = false;
    fighter.targetBindOffsetX = 0.0f;
    fighter.targetBindOffsetY = 0.0f;
    fighter.targetBindFacing = 0;
    fighter.appliedHitDefIds.clear();
    clearStateRuntimeControllerTracking(fighter);
    fighter.moveContact = false;
    fighter.moveHit = false;
    fighter.moveGuarded = false;
    fighter.guarding = false;
    fighter.crouchGuard = false;
    fighter.onGround = true;
    fighter.jumpBaseAction = 0;
    fighter.jumpPeakActionApplied = false;
}

void applyTrainingPowerMode(AppState& state) {
    if (state.frontend.pendingMode != PendingMode::Training || state.training.options.powerMode != TrainingPowerMode::Max) {
        return;
    }
    for (auto& fighter : state.fighters) {
        fighter.power = std::max(0, characterConstantsForActor(state, fighter).maxPower);
    }
}

const CharacterConstants& characterConstantsForFighterIndex(const AppState& state, size_t fighterIndex) {
    if (const auto* runtime = characterRuntimeForFighterIndex(state, fighterIndex)) {
        return runtime->constants;
    }
    return state.characterConstants;
}

int characterMaxLifeForConstants(const CharacterConstants& constants) {
    return std::max(1, constants.life);
}

int characterMaxLifeForActor(const AppState& state, const FighterState& fighter) {
    if (fighter.maxLifeOverride > 0) {
        return fighter.maxLifeOverride;
    }
    return characterMaxLifeForConstants(characterConstantsForActor(state, fighter));
}

int characterMaxLifeForFighterIndex(const AppState& state, size_t fighterIndex) {
    if (fighterIndex < state.fighters.size() && state.fighters[fighterIndex].maxLifeOverride > 0) {
        return state.fighters[fighterIndex].maxLifeOverride;
    }
    return characterMaxLifeForConstants(characterConstantsForFighterIndex(state, fighterIndex));
}

const std::vector<AnimationClip>* characterClipsForFighterIndex(const AppState& state, size_t fighterIndex) {
    if (const auto* runtime = characterRuntimeForFighterIndex(state, fighterIndex)) {
        return &runtime->clips;
    }
    if ((state.frontend.pendingMode == PendingMode::Arena || state.frontend.pendingMode == PendingMode::Story)
        && fighterIndex < state.arenaFighterClips.size()
        && !state.arenaFighterClips[fighterIndex].empty()) {
        return &state.arenaFighterClips[fighterIndex];
    }
    if (fighterIndex == 1 && !state.opponentCharacterClips.empty()) {
        return &state.opponentCharacterClips;
    }
    return &state.characterClips;
}

float saneCharacterScale(float value) {
    return std::isfinite(value) && value > 0.0f ? std::clamp(value, 0.05f, 4.0f) : 1.0f;
}

float maxSpriteHeightForClip(const AnimationClip& clip) {
    float height = 0.0f;
    for (const auto& frame : clip.frames) {
        height = std::max(height, static_cast<float>(frame.sprite.height));
    }
    return height;
}

float sampledCharacterVisualHeight(const std::vector<AnimationClip>& clips) {
    constexpr std::array<int, 8> preferredActions{ 0, 20, 21, 40, 41, 42, 43, 50 };
    float height = 0.0f;
    for (const int action : preferredActions) {
        if (const AnimationClip* clip = findExactClipInSet(clips, action)) {
            height = std::max(height, maxSpriteHeightForClip(*clip));
        }
    }
    if (height > 0.0f) {
        return height;
    }
    for (size_t i = 0; i < std::min<size_t>(clips.size(), 24); ++i) {
        height = std::max(height, maxSpriteHeightForClip(clips[i]));
    }
    return height;
}

float characterAutoFitFactor(const std::vector<AnimationClip>& clips, float baseScaleY) {
    constexpr float kMaxComfortableCharacterHeight = 132.0f;
    constexpr float kMinAutoFitFactor = 0.20f;
    const float visualHeight = sampledCharacterVisualHeight(clips);
    if (visualHeight <= 0.0f || baseScaleY <= 0.0f) {
        return 1.0f;
    }
    const float scaledHeight = visualHeight * baseScaleY;
    if (scaledHeight <= kMaxComfortableCharacterHeight) {
        return 1.0f;
    }
    return std::clamp(kMaxComfortableCharacterHeight / scaledHeight, kMinAutoFitFactor, 1.0f);
}

std::pair<float, float> initialFighterScaleForIndex(const AppState& state, size_t fighterIndex) {
    const CharacterConstants& constants = characterConstantsForFighterIndex(state, fighterIndex);
    const float baseScaleX = saneCharacterScale(constants.sizeScaleX);
    const float baseScaleY = saneCharacterScale(constants.sizeScaleY);
    const std::vector<AnimationClip>* clips = characterClipsForFighterIndex(state, fighterIndex);
    const float autoFit = clips ? characterAutoFitFactor(*clips, baseScaleY) : 1.0f;
    return { baseScaleX * autoFit, baseScaleY * autoFit };
}

void applyInitialFighterScale(AppState& state, FighterState& fighter, size_t fighterIndex) {
    const auto [scaleX, scaleY] = initialFighterScaleForIndex(state, fighterIndex);
    fighter.scaleX = scaleX;
    fighter.scaleY = scaleY;
}

float clampFighterOriginToStage(float x, const StageSlot& stage);
void clearProgressionMatchAward(AppState& state);

#include "FightSessionRuntime.h"
