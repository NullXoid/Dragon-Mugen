#pragma once

// Internal App.cpp implementation shard.
// Command state execution, landing, recovery, dizzy, CPU, and training hook declarations.

#include "CommandStateEligibility.h"

bool expressionMentionsSelfStateNo(std::string_view expression) {
    const std::string lowered = lowercaseCopy(expression);
    size_t pos = lowered.find("stateno");
    while (pos != std::string::npos) {
        const bool leftOk = pos == 0 || !std::isalnum(static_cast<unsigned char>(lowered[pos - 1]));
        const size_t right = pos + std::string_view("stateno").size();
        const bool rightOk = right >= lowered.size() || !std::isalnum(static_cast<unsigned char>(lowered[right]));
        if (leftOk && rightOk) {
            return true;
        }
        pos = lowered.find("stateno", pos + 1);
    }
    return false;
}

bool commandEntryHasSelfStateNoGate(const CommandStateEntry& entry) {
    for (const auto& condition : entry.intConditions) {
        if (condition.subject == CommandConditionSubject::StateNo) {
            return true;
        }
    }
    for (const auto& condition : entry.intRangeConditions) {
        if (condition.subject == CommandConditionSubject::StateNo) {
            return true;
        }
    }
    for (const auto& condition : entry.expressionConditions) {
        if (expressionMentionsSelfStateNo(condition.lhs) || expressionMentionsSelfStateNo(condition.rhs)) {
            return true;
        }
    }
    return std::any_of(entry.booleanExpressions.begin(), entry.booleanExpressions.end(), expressionMentionsSelfStateNo);
}

bool commandEntryConditionRequiresTruthySubject(const MugenExpressionCondition& condition, std::string_view subject) {
    if (!equalsNoCase(trim(condition.lhs), subject)) {
        return false;
    }
    const auto rhs = parsePlainFloatValue(condition.rhs);
    if (!rhs) {
        return false;
    }
    switch (condition.op) {
    case CompareOp::Equal:
        return *rhs != 0.0f;
    case CompareOp::NotEqual:
        return *rhs == 0.0f;
    case CompareOp::Greater:
        return *rhs < 1.0f;
    case CompareOp::GreaterEqual:
        return *rhs <= 1.0f;
    case CompareOp::Less:
        return false;
    case CompareOp::LessEqual:
        return false;
    }
    return false;
}

bool commandEntryMentionsRuntimeFlag(const CommandStateEntry& entry, std::string_view flag) {
    for (const auto& condition : entry.expressionConditions) {
        if (commandEntryConditionRequiresTruthySubject(condition, flag)) {
            return true;
        }
    }
    return std::any_of(entry.booleanExpressions.begin(), entry.booleanExpressions.end(), [flag](const std::string& expression) {
        return lowercaseCopy(expression).find(flag) != std::string::npos;
    });
}

void applyCommandEntryDemoRuntimePrereqs(FighterState& fighter, const CommandStateEntry& entry) {
    if (entry.requiresMoveContact || commandEntryMentionsRuntimeFlag(entry, "movecontact")) {
        fighter.moveContact = true;
    }
    if (commandEntryMentionsRuntimeFlag(entry, "movehit")) {
        fighter.moveContact = true;
        fighter.moveHit = true;
        fighter.moveGuarded = false;
    }
    if (commandEntryMentionsRuntimeFlag(entry, "moveguarded")) {
        fighter.moveContact = true;
        fighter.moveGuarded = true;
    }
}

bool applyCommandState(AppState& state, FighterState& fighter, const FighterState* opponent, const std::vector<std::string>& commands) {
    for (const auto& entry : commandEntriesForActor(state, fighter)) {
        if (canEnterCommandState(state, fighter, opponent, entry, commands)) {
            const auto targetState = resolveCommandTargetState(state, fighter, opponent, entry, commands);
            if (!targetState) {
                continue;
            }
            enterState(state, fighter, *targetState);
            return true;
        }
    }
    return false;
}

bool applyPreferredCommandState(
    AppState& state,
    FighterState& fighter,
    const FighterState* opponent,
    const std::vector<std::string>& commands,
    const CommandStateEntry& entry) {
    if (!canEnterCommandState(state, fighter, opponent, entry, commands)) {
        return false;
    }
    const auto targetState = resolveCommandTargetState(state, fighter, opponent, entry, commands);
    if (!targetState) {
        return false;
    }
    enterState(state, fighter, *targetState);
    return true;
}

