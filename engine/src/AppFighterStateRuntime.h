#pragma once

// Internal App.cpp implementation shard.
// Fighter common-state, physics, variable, and pause runtime helpers.

bool isCrouchStateNo(int stateNo) {
    return stateNo == 10 || stateNo == 11 || stateNo == 12;
}

bool fighterAnimationEnded(const AppState& state, const FighterState& fighter) {
    const AnimationClip* clip = findExactClipForActor(state, fighter, fighter.action);
    return clip && !clip->hasInfiniteDuration && !clip->hasLoopStart && fighter.animTick >= clip->loopTicks;
}

void enterCrouchState(const AppState& state, FighterState& fighter, int stateNo) {
    fighter.guarding = false;
    fighter.crouchGuard = false;
    fighter.stateNo = stateNo;
    fighter.stateTime = 0;
    fighter.appliedHitDefIds.clear();
    clearStateRuntimeControllerTracking(fighter);
    fighter.moveContact = false;
    fighter.moveHit = false;
    fighter.moveGuarded = false;
    fighter.stateType = stateNo == 12 ? 'S' : 'C';
    fighter.moveType = 'I';
    fighter.physics = stateNo == 12 ? 'S' : 'C';
    fighter.ctrl = true;
    fighter.onGround = true;
    fighter.vx = 0.0f;
    fighter.vy = 0.0f;
    fighter.hitPauseTicks = 0;
    fighter.hitStunTicks = 0;
    fighter.hitSlideTicks = 0;
    setFighterAction(fighter, findExactClipForActor(state, fighter, stateNo) ? stateNo : 0);
}

void updatePlayerCrouchInput(const AppState& state, FighterState& fighter, bool holdingDown) {
    if (!fighter.onGround || fighter.moveType == 'H' || fighter.guarding) {
        return;
    }

    if (holdingDown) {
        if (fighter.stateNo == 0 || fighter.stateNo == 12) {
            enterCrouchState(state, fighter, 10);
        } else if (fighter.stateNo == 10 && fighterAnimationEnded(state, fighter)) {
            enterCrouchState(state, fighter, 11);
        }
    } else if (fighter.stateNo == 10 || fighter.stateNo == 11) {
        enterCrouchState(state, fighter, 12);
    } else if (fighter.stateNo == 12 && fighterAnimationEnded(state, fighter)) {
        enterState(state, fighter, 0);
    }
}

bool isTripFallImpactChange(int previousStateNo, int targetState) {
    return (previousStateNo == 5070 || previousStateNo == 5071)
        && (targetState == 5100 || targetState == 5110 || targetState == 5170);
}

int liedownRecoveryTicks(const AppState& state, const FighterState& fighter) {
    if (fighter.hitDownRecoverTime >= 0) {
        return std::max(1, fighter.hitDownRecoverTime);
    }
    return std::max(1, characterConstantsForActor(state, fighter).liedownTime);
}

void applyGroundPhysicsFriction(const AppState& state, FighterState& fighter) {
    const CharacterConstants& constants = characterConstantsForActor(state, fighter);
    const bool crouching = fighter.physics == 'C' || fighter.stateType == 'C';
    const float friction = crouching ? constants.movementCrouchFriction : constants.movementStandFriction;
    const float threshold = std::max(0.0f, crouching
        ? constants.movementCrouchFrictionThreshold
        : constants.movementStandFrictionThreshold);
    if (std::abs(fighter.vx) < threshold) {
        fighter.vx = 0.0f;
        return;
    }
    fighter.vx *= friction;
}

void startFallGroundLiedownRecovery(const AppState& state, FighterState& fighter) {
    fighter.hitStunTicks = std::max(fighter.hitStunTicks, liedownRecoveryTicks(state, fighter));
}

void restoreTripFallImpactRuntime(const AppState& state, FighterState& fighter, int previousStateNo, int targetState) {
    if (!fighter.hitFall || !fighter.hitFallTrip || !isTripFallImpactChange(previousStateNo, targetState)) {
        return;
    }
    fighter.y = 0.0f;
    fighter.vy = 0.0f;
    fighter.onGround = true;
    startFallGroundLiedownRecovery(state, fighter);
}