bool applyHitCommandState(AppState& state, FighterState& fighter, const FighterState* opponent, const std::vector<std::string>& commands) {
    for (const auto& entry : commandEntriesForActor(state, fighter)) {
        if (!commandEntryHasSelfStateNoGate(entry) || !canEnterCommandState(state, fighter, opponent, entry, commands)) {
            continue;
        }
        const auto targetState = resolveCommandTargetState(state, fighter, opponent, entry, commands);
        if (!targetState) {
            continue;
        }
        enterState(state, fighter, *targetState);
        return true;
    }
    return false;
}

void updateStateZeroFromMovement(const AppState& state, FighterState& fighter) {
    if (fighter.stateNo != 0 || fighter.guarding) {
        return;
    }
    fighter.ctrl = true;
    fighter.stateType = fighter.onGround ? 'S' : 'A';
    fighter.moveType = 'I';
    fighter.physics = fighter.onGround ? 'S' : 'A';
    if (!fighter.onGround && !fighter.jumpPeakActionApplied && fighter.vy > -2.0f) {
        const int peakAction = airMovementBaseAction(fighter) + 3;
        fighter.jumpPeakActionApplied = findExactClipForActor(state, fighter, peakAction) != nullptr;
    }
    setFighterAction(fighter, chooseMovementAction(state, fighter));
}

void finishStateIfAnimationEnded(const AppState& state, FighterState& fighter) {
    if (fighter.stateNo == 0
        || fighter.moveType == 'H'
        || isCrouchStateNo(fighter.stateNo)
        || fighter.stateNo == 20
        || isGroundGuardCommonState(fighter.stateNo)) {
        return;
    }

    if (fighterAnimationEnded(state, fighter)) {
        const StateDefinition* stateDef = findStateDefinitionForActor(state, fighter, fighter.stateNo);
        if (stateDef && stateDef->hasAnimEndChangeState) {
            std::optional<bool> ctrl;
            if (stateDef->hasAnimEndCtrl) {
                ctrl = stateDef->animEndCtrl;
            }
            const auto targetState = evalMugenExpression(
                state,
                fighter,
                stateDef->animEndChangeStateExpression.empty()
                    ? std::to_string(stateDef->animEndChangeState)
                    : stateDef->animEndChangeStateExpression,
                nullptr,
                nullptr);
            if (targetState) {
                applyParsedChangeState(state, fighter, static_cast<int>(std::lround(*targetState)), ctrl, stateDef->animEndSelfState);
            }
        } else if (stateDef && !stateDef->changeStates.empty()) {
            return;
        } else if (!fighter.helper && !(fighter.stateType == 'A' && fighter.physics == 'N')) {
            enterState(state, fighter, 0);
        }
    }
}

void clearHitStatusForRecovery(FighterState& fighter) {
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
    fighter.customHitState = false;
    fighter.customStateOwnerIndex = -1;
    fighter.guarding = false;
    fighter.crouchGuard = false;
}

void enterCommonLandingState(const AppState& state, FighterState& fighter) {
    const int action = firstExistingActionForActor(state, fighter, { 47, 52, 0 });
    fighter.prevStateNo = fighter.stateNo;
    fighter.stateNo = action == 52 ? 52 : (action == 47 ? 47 : 0);
    fighter.stateTime = 0;
    fighter.stateType = 'S';
    fighter.moveType = 'I';
    fighter.physics = 'S';
    fighter.ctrl = true;
    fighter.onGround = true;
    fighter.y = 0.0f;
    fighter.vx = 0.0f;
    fighter.vy = 0.0f;
    clearHitStatusForRecovery(fighter);
    setFighterAction(fighter, action);
}

void updateNeutralAirLandingFallback(const AppState& state, FighterState& fighter) {
    if (fighter.helper
        || fighter.stateType != 'A'
        || fighter.physics != 'N'
        || fighter.moveType != 'I'
        || !fighter.ctrl
        || fighter.y < 0.0f
        || fighter.vy < 0.0f) {
        return;
    }
    enterCommonLandingState(state, fighter);
}

void enterCommonRecoveryLandingState(const AppState& state, FighterState& fighter) {
    const int action = firstExistingActionForActor(state, fighter, { 5170, 47, 52, 0 });
    fighter.prevStateNo = fighter.stateNo;
    fighter.stateNo = action == 5170 ? 5170 : (action == 52 ? 52 : (action == 47 ? 47 : 0));
    fighter.stateTime = 0;
    fighter.stateType = 'S';
    fighter.moveType = 'I';
    fighter.physics = 'S';
    fighter.ctrl = true;
    fighter.onGround = true;
    fighter.y = 0.0f;
    fighter.vx = 0.0f;
    fighter.vy = 0.0f;
    clearHitStatusForRecovery(fighter);
    setFighterAction(fighter, action);
}

void enterDirectCommonRecoveryState(
    const AppState& state,
    FighterState& fighter,
    int stateNo,
    int action,
    char stateType,
    char physics,
    bool ctrl) {
    fighter.prevStateNo = fighter.stateNo;
    fighter.stateNo = stateNo;
    fighter.stateTime = 0;
    fighter.stateType = stateType;
    fighter.moveType = 'I';
    fighter.physics = physics;
    fighter.ctrl = ctrl;
    fighter.onGround = stateType != 'A';
    clearHitStatusForRecovery(fighter);
    setFighterAction(fighter, action);
}

void enterAirRecoveryState(const AppState& state, FighterState& fighter, bool nearGround) {
    const int action = nearGround
        ? firstExistingActionForActor(state, fighter, { 5200, 5140, 5210, 5040, 47, 0 })
        : firstExistingActionForActor(state, fighter, { 5210, 5040, 5140, 5200, 47, 0 });
    if (action == 47 || action == 0) {
        enterCommonLandingState(state, fighter);
        return;
    }
    enterDirectCommonRecoveryState(state, fighter, action, action, 'A', 'A', true);
    fighter.onGround = false;
    fighter.vy = std::min(fighter.vy, nearGround ? -3.5f : -5.5f);
}

void triggerFallEnvShakeIfNeeded(AppState& state, FighterState& target);

#include "FallFallbackRuntime.h"

bool isCommonAirRecoveryState(int stateNo) {
    return stateNo == 5040
        || stateNo == 5140
        || stateNo == 5200
        || stateNo == 5210;
}

void updateCommonAirRecoveryState(const AppState& state, FighterState& fighter) {
    if (!isCommonAirRecoveryState(fighter.stateNo)) {
        return;
    }
    if (fighter.onGround) {
        enterCommonRecoveryLandingState(state, fighter);
    }
}

bool isCommonDizzyAction(int action) {
    return action == 5300 || action == 5301;
}

bool isCommonDizzyStateNo(int stateNo) {
    return stateNo == 2500 || stateNo == 5300 || stateNo == 5301;
}

void updateCommonDizzyState(const AppState& state, FighterState& fighter) {
    const bool dizzyLike = isCommonDizzyStateNo(fighter.stateNo) || isCommonDizzyAction(fighter.action);
    if (!dizzyLike
        || fighter.customHitState
        || fighter.moveType == 'H') {
        return;
    }

    fighter.stateType = 'S';
    fighter.moveType = 'I';
    fighter.physics = 'S';
    fighter.ctrl = false;
    fighter.customHitState = false;
    fighter.onGround = true;
    fighter.y = 0.0f;
    fighter.vx = 0.0f;
    fighter.vy = 0.0f;
    fighter.notHitByTicks = 0;
    fighter.notHitByValue.clear();
    fighter.hitByTicks = 0;
    fighter.hitByValue.clear();

    if (fighter.stateTime >= 200) {
        clearFighterHitRuntime(fighter);
        enterState(state, fighter, 0);
    }
}

void triggerFallEnvShakeIfNeeded(AppState& state, FighterState& target) {
    if (target.hitFallEnvShakePlayed) {
        return;
    }
    target.hitFallEnvShakePlayed = true;
    startEnvShake(state, target.hitFallEnvShake);
}