void applyParsedChangeState(const AppState& state, FighterState& fighter, int targetState, std::optional<bool> ctrl, bool selfState = false) {
    const int previousStateNo = fighter.stateNo;
    const bool wasCustomState = fighter.customHitState && fighter.customStateOwnerIndex >= 0;
    const int previousCustomOwnerIndex = fighter.customStateOwnerIndex;
    if (selfState) {
        fighter.customStateOwnerIndex = -1;
        fighter.customHitState = false;
    }
    bool changed = true;
    if (isCrouchStateNo(targetState)) {
        enterCrouchState(state, fighter, targetState);
    } else {
        changed = enterState(state, fighter, targetState);
    }
    if (!changed && selfState) {
        fighter.customStateOwnerIndex = previousCustomOwnerIndex;
        fighter.customHitState = wasCustomState;
        return;
    }
    if (!selfState && wasCustomState) {
        fighter.customStateOwnerIndex = previousCustomOwnerIndex;
        fighter.customHitState = true;
    }
    restoreTripFallImpactRuntime(state, fighter, previousStateNo, targetState);
    if (ctrl) {
        fighter.ctrl = *ctrl;
    }
}

bool stateControllerTriggerActive(
    const AppState& state,
    const FighterState& fighter,
    const StateControllerTrigger& trigger,
    const FighterState* opponent,
    const StageSlot* stage);
bool simpleControllerTriggerActive(const AppState& state, const FighterState& fighter, int triggerTime, int triggerAnimElem);
int stateControllerDomainKey(int domain, int controllerId);
bool shouldRunStateRuntimeController(
    const AppState& state,
    FighterState& fighter,
    int controllerId,
    const StateControllerTrigger& trigger,
    const FighterState* opponent,
    const StageSlot* stage);
bool shouldRunSimpleStateRuntimeController(
    const AppState& state,
    FighterState& fighter,
    int controllerId,
    const StateControllerTrigger& trigger,
    int triggerTime,
    int triggerAnimElem);

bool stateCtrlAlreadyFired(const FighterState& fighter, int ctrlControllerId) {
    return std::find(
        fighter.firedStateCtrlControllerIds.begin(),
        fighter.firedStateCtrlControllerIds.end(),
        ctrlControllerId) != fighter.firedStateCtrlControllerIds.end();
}

void markStateCtrlFired(FighterState& fighter, int ctrlControllerId) {
    if (!stateCtrlAlreadyFired(fighter, ctrlControllerId)) {
        fighter.firedStateCtrlControllerIds.push_back(ctrlControllerId);
    }
}

void updateStateCtrlControllers(AppState& state, FighterState& fighter) {
    forEachRuntimeControllerStateDefinition(state, fighter, [&](const StateDefinition& stateDef) {
        for (const auto& ctrl : stateDef.ctrlSets) {
            const int controllerKey = stateControllerDomainKey(2, ctrl.id);
            const bool shouldRun = ctrl.trigger.hasTrigger
                ? shouldRunStateRuntimeController(state, fighter, controllerKey, ctrl.trigger, nullptr, nullptr)
                : shouldRunSimpleStateRuntimeController(
                    state,
                    fighter,
                    controllerKey,
                    ctrl.trigger,
                    ctrl.triggerTime,
                    ctrl.triggerAnimElem);
            if (!shouldRun) {
                continue;
            }
            fighter.ctrl = ctrl.value;
        }
        return true;
    });
}

bool statePosAddAlreadyFired(const FighterState& fighter, int posAddControllerId) {
    return std::find(
        fighter.firedStatePosAddControllerIds.begin(),
        fighter.firedStatePosAddControllerIds.end(),
        posAddControllerId) != fighter.firedStatePosAddControllerIds.end();
}

void markStatePosAddFired(FighterState& fighter, int posAddControllerId) {
    if (!statePosAddAlreadyFired(fighter, posAddControllerId)) {
        fighter.firedStatePosAddControllerIds.push_back(posAddControllerId);
    }
}