void updateGroundGetHitState(AppState& state, FighterState& target) {
    if (target.moveType != 'H') {
        return;
    }

    if (resolveTripFallGrounding(state, target)
        || resolveCommonAirFallGrounding(state, target)
        || resolveArenaFallGrounding(state, target)) {
        return;
    }

    if (fallFallbacksEnabled(state) && target.hitFall && target.stateNo == 5100 && fighterAnimationEnded(state, target)) {
        if (std::abs(target.hitFallBounceYVelocity) < 0.05f || !findExactClipForActor(state, target, 5160)) {
            const int action = liedownImpactActionForFighter(state, target);
            if (action == 0) {
                enterState(state, target, 0);
                return;
            }
            enterGroundImpactState(state, target, 5110, action);
            triggerFallEnvShakeIfNeeded(state, target);
            startFallGroundLiedownRecovery(state, target);
            return;
        }

        target.prevStateNo = target.stateNo;
        target.stateNo = 5101;
        target.stateTime = 0;
        target.stateType = 'L';
        target.physics = 'N';
        target.ctrl = false;
        target.onGround = false;
        const CharacterConstants& constants = characterConstantsForActor(state, target);
        target.y = constants.movementDownBounceOffsetY;
        target.x += constants.movementDownBounceOffsetX * static_cast<float>(target.facing);
        target.vx = target.hitFallBounceXVelocity;
        target.vy = target.hitFallBounceYVelocity;
        setFighterAction(target, fallBounceActionForFighter(state, target));
        return;
    }

    if (fallFallbacksEnabled(state)
        && target.hitFall
        && target.stateNo == 5101
        && target.vy > 0.0f
        && target.y >= characterConstantsForActor(state, target).movementDownBounceGroundLevel) {
        target.y = 0.0f;
        target.vy = 0.0f;
        target.onGround = true;
        enterFallGroundImpactIfAvailable(state, target);
        return;
    }

    if (fallFallbacksEnabled(state)
        && target.hitFall
        && (target.stateNo == 5160 || target.stateNo == 5071 || target.stateNo == 5050)
        && target.onGround) {
        enterFallGroundImpactIfAvailable(state, target);
        return;
    }

    if (fallFallbacksEnabled(state)
        && target.hitFall
        && (target.stateNo == 5170 || target.stateNo == 5080)
        && fighterAnimationEnded(state, target)) {
        target.stateNo = 5110;
        target.stateTime = 0;
        target.stateType = 'L';
        target.physics = 'N';
        target.onGround = true;
        target.y = 0.0f;
        setFighterAction(target, liedownRestActionForFighter(state, target));
        return;
    }

    if ((target.hitFall || target.hitDowned) && target.stateNo == 5120) {
        target.stateType = 'S';
        target.physics = 'S';
        target.onGround = true;
        target.vx = 0.0f;
        target.vy = 0.0f;
        if (!fighterAnimationEnded(state, target)) {
            return;
        }
        clearFighterHitRuntime(target);
        enterState(state, target, 0);
        return;
    }

    if (target.hitPauseTicks > 0) {
        target.vx = 0.0f;
        target.vy = 0.0f;
        --target.hitPauseTicks;
        if (target.hitPauseTicks == 0) {
            clampArenaHitFallRuntime(state, target);
            if (target.hitDowned && std::abs(target.hitVelocityY) > 0.05f) {
                target.stateNo = 5030;
                target.stateTime = 0;
                target.stateType = 'A';
                target.physics = 'N';
                target.onGround = false;
                target.vx = target.hitVelocityX;
                target.vy = target.hitVelocityY;
                target.hitAirborne = true;
                setFighterAction(target, firstExistingActionForActor(state, target, { target.sysVars[2], 5090, 5030, target.hitRecoverAnim, 0 }));
                return;
            }
            if (target.hitDowned && !target.hitFall) {
                target.stateNo = 5080;
                target.stateTime = 0;
                target.stateType = 'L';
                target.physics = 'N';
                target.onGround = true;
                target.y = 0.0f;
                target.vx = target.hitVelocityX;
                target.vy = 0.0f;
                setFighterAction(target, firstExistingActionForActor(state, target, { 5080, 5110, 5170, 0 }));
                return;
            }
            const bool enteringTripShake = target.hitFall && target.hitFallTrip;
            target.stateNo = target.hitFall
                ? (enteringTripShake ? 5070 : 5050)
                : (target.hitAirborne ? 5030 : (target.hitCrouch ? 5025 : 5001));
            target.stateTime = 0;
            if (enteringTripShake) {
                target.stateType = 'A';
                target.physics = 'N';
                target.onGround = false;
            } else if (target.hitAirborne || target.hitFall || target.hitVelocityY < 0.0f) {
                target.stateType = 'A';
                target.physics = target.hitFall ? 'N' : 'A';
                target.onGround = false;
            } else {
                target.stateType = target.hitCrouch ? 'C' : 'S';
                target.physics = target.hitCrouch ? 'C' : 'S';
            }
            target.vx = enteringTripShake ? 0.0f : target.hitVelocityX;
            target.vy = enteringTripShake ? 0.0f : target.hitVelocityY;
            const int recoverAction = target.hitFall
                ? (target.hitFallTrip && findExactClipForActor(state, target, 5070) ? 5070 : target.hitFallAirAction)
                : (target.hitAirborne
                    ? firstExistingActionForActor(state, target, { 5030, target.hitRecoverAnim, 5005, 0 })
                    : firstExistingActionForActor(state, target, { target.hitRecoverAnim, 5005, 0 }));
            setFighterAction(target, recoverAction);
        }
        return;
    }

    if (target.hitFall && !target.onGround && !findStateDefinitionForActor(state, target, target.stateNo)) {
        if (target.stateNo == 5050 || target.stateNo == 5071) {
            const CharacterConstants& constants = characterConstantsForActor(state, target);
            target.vy += target.hitFallYAccel > 0.0f ? target.hitFallYAccel : constants.movementYAccel;
        } else if (target.stateNo == 5101 || target.stateNo == 5160) {
            target.vy += characterConstantsForActor(state, target).movementDownBounceYAccel;
        }
    }

    if (target.hitFall && (target.stateNo == 5050 || target.stateNo == 5071)) {
        return;
    }

    if (target.hitFall && (target.stateNo == 5101 || target.stateNo == 5160)) {
        return;
    }

    if (target.hitFall
        && target.stateNo == 5110
        && std::abs(target.vx) < std::max(0.0f, characterConstantsForActor(state, target).movementDownFrictionThreshold)) {
        target.vx = 0.0f;
    }

    const bool groundedHitSlide = !target.hitFall
        && !target.hitAirborne
        && target.onGround
        && (target.physics == 'S' || target.physics == 'C');
    if (target.hitSlideTicks > 0) {
        --target.hitSlideTicks;
        if (groundedHitSlide) {
            applyGroundPhysicsFriction(state, target);
        }
    } else if (groundedHitSlide) {
        target.vx = 0.0f;
    } else {
        target.vx *= 0.6f;
    }

    if (target.hitStunTicks > 0) {
        --target.hitStunTicks;
    }
    if (target.hitAirborne && !target.hitFall) {
        if (target.onGround) {
            enterCommonLandingState(state, target);
            return;
        }
        if (target.hitStunTicks <= 0) {
            const int action = firstExistingActionForActor(state, target, { 5040, 5210, 5140, 47, 0 });
            if (action == 47 || action == 0) {
                enterCommonLandingState(state, target);
            } else {
                enterDirectCommonRecoveryState(state, target, action, action, 'A', 'A', true);
                target.onGround = false;
            }
            return;
        }
    }
    if (target.hitFall && target.stateNo != 5110) {
        return;
    }
    if (target.hitStunTicks <= 0) {
        target.vx = 0.0f;
        target.vy = 0.0f;
        target.hitVelocityX = 0.0f;
        target.hitVelocityY = 0.0f;
        target.hitSlideTicks = 0;
        if ((target.hitFall || target.hitDowned) && target.hitDownRecover && findExactClipForActor(state, target, 5120)) {
            target.stateNo = 5120;
            target.stateTime = 0;
            target.stateType = 'S';
            target.physics = 'S';
            target.ctrl = false;
            target.onGround = true;
            setFighterAction(target, getUpActionForFighter(state, target));
            return;
        }
        clearFighterHitRuntime(target);
        enterState(state, target, 0);
    }
}

void updateGroundGuardState(const AppState& state, FighterState& target) {
    if (!target.guarding) {
        return;
    }

    const bool crouch = target.crouchGuard;
    target.stateType = crouch ? 'C' : 'S';
    target.moveType = 'H';
    target.ctrl = false;
    target.onGround = true;

    if (target.hitPauseTicks > 0) {
        target.stateNo = crouch ? 152 : 150;
        target.physics = 'N';
        target.vx = 0.0f;
        target.vy = 0.0f;
        --target.hitPauseTicks;
        if (target.hitPauseTicks == 0) {
            target.stateNo = crouch ? 153 : 151;
            target.stateTime = 0;
            target.physics = crouch ? 'C' : 'S';
            target.vx = target.hitVelocityX;
            target.vy = target.hitVelocityY;
        }
        return;
    }

    target.stateNo = crouch ? 153 : 151;
    target.physics = crouch ? 'C' : 'S';
    if (target.hitSlideTicks > 0) {
        --target.hitSlideTicks;
        applyGroundPhysicsFriction(state, target);
    } else {
        target.vx = 0.0f;
        target.vy = 0.0f;
    }

    if (target.hitStunTicks > 0) {
        --target.hitStunTicks;
    }
    if (target.hitStunTicks <= 0) {
        clearFighterHitRuntime(target);
        enterGroundGuardReadyState(state, target, crouch ? GuardStance::Crouch : GuardStance::Stand);
    }
}

constexpr int kJumpInputBufferTicks = 18;