bool stateChangeAnimAlreadyFired(const FighterState& fighter, int changeAnimControllerId) {
    return std::find(
        fighter.firedStateChangeAnimControllerIds.begin(),
        fighter.firedStateChangeAnimControllerIds.end(),
        changeAnimControllerId) != fighter.firedStateChangeAnimControllerIds.end();
}

void markStateChangeAnimFired(FighterState& fighter, int changeAnimControllerId) {
    if (!stateChangeAnimAlreadyFired(fighter, changeAnimControllerId)) {
        fighter.firedStateChangeAnimControllerIds.push_back(changeAnimControllerId);
    }
}

#include "RuntimeExpressionEvaluation.h"

void setFighterVariableValue(FighterState& fighter, const MugenVariableRef& ref, float value) {
    if (!variableRefInRange(ref)) {
        return;
    }
    switch (ref.bank) {
    case MugenVariableBank::Var:
        fighter.vars[static_cast<size_t>(ref.index)] = static_cast<int>(std::lround(value));
        break;
    case MugenVariableBank::SysVar:
        fighter.sysVars[static_cast<size_t>(ref.index)] = static_cast<int>(std::lround(value));
        break;
    case MugenVariableBank::FVar:
        fighter.fvars[static_cast<size_t>(ref.index)] = value;
        break;
    case MugenVariableBank::SysFVar:
        fighter.sysFvars[static_cast<size_t>(ref.index)] = value;
        break;
    default:
        break;
    }
}

int fighterIndexInState(const AppState& state, const FighterState& fighter) {
    const auto* first = state.fighters.data();
    const auto* current = &fighter;
    if (current < first || current >= first + state.fighters.size()) {
        return -1;
    }
    return static_cast<int>(current - first);
}

#include "StateControllerUtilityRuntime.h"

bool globalPauseActive(const AppState& state) {
    return state.globalPauseTicks > 0;
}

bool fighterCanUpdateDuringGlobalPause(const AppState& state, int fighterIndex) {
    if (!globalPauseActive(state)) {
        return true;
    }
    return fighterIndex == state.globalPauseOwnerIndex && state.globalPauseOwnerMoveTicks > 0;
}

void startGlobalPause(AppState& state, FighterState& fighter, const StatePauseController& pause) {
    const int ownerIndex = fighter.helper ? fighter.ownerIndex : fighterIndexInState(state, fighter);
    const int effectivePauseTicks = pause.time > 0 ? pause.time + 1 : 0;
    const int effectiveMoveTicks = pause.moveTime > 0 ? pause.moveTime + 1 : pause.moveTime;
    state.globalPauseTicks = std::max(state.globalPauseTicks, effectivePauseTicks);
    state.globalPauseOwnerIndex = ownerIndex;
    state.globalPauseOwnerMoveTicks = std::max(state.globalPauseOwnerMoveTicks, effectiveMoveTicks);
    state.globalPauseIsSuper = pause.superPause;
    if (pause.powerAdd != 0) {
        fighter.power = std::clamp(
            fighter.power + pause.powerAdd,
            0,
            std::max(0, characterConstantsForActor(state, fighter).maxPower));
    }
    if (pause.soundGroup >= 0 && pause.soundIndex >= 0) {
        playSound(state, pause.soundGroup, pause.soundIndex, pause.soundForceCommon, -1, false, 1.0f, false, ownerIndex);
    }
}

void updateGlobalPauseTimers(AppState& state) {
    if (state.globalPauseTicks > 0) {
        --state.globalPauseTicks;
    }
    if (state.globalPauseOwnerMoveTicks > 0) {
        --state.globalPauseOwnerMoveTicks;
    }
    if (state.globalPauseTicks <= 0) {
        state.globalPauseTicks = 0;
        state.globalPauseOwnerIndex = -1;
        state.globalPauseOwnerMoveTicks = 0;
        state.globalPauseIsSuper = false;
    }
}

#include "StateControllerVariableRuntime.h"