void updateJumpInputBuffer(
    FighterState& fighter,
    const FighterInputState& input,
    bool jumpPressedThisFrame,
    bool commandButtonHeld) {
    if (fighter.guarding || fighter.moveType == 'H') {
        fighter.jumpInputBufferTicks = 0;
        if (!input.up) {
            fighter.jumpInputConsumedWhileHeld = false;
        }
        return;
    }

    if (!input.up) {
        fighter.jumpInputConsumedWhileHeld = false;
        if (fighter.jumpInputBufferTicks > 0) {
            --fighter.jumpInputBufferTicks;
        }
        return;
    }

    if (jumpPressedThisFrame) {
        fighter.jumpInputBufferTicks = kJumpInputBufferTicks;
        fighter.jumpInputConsumedWhileHeld = false;
        return;
    }

    const bool waitingForActionableGround =
        !fighter.ctrl
        || !fighter.onGround
        || fighter.moveType != 'I'
        || fighter.stateNo != 0
        || fighter.hitPauseTicks > 0
        || fighter.guarding;
    if (!fighter.jumpInputConsumedWhileHeld && (waitingForActionableGround || commandButtonHeld)) {
        fighter.jumpInputBufferTicks = kJumpInputBufferTicks;
        return;
    }

    if (fighter.jumpInputBufferTicks > 0) {
        --fighter.jumpInputBufferTicks;
    }
}

void consumeJumpInputBuffer(FighterState& fighter) {
    fighter.jumpInputBufferTicks = 0;
    fighter.jumpInputConsumedWhileHeld = true;
}

void updateControlledFighter(
    AppState& state,
    FighterState& fighter,
    const FighterState* opponent,
    const FighterInputState& input,
    const CommandStateEntry* preferredCommandEntry = nullptr) {
    FramePerfScope scope(state.framePerf, FramePerfSection::FighterUpdate);
    FighterInputState commandInput = input;
    const bool arenaDepth = arenaDepthActive(state);
    if (!arenaDepth) {
        fighter.arenaDepthModifierHeld = false;
        fighter.arenaDepthModifierLastTapFrame = -100000;
        fighter.arenaDepthSidestepTicks = 0;
        fighter.arenaDepthSidestepVelocity = 0.0f;
    }
    const bool depthModifierPressedThisFrame = arenaDepth && input.depthModifier && !fighter.arenaDepthModifierHeld;
    fighter.arenaDepthModifierHeld = arenaDepth && input.depthModifier;
    const bool depthInputActive = arenaDepth && input.depthModifier;
    if (depthInputActive) {
        commandInput.up = false;
        commandInput.down = false;
    }
    pushFighterInputFrame(fighter, commandInput, state.frame);
    const bool attackButtonHeld = input.s || input.x || input.y || input.z || input.a || input.b || input.c;
    const bool jumpPressedThisFrame = commandInput.up && !previousFighterInputHeldUp(fighter);
    updateJumpInputBuffer(fighter, commandInput, jumpPressedThisFrame, attackButtonHeld);

    if (fighter.guarding) {
        updateGroundGuardState(state, fighter);
        return;
    }

    if (fighter.moveType == 'H' && !fighter.customHitState) {
        const auto commands = collectFighterCommands(commandInput, fighter, commandDefinitionsForActor(state, fighter));
        if (!applyHitCommandState(state, fighter, opponent, commands)) {
            updateGroundGetHitState(state, fighter);
        }
        return;
    }

    if (fighter.hitPauseTicks > 0) {
        fighter.vx = 0.0f;
        --fighter.hitPauseTicks;
        return;
    }

    if (fighter.moveType == 'H' && fighter.customHitState) {
        return;
    }

    if (updateGroundGuardInputState(state, fighter, commandInput)) {
        return;
    }

    updateStateZeroFromMovement(state, fighter);
    const bool holdingDown = commandInput.down && fighter.onGround;
    updatePlayerCrouchInput(state, fighter, holdingDown);
    const bool holdingHorizontal = commandInput.left != commandInput.right;
    const bool holdingUp = commandInput.up;
    const bool movementLocked = fighterHasAssertSpecialFlag(fighter, "nowalk");
    const int heldWalkAction = ((fighter.facing >= 0 && input.right) || (fighter.facing < 0 && input.left)) ? 20 : 21;
    const auto startFallbackJump = [&state, &fighter, &input]() {
        consumeJumpInputBuffer(fighter);
        const CharacterConstants& constants = characterConstantsForActor(state, fighter);
        const bool holdingForward = (fighter.facing >= 0 && input.right) || (fighter.facing < 0 && input.left);
        const bool holdingBack = (fighter.facing >= 0 && input.left) || (fighter.facing < 0 && input.right);
        const int localDirection = holdingForward == holdingBack ? 0 : (holdingForward ? 1 : -1);
        const float localVelocityX = localDirection == 0
            ? constants.velocityJumpNeuX
            : (localDirection > 0 ? constants.velocityJumpFwdX : constants.velocityJumpBackX);
        fighter.vx = localVelocityX * static_cast<float>(fighter.facing);
        fighter.vy = constants.velocityJumpY;
        if (findStateDefinitionForActor(state, fighter, 50)) {
            enterState(state, fighter, 50);
            fighter.vx = localVelocityX * static_cast<float>(fighter.facing);
            fighter.vy = constants.velocityJumpY;
        }
        fighter.onGround = false;
        fighter.stateType = 'A';
        fighter.physics = 'A';
        fighter.jumpBaseAction =
            localDirection == 0 ? 41 : (localDirection > 0 ? 42 : 43);
        fighter.jumpPeakActionApplied = false;
    };

    if (arenaDepth) {
        const bool canMoveDepth = fighter.ctrl && !movementLocked && fighter.onGround;
        if (!canMoveDepth) {
            fighter.depthVz = 0.0f;
            fighter.arenaDepthSidestepTicks = 0;
            fighter.arenaDepthSidestepVelocity = 0.0f;
        } else {
            if (depthModifierPressedThisFrame) {
                const bool doubleTap =
                    state.frame - fighter.arenaDepthModifierLastTapFrame <= state.arenaConfig.depthModifierDoubleTapFrames;
                if (doubleTap) {
                    int direction = input.down != input.up
                        ? (input.down ? 1 : -1)
                        : (fighter.arenaDepthSidestepDirection >= 0 ? 1 : -1);
                    if (fighter.depthZ >= state.arenaConfig.depthMax - 0.5f) {
                        direction = -1;
                    } else if (fighter.depthZ <= state.arenaConfig.depthMin + 0.5f) {
                        direction = 1;
                    }
                    fighter.arenaDepthSidestepTicks = state.arenaConfig.depthSidestepFrames;
                    fighter.arenaDepthSidestepVelocity =
                        static_cast<float>(direction) * state.arenaConfig.depthSidestepDistance
                        / static_cast<float>(std::max(1, state.arenaConfig.depthSidestepFrames));
                    fighter.arenaDepthSidestepDirection = -direction;
                }
                fighter.arenaDepthModifierLastTapFrame = state.frame;
            }

            if (fighter.arenaDepthSidestepTicks > 0) {
                fighter.depthVz = fighter.arenaDepthSidestepVelocity;
                --fighter.arenaDepthSidestepTicks;
            } else if (input.depthModifier && input.up != input.down) {
                fighter.depthVz = input.down ? state.arenaConfig.depthMoveSpeed : -state.arenaConfig.depthMoveSpeed;
            } else {
                fighter.depthVz = 0.0f;
            }
        }
        if (fighter.stateNo == 0) {
            updateStateZeroFromMovement(state, fighter);
        }
    }

    if (preferredCommandEntry) {
        applyCommandEntryDemoRuntimePrereqs(fighter, *preferredCommandEntry);
    }
    const auto commands = collectFighterCommands(commandInput, fighter, commandDefinitionsForActor(state, fighter));
    const bool changedStateFromCommand = preferredCommandEntry
        ? applyPreferredCommandState(state, fighter, opponent, commands, *preferredCommandEntry)
        : applyCommandState(state, fighter, opponent, commands);

    if (!changedStateFromCommand && fighter.stateNo == 20 && (!holdingHorizontal || holdingDown || holdingUp || !fighter.ctrl)) {
        enterState(state, fighter, 0);
    } else if (!changedStateFromCommand && fighter.stateNo == 20 && holdingHorizontal) {
        if (findExactClipForActor(state, fighter, heldWalkAction)) {
            setFighterAction(fighter, heldWalkAction);
        }
    }

    if (fighter.stateNo == 0) {
        if (fighter.onGround) {
            fighter.vx = 0.0f;
            fighter.jumpBaseAction = 0;
            fighter.jumpPeakActionApplied = false;
            if (!changedStateFromCommand && !movementLocked && !holdingDown && !holdingUp && fighter.ctrl && holdingHorizontal) {
                if (findStateDefinitionForActor(state, fighter, 20)) {
                    enterState(state, fighter, 20);
                    if (findExactClipForActor(state, fighter, heldWalkAction)) {
                        setFighterAction(fighter, heldWalkAction);
                    }
                } else {
                    const CharacterConstants& constants = characterConstantsForActor(state, fighter);
                    const bool movingForward = (fighter.facing >= 0 && input.right) || (fighter.facing < 0 && input.left);
                    const float localVelocity = movingForward
                        ? constants.velocityWalkFwdX
                        : constants.velocityWalkBackX;
                    fighter.vx = localVelocity * static_cast<float>(fighter.facing);
                }
            }
        }
        if (!changedStateFromCommand
            && !movementLocked
            && !holdingDown
            && fighter.ctrl
            && !attackButtonHeld
            && fighter.jumpInputBufferTicks > 0
            && fighter.onGround) {
            startFallbackJump();
        }
    }
}

void updateTrainingDummy(AppState& state, FighterState& dummy) {
    if (state.training.options.dummyFrozen) {
        if (dummy.moveType == 'H' || dummy.guarding) {
            clearFighterHitRuntime(dummy);
            enterState(state, dummy, 0);
        }
        dummy.vx = 0.0f;
        dummy.vy = 0.0f;
        dummy.y = 0.0f;
        dummy.onGround = true;
    } else if (dummy.guarding) {
        updateGroundGuardState(state, dummy);
    } else if (dummy.moveType == 'H' && dummy.customHitState) {
        return;
    } else if (dummy.moveType == 'H') {
        updateGroundGetHitState(state, dummy);
    } else {
        dummy.vx = 0.0f;
        dummy.vy = 0.0f;
        dummy.y = 0.0f;
        dummy.onGround = true;
        const GuardStance idleGuard = dummyGuardIdleStance(state.training.options.dummyGuardMode);
        if (idleGuard != GuardStance::None) {
            if (!isGroundGuardCommonState(dummy.stateNo)
                || guardStanceFromCommonState(dummy) != idleGuard
                || dummy.stateNo == guardEndStateNo()) {
                enterGroundGuardReadyState(state, dummy, idleGuard);
            } else {
                updateGroundGuardReadyState(state, dummy);
            }
        } else if (isGroundGuardCommonState(dummy.stateNo)) {
            if (dummy.stateNo == guardEndStateNo()) {
                updateGroundGuardReadyState(state, dummy);
            } else {
                enterGroundGuardEndState(state, dummy);
            }
        }
    }
}

bool cpuCanChooseInput(const FighterState& cpu) {
    return cpu.ctrl
        && cpu.onGround
        && cpu.stateType != 'A'
        && cpu.moveType != 'H'
        && !cpu.guarding
        && cpu.hitPauseTicks <= 0;
}

void holdBackInput(FighterInputState& input, const FighterState& fighter) {
    if (fighter.facing >= 0) {
        input.left = true;
    } else {
        input.right = true;
    }
}

void holdTowardTargetInput(FighterInputState& input, const FighterState& fighter, const FighterState& target) {
    if (target.x < fighter.x) {
        input.left = true;
    } else if (target.x > fighter.x) {
        input.right = true;
    } else if (fighter.facing >= 0) {
        input.right = true;
    } else {
        input.left = true;
    }
}

FighterInputState buildCpuOpponentInput(const AppState& state, const FighterState& cpu, const FighterState& target) {
    FighterInputState input;
    if (!cpuCanChooseInput(cpu)) {
        return input;
    }

    const float distance = std::fabs(target.x - cpu.x);
    constexpr float guardDistance = 56.0f;
    constexpr float approachDistance = 48.0f;
    if (arenaDepthActive(state)) {
        const float depthDelta = target.depthZ - cpu.depthZ;
        const float depthAlignTolerance = std::max(2.0f, state.arenaConfig.fighterDepthHitTolerance * 0.5f);
        if (std::fabs(depthDelta) > depthAlignTolerance) {
            input.depthModifier = true;
            if (depthDelta > 0.0f) {
                input.down = true;
            } else {
                input.up = true;
            }
            return input;
        }
    }

    if (target.moveType == 'A' && distance <= guardDistance) {
        holdBackInput(input, cpu);
    } else if (distance > approachDistance) {
        holdTowardTargetInput(input, cpu, target);
    } else if (state.frame % 45 < 2) {
        input.x = true;
    }
    return input;
}

void updateCpuOpponent(AppState& state, FighterState& opponent, const FighterState& target) {
    const FighterInputState input = buildCpuOpponentInput(state, opponent, target);
    updateControlledFighter(state, opponent, &target, input);
}

bool trainingCommandDemoActive(const AppState& state);
const CommandStateEntry* selectedTrainingCommandEntry(const AppState& state, int* selectedIndex);
FighterInputState nextTrainingCommandDemoInput(AppState& state, FighterState& demoFighter);
void updateTrainingCommandPracticeTimers(AppState& state);
void updateTrainingCommandPracticeProgress(
    AppState& state,
    const FighterState& fighterBeforeUpdate,
    const FighterState& fighterAfterUpdate,
    const FighterState* opponent);

