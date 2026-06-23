#pragma once

// Internal App.cpp implementation shard.
// State-controller runtime execution helpers for meters, movement, helpers, and effects.

#include "StateControllerPowerRuntime.h"

void updateStateMeterControllers(
    AppState& state,
    FighterState& fighter,
    const FighterState* opponent = nullptr,
    const StageSlot* stage = nullptr) {
    forEachRuntimeControllerStateDefinition(state, fighter, [&](const StateDefinition& stateDef) {
        updateStatePowerControllersForDefinition(state, fighter, stateDef, opponent, stage);

        for (const auto& lifeAdd : stateDef.lifeAdds) {
            if (!shouldRunStateRuntimeController(state, fighter, lifeAdd.id, lifeAdd.trigger, opponent, stage)) {
                continue;
            }
            const auto value = evalMugenExpression(state, fighter, lifeAdd.valueExpression, opponent, stage);
            if (!value) {
                continue;
            }
            const int minimumLife = lifeAdd.kill ? 0 : 1;
            fighter.life = std::clamp(
                fighter.life + static_cast<int>(std::lround(*value)),
                minimumLife,
                characterMaxLifeForActor(state, fighter));
        }

        for (const auto& hitAdd : stateDef.hitAdds) {
            if (!shouldRunStateRuntimeController(state, fighter, hitAdd.id, hitAdd.trigger, opponent, stage)) {
                continue;
            }
            const auto value = evalMugenExpression(state, fighter, hitAdd.valueExpression, opponent, stage);
            if (!value) {
                continue;
            }
            fighter.hitCount = std::max(0, fighter.hitCount + static_cast<int>(std::lround(*value)));
        }

        for (const auto& attackDist : stateDef.attackDists) {
            if (!shouldRunStateRuntimeController(state, fighter, attackDist.id, attackDist.trigger, opponent, stage)) {
                continue;
            }
            const auto value = evalMugenExpression(state, fighter, attackDist.valueExpression, opponent, stage);
            if (!value) {
                continue;
            }
            fighter.attackDistanceOverride = std::max(0, static_cast<int>(std::lround(*value)));
        }

        for (const auto& defenceMulSet : stateDef.defenceMulSets) {
            if (!shouldRunStateRuntimeController(state, fighter, defenceMulSet.id, defenceMulSet.trigger, opponent, stage)) {
                continue;
            }
            const auto value = evalMugenExpression(state, fighter, defenceMulSet.valueExpression, opponent, stage);
            if (!value) {
                continue;
            }
            fighter.defenceMultiplier = std::max(0.001f, *value);
        }

        for (const auto& attackMulSet : stateDef.attackMulSets) {
            if (!shouldRunStateRuntimeController(state, fighter, attackMulSet.id, attackMulSet.trigger, opponent, stage)) {
                continue;
            }
            const auto value = evalMugenExpression(state, fighter, attackMulSet.valueExpression, opponent, stage);
            if (!value) {
                continue;
            }
            fighter.attackMultiplier = std::max(0.0f, *value);
        }
        return true;
    });
}

#include "StateControllerPosAddRuntime.h"

void updateStatePosAddControllers(
    AppState& state,
    FighterState& fighter,
    const FighterState* opponent = nullptr,
    const StageSlot* stage = nullptr) {
    forEachRuntimeControllerStateDefinition(state, fighter, [&](const StateDefinition& runtimeStateDef) {
        updateStatePosAddControllersForDefinition(state, fighter, runtimeStateDef, opponent, stage);
        return true;
    });
}

void updateStateChangeAnimControllers(
    AppState& state,
    FighterState& fighter,
    const FighterState* opponent = nullptr,
    const StageSlot* stage = nullptr) {
    forEachRuntimeControllerStateDefinition(state, fighter, [&](const StateDefinition& runtimeStateDef) {
        const StateDefinition* stateDef = &runtimeStateDef;

    for (const auto& changeAnim : stateDef->changeAnims) {
        if (changeAnim.trigger.hasTrigger) {
            if (!shouldRunStateRuntimeController(state, fighter, changeAnim.id, changeAnim.trigger, opponent, stage)) {
                continue;
            }
        } else {
            if (stateRuntimeControllerBlockedByHitPause(
                    fighter,
                    stateControllerDomainKey(4, changeAnim.id),
                    changeAnim.trigger)
                || !changeAnimTriggerActive(state, fighter, changeAnim)
                || !stateRuntimeControllerPersistenceAllowsRun(
                    fighter,
                    stateControllerDomainKey(4, changeAnim.id),
                    changeAnim.trigger)) {
                continue;
            }
        }
        int action = changeAnim.value;
        int elem = changeAnim.elem;
        if (!trim(changeAnim.valueExpression).empty()) {
            if (const auto value = evalMugenExpression(state, fighter, changeAnim.valueExpression, opponent, stage)) {
                action = static_cast<int>(std::lround(*value));
            }
        }
        if (!trim(changeAnim.elemExpression).empty()) {
            if (const auto value = evalMugenExpression(state, fighter, changeAnim.elemExpression, opponent, stage)) {
                elem = static_cast<int>(std::lround(*value));
            }
        }
        const int selfOwnerIndex = fighter.helper ? fighter.ownerIndex : fighterIndexInState(state, fighter);
        const int customOwnerIndex = fighter.customStateOwnerIndex >= 0
            && fighter.customStateOwnerIndex < static_cast<int>(state.fighters.size())
            ? fighter.customStateOwnerIndex
            : selfOwnerIndex;
        const int actionOwnerIndex = changeAnim.useCustomStateOwnerAnimation ? customOwnerIndex : selfOwnerIndex;
        if (setFighterActionElementWithOwner(state, fighter, action, elem, actionOwnerIndex)) {
            continue;
        }

        if (!changeAnim.useCustomStateOwnerAnimation || selfOwnerIndex < 0) {
            continue;
        }

        std::array<int, 4> customThrowFallbacks{ 0, 0, 0, 0 };
        if (action == 950) {
            customThrowFallbacks = { 850, 840, 0, 0 };
        } else if (action == 960) {
            customThrowFallbacks = { 840, 850, 0, 0 };
        } else if (action >= 900 && action < 1000) {
            customThrowFallbacks = { action - 100, action - 110, 850, 840 };
        }
        if (customOwnerIndex >= 0) {
            bool usedCustomThrowFallback = false;
            for (const int fallbackAction : customThrowFallbacks) {
                if (fallbackAction <= 0) {
                    continue;
                }
                if (setFighterActionElementWithOwner(state, fighter, fallbackAction, 1, customOwnerIndex)) {
                    usedCustomThrowFallback = true;
                    break;
                }
            }
            if (usedCustomThrowFallback) {
                continue;
            }
        }

        const std::array<int, 6> airFallbacks{ 5030, 5050, 5060, 5070, 5100, 5000 };
        const std::array<int, 6> groundFallbacks{ 5000, 5010, 5020, 5030, 5050, 0 };
        const auto& fallbacks = (fighter.stateType == 'A' || !fighter.onGround) ? airFallbacks : groundFallbacks;
        for (const int fallbackAction : fallbacks) {
            if (fallbackAction == 0) {
                break;
            }
            if (setFighterActionElementWithOwner(state, fighter, fallbackAction, 1, selfOwnerIndex)) {
                break;
            }
        }
    }
        return true;
    });
}

#include "StateControllerVelocityRuntime.h"
#include "StateControllerPosSetRuntime.h"
#include "StateControllerSprPriorityRuntime.h"
#include "StateControllerPosFreezeRuntime.h"
#include "StateControllerTurnRuntime.h"

void updateStateMovementControllers(
    AppState& state,
    FighterState& fighter,
    const FighterState* opponent = nullptr,
    const StageSlot* stage = nullptr) {
    updateStateVariableControllers(state, fighter, opponent, stage);
    updateStateMeterControllers(state, fighter, opponent, stage);
    updateStateVisualControllers(state, fighter, opponent, stage);

    forEachRuntimeControllerStateDefinition(state, fighter, [&](const StateDefinition& runtimeStateDef) {
        const StateDefinition* stateDef = &runtimeStateDef;

    updateStateSprPriorityControllersForDefinition(state, fighter, *stateDef, opponent, stage);

    updateStatePosFreezeControllersForDefinition(state, fighter, *stateDef, opponent, stage);

    updateStateTurnControllersForDefinition(state, fighter, *stateDef, opponent, stage);

    for (const auto& pause : stateDef->pauses) {
        if (!shouldRunStatePauseController(state, fighter, pause.id, pause.trigger, opponent, stage)) {
            continue;
        }
        startGlobalPause(state, fighter, pause);
    }

    for (const auto& forceFeedback : stateDef->forceFeedbacks) {
        if (!shouldRunStateRuntimeController(state, fighter, forceFeedback.id, forceFeedback.trigger, opponent, stage)) {
            continue;
        }
        runForceFeedbackController(state, fighter, forceFeedback);
    }

    for (const auto& envShake : stateDef->envShakes) {
        if (!shouldRunStateRuntimeController(state, fighter, envShake.id, envShake.trigger, opponent, stage)) {
            continue;
        }
        startEnvShake(state, envShake.shake);
    }

    for (const auto& fallEnvShake : stateDef->fallEnvShakes) {
        if (!shouldRunStateRuntimeController(state, fighter, fallEnvShake.id, fallEnvShake.trigger, opponent, stage)) {
            continue;
        }
        if (!fighter.hitFallEnvShakePlayed) {
            fighter.hitFallEnvShakePlayed = true;
            startEnvShake(state, fighter.hitFallEnvShake);
        }
    }

    for (const auto& paletteEffect : stateDef->paletteEffects) {
        if (!shouldRunStateRuntimeController(state, fighter, paletteEffect.id, paletteEffect.trigger, opponent, stage)) {
            continue;
        }
        if (paletteEffect.background) {
            startPaletteEffect(state.backgroundPaletteEffect, paletteEffect.effect);
        } else {
            startPaletteEffect(fighter.paletteEffect, paletteEffect.effect);
        }
    }

    for (const auto& envColor : stateDef->envColors) {
        if (!shouldRunStateRuntimeController(state, fighter, envColor.id, envColor.trigger, opponent, stage)) {
            continue;
        }
        startEnvColor(state, envColor);
    }

    for (const auto& hitVelSet : stateDef->hitVelSets) {
        if (!shouldRunStateRuntimeController(state, fighter, hitVelSet.id, hitVelSet.trigger, opponent, stage)) {
            continue;
        }
        if (hitVelSet.applyX) {
            fighter.vx = fighter.hitVelocityX;
        }
        if (hitVelSet.applyY) {
            fighter.vy = fighter.hitVelocityY;
            clampArenaAppliedFallVelocity(state, fighter, false);
            if (fighter.vy < 0.0f) {
                fighter.onGround = false;
                fighter.stateType = 'A';
            }
        }
    }

    for (const auto& hitFallDamage : stateDef->hitFallDamages) {
        if (!shouldRunStateRuntimeController(state, fighter, hitFallDamage.id, hitFallDamage.trigger, opponent, stage)) {
            continue;
        }
        if (fighter.hitFall && fighter.hitFallDamage > 0) {
            fighter.life = std::max(0, fighter.life - fighter.hitFallDamage);
            fighter.hitFallDamage = 0;
        }
    }

    for (const auto& hitFallVel : stateDef->hitFallVels) {
        if (!shouldRunStateRuntimeController(state, fighter, hitFallVel.id, hitFallVel.trigger, opponent, stage)) {
            continue;
        }
        if (fighter.hitFall) {
            fighter.vx = fighter.hitFallBounceXVelocity;
            fighter.vy = fighter.hitFallBounceYVelocity;
            clampArenaAppliedFallVelocity(state, fighter, true);
            if (fighter.vy < 0.0f) {
                fighter.onGround = false;
                fighter.stateType = 'A';
            }
        }
    }

    for (const auto& hitFallSet : stateDef->hitFallSets) {
        if (!shouldRunStateRuntimeController(state, fighter, hitFallSet.id, hitFallSet.trigger, opponent, stage)) {
            continue;
        }
        if (hitFallSet.value == 0) {
            fighter.hitFall = false;
        } else if (hitFallSet.value == 1) {
            fighter.hitFall = true;
        }
        if (hitFallSet.hasXVelocity) {
            fighter.hitFallBounceXVelocity = hitFallSet.xVelocity * static_cast<float>(fighter.facing);
        }
        if (hitFallSet.hasYVelocity) {
            fighter.hitFallBounceYVelocity = hitFallSet.yVelocity;
            clampArenaHitFallRuntime(state, fighter);
        }
    }

    for (const auto& protection : stateDef->hitProtections) {
        if (!shouldRunStateRuntimeController(state, fighter, protection.id, protection.trigger, opponent, stage)) {
            continue;
        }
        if (protection.notHitBy) {
            fighter.notHitByTicks = std::max(fighter.notHitByTicks, protection.time);
            fighter.notHitByValue = protection.value;
        } else {
            fighter.hitByTicks = std::max(fighter.hitByTicks, protection.time);
            fighter.hitByValue = protection.value;
        }
    }

    for (const auto& typeSet : stateDef->stateTypeSets) {
        if (!shouldRunStateRuntimeController(state, fighter, typeSet.id, typeSet.trigger, opponent, stage)) {
            continue;
        }
        if (typeSet.hasStateType) {
            fighter.stateType = typeSet.stateType;
            fighter.onGround = fighter.stateType != 'A';
        }
        if (typeSet.hasMoveType) {
            fighter.moveType = typeSet.moveType;
        }
        if (typeSet.hasPhysics) {
            fighter.physics = typeSet.physics;
        }
    }

    updateStateVelocityControllersForDefinition(state, fighter, *stateDef, opponent, stage);

    updateStatePosSetControllersForDefinition(state, fighter, *stateDef, opponent, stage);

    for (const auto& screenBound : stateDef->screenBounds) {
        if (!shouldRunStateRuntimeController(state, fighter, screenBound.id, screenBound.trigger, opponent, stage)) {
            continue;
        }
        fighter.screenBound = screenBound.value;
        fighter.screenBoundMoveCameraX = screenBound.moveCameraX;
        fighter.screenBoundMoveCameraY = screenBound.moveCameraY;
    }

    for (const auto& width : stateDef->widths) {
        if (!shouldRunStateRuntimeController(state, fighter, width.id, width.trigger, opponent, stage)) {
            continue;
        }
        if (width.hasEdge) {
            fighter.edgeWidthFront = width.edgeFront;
            fighter.edgeWidthBack = width.edgeBack;
        }
        if (width.hasPlayer) {
            fighter.playerWidthFront = width.playerFront;
            fighter.playerWidthBack = width.playerBack;
        }
    }

    for (const auto& playerPush : stateDef->playerPushes) {
        if (!shouldRunStateRuntimeController(state, fighter, playerPush.id, playerPush.trigger, opponent, stage)) {
            continue;
        }
        fighter.playerPush = playerPush.value;
    }
        return true;
    });
}

FighterState* opponentForActor(AppState& state, const FighterState& actor) {
    int ownerIndex = actor.helper ? actor.ownerIndex : fighterIndexInState(state, actor);
    if (ownerIndex < 0 || state.fighters.size() < 2) {
        return nullptr;
    }
    if (isStoryMode(state)) {
        const int target = storyProjectileDefenderIndex(state, ownerIndex);
        return target >= 0 && target < static_cast<int>(state.fighters.size())
            ? &state.fighters[static_cast<size_t>(target)]
            : nullptr;
    }
    return &state.fighters[static_cast<size_t>(ownerIndex == 0 ? 1 : 0)];
}

const FighterState* opponentForActor(const AppState& state, const FighterState& actor) {
    int ownerIndex = actor.helper ? actor.ownerIndex : fighterIndexInState(state, actor);
    if (ownerIndex < 0 || state.fighters.size() < 2) {
        return nullptr;
    }
    if (isStoryMode(state)) {
        const int target = storyProjectileDefenderIndex(state, ownerIndex);
        return target >= 0 && target < static_cast<int>(state.fighters.size())
            ? &state.fighters[static_cast<size_t>(target)]
            : nullptr;
    }
    return &state.fighters[static_cast<size_t>(ownerIndex == 0 ? 1 : 0)];
}

void spawnStateHelper(
    AppState& state,
    const FighterState& owner,
    const FighterState* opponent,
    const StageSlot* stage,
    const StateHelperController& controller) {
    const int ownerIndex = fighterIndexInState(state, owner);
    if (ownerIndex < 0) {
        return;
    }

    const auto evalFloat = [&](const std::string& expression, float fallback) {
        if (trim(expression).empty()) {
            return fallback;
        }
        if (const auto value = evalMugenExpression(state, owner, expression, opponent, stage)) {
            return *value;
        }
        return fallback;
    };
    const auto evalInt = [&](const std::string& expression, int fallback) {
        if (trim(expression).empty()) {
            return fallback;
        }
        if (const auto value = evalMugenExpression(state, owner, expression, opponent, stage)) {
            return static_cast<int>(std::lround(*value));
        }
        return fallback;
    };

    const int stateNo = evalInt(controller.stateNoExpression, controller.stateNo);
    if (!findStateDefinitionForActor(state, owner, stateNo)) {
        return;
    }

    const float offsetX = evalFloat(controller.xExpression, controller.x);
    const float offsetY = evalFloat(controller.yExpression, controller.y);
    const float scaleX = evalFloat(controller.scaleXExpression, controller.scaleX);
    const float scaleY = evalFloat(controller.scaleYExpression, controller.scaleY);
    float baseX = owner.x;
    float baseY = owner.y;
    float offsetFacing = static_cast<float>(owner.facing);
    int baseFacing = owner.facing;
    if (controller.postype == "p2" && opponent) {
        baseX = opponent->x;
        baseY = opponent->y;
        offsetFacing = static_cast<float>(opponent->facing);
        baseFacing = opponent->facing;
    }

    FighterState helper;
    helper.helper = true;
    helper.ownerIndex = ownerIndex;
    helper.helperId = controller.helperId;
    helper.x = baseX + offsetX * offsetFacing;
    helper.y = baseY + offsetY;
    helper.depthZ = owner.depthZ;
    helper.facing = controller.facing == 0 ? baseFacing : (controller.facing > 0 ? baseFacing : -baseFacing);
    helper.onGround = helper.y >= 0.0f;
    helper.life = 1000;
    helper.sprPriority = controller.sprPriority;
    helper.pauseMoveTime = controller.pauseMoveTime;
    helper.superMoveTime = controller.superMoveTime;
    helper.scaleX = owner.scaleX * scaleX;
    helper.scaleY = owner.scaleY * scaleY;
    if (!enterState(state, helper, stateNo)) {
        return;
    }
    state.helpers.push_back(std::move(helper));
}

int rootOwnerIndexInState(const AppState& state, const FighterState& actor) {
    if (actor.helper) {
        return actor.ownerIndex;
    }
    return fighterIndexInState(state, actor);
}

void spawnStateProjectile(
    AppState& state,
    const FighterState& owner,
    const FighterState* opponent,
    const StageSlot* stage,
    const StateProjectileController& controller) {
    const int ownerIndex = rootOwnerIndexInState(state, owner);
    if (ownerIndex < 0
        || ownerIndex >= static_cast<int>(state.fighters.size())
        || !findExactClipForFighter(state, static_cast<size_t>(ownerIndex), controller.anim)) {
        return;
    }

    const auto evalFloat = [&](const std::string& expression, float fallback) {
        if (trim(expression).empty()) {
            return fallback;
        }
        if (const auto value = evalMugenExpression(state, owner, expression, opponent, stage)) {
            return *value;
        }
        return fallback;
    };
    const float offsetX = evalFloat(controller.xExpression, controller.x);
    const float offsetY = evalFloat(controller.yExpression, controller.y);
    const std::string postype = lowercaseCopy(controller.postype);
    float baseX = owner.x;
    float baseY = owner.y;
    float offsetFacing = static_cast<float>(owner.facing);
    if (postype == "p2" && opponent) {
        baseX = opponent->x;
        baseY = opponent->y;
        offsetFacing = static_cast<float>(opponent->facing);
    } else if (postype == "front" || postype == "back") {
        const float visibleLeft = state.cameraX - logicalWidthF(state) / 2.0f;
        const float visibleRight = state.cameraX + logicalWidthF(state) / 2.0f;
        const bool useFrontEdge = postype == "front";
        const bool edgeIsRight = useFrontEdge ? owner.facing > 0 : owner.facing < 0;
        baseX = edgeIsRight ? visibleRight : visibleLeft;
        baseY = owner.y;
        offsetFacing = edgeIsRight ? 1.0f : -1.0f;
    } else if (postype == "left") {
        baseX = state.cameraX - logicalWidthF(state) / 2.0f;
        baseY = 0.0f;
        offsetFacing = 1.0f;
    } else if (postype == "right") {
        baseX = state.cameraX + logicalWidthF(state) / 2.0f;
        baseY = 0.0f;
        offsetFacing = 1.0f;
    }

    RuntimeProjectile projectile;
    projectile.id = controller.projectileId;
    projectile.ownerIndex = ownerIndex;
    projectile.action = controller.anim;
    projectile.hitAction = controller.hitAnim;
    projectile.removeAction = controller.removeAnim;
    projectile.cancelAction = controller.cancelAnim;
    projectile.x = baseX + offsetX * offsetFacing;
    projectile.y = baseY + offsetY;
    projectile.depthZ = owner.depthZ;
    projectile.vx = evalFloat(controller.vxExpression, controller.vx);
    projectile.vy = evalFloat(controller.vyExpression, controller.vy);
    projectile.facing = owner.facing;
    projectile.hitsRemaining = std::max(1, controller.hits);
    projectile.removeTime = controller.removeTime;
    projectile.missTime = controller.missTime;
    projectile.removeWhenHit = controller.removeWhenHit;
    projectile.priority = controller.priority;
    projectile.cancelPriority = controller.cancelPriority;
    projectile.pauseMoveTime = controller.pauseMoveTime;
    projectile.superMoveTime = controller.superMoveTime;
    projectile.projEdgeBound = controller.projEdgeBound;
    projectile.projStageBound = controller.projStageBound;
    projectile.projHeightBoundLow = controller.projHeightBoundLow;
    projectile.projHeightBoundHigh = controller.projHeightBoundHigh;
    projectile.ax = evalFloat(controller.axExpression, controller.ax);
    projectile.ay = evalFloat(controller.ayExpression, controller.ay);
    projectile.velMulX = evalFloat(controller.velMulXExpression, controller.velMulX);
    projectile.velMulY = evalFloat(controller.velMulYExpression, controller.velMulY);
    projectile.removeVx = evalFloat(controller.removeVxExpression, controller.removeVx);
    projectile.removeVy = evalFloat(controller.removeVyExpression, controller.removeVy);
    projectile.scaleX = owner.scaleX * evalFloat(controller.scaleXExpression, controller.scaleX);
    projectile.scaleY = owner.scaleY * evalFloat(controller.scaleYExpression, controller.scaleY);
    projectile.shadowEnabled = controller.shadowEnabled;
    projectile.shadowR = controller.shadowR;
    projectile.shadowG = controller.shadowG;
    projectile.shadowB = controller.shadowB;
    projectile.hitDef = controller.hitDef;
    state.projectiles.push_back(std::move(projectile));
}

void applyRootBinding(AppState& state, FighterState& helper) {
    if (!helper.helper || helper.rootBindTicks <= 0) {
        return;
    }
    FighterState* root = fighterOwner(state, helper);
    if (!root) {
        helper.rootBindTicks = 0;
        return;
    }

    helper.x = root->x + helper.rootBindOffsetX * static_cast<float>(root->facing);
    helper.y = root->y + helper.rootBindOffsetY;
    helper.depthZ = root->depthZ;
    if (helper.rootBindFacing > 0) {
        helper.facing = root->facing;
    } else if (helper.rootBindFacing < 0) {
        helper.facing = -root->facing;
    }
    --helper.rootBindTicks;
}

void updateStateProjectileControllers(AppState& state, FighterState& fighter, const FighterState* opponent, const StageSlot* stage) {
    forEachRuntimeControllerStateDefinition(state, fighter, [&](const StateDefinition& stateDef) {
        for (const auto& projectile : stateDef.projectiles) {
            if (!shouldRunStateOneShotController(state, fighter, projectile.id, projectile.trigger, opponent, stage)) {
                continue;
            }
            spawnStateProjectile(state, fighter, opponent, stage, projectile);
        }
        return true;
    });
}

void updateStateHelperControllers(AppState& state, FighterState& fighter, const FighterState* opponent, const StageSlot* stage) {
    forEachRuntimeControllerStateDefinition(state, fighter, [&](const StateDefinition& stateDef) {
        for (const auto& helper : stateDef.helpers) {
            if (!shouldRunStateOneShotController(state, fighter, helper.id, helper.trigger, opponent, stage)) {
                continue;
            }
            spawnStateHelper(state, fighter, opponent, stage, helper);
        }

    for (const auto& bind : stateDef.bindToParents) {
        if (!fighter.helper || !shouldRunStateRuntimeController(state, fighter, bind.id, bind.trigger, opponent, stage)) {
            continue;
        }
        if (const FighterState* owner = fighterOwner(state, fighter)) {
            fighter.x = owner->x + bind.x * static_cast<float>(owner->facing);
            fighter.y = owner->y + bind.y;
            fighter.depthZ = owner->depthZ;
            fighter.facing = owner->facing;
        }
    }

    for (const auto& bind : stateDef.bindToRoots) {
        if (!fighter.helper || !shouldRunStateRuntimeController(state, fighter, bind.id, bind.trigger, opponent, stage)) {
            continue;
        }
        fighter.rootBindTicks = std::max(1, bind.time);
        fighter.rootBindOffsetX = bind.x;
        fighter.rootBindOffsetY = bind.y;
        fighter.rootBindFacing = bind.facing;
        applyRootBinding(state, fighter);
    }

    for (const auto& parentVarAdd : stateDef.parentVarAdds) {
        if (!fighter.helper || !shouldRunStateRuntimeController(state, fighter, parentVarAdd.id, parentVarAdd.trigger, opponent, stage)) {
            continue;
        }
        FighterState* owner = fighterOwner(state, fighter);
        if (!owner) {
            continue;
        }
        const auto value = evalMugenExpression(state, fighter, parentVarAdd.valueExpression, opponent, stage);
        if (!value) {
            continue;
        }
        setFighterVariableValue(*owner, parentVarAdd.target, fighterVariableValue(*owner, parentVarAdd.target) + *value);
    }

    for (const auto& destroySelf : stateDef.destroySelfs) {
        if (!fighter.helper || !shouldRunStateRuntimeController(state, fighter, destroySelf.id, destroySelf.trigger, opponent, stage)) {
            continue;
        }
        fighter.destroyRequested = true;
        return false;
    }
        return true;
    });
}

void spawnStateExplod(
    AppState& state,
    const FighterState& fighter,
    const FighterState* opponent,
    const StageSlot& stage,
    const StateExplodController& explod) {
    const int clipOwnerIndex = rootOwnerIndexInState(state, fighter);
    const AnimationClip* clip = explod.fromFightFx
        ? findFightFxClip(state, explod.anim)
        : (clipOwnerIndex >= 0 && clipOwnerIndex < static_cast<int>(state.fighters.size())
            ? findExactClipForFighter(state, static_cast<size_t>(clipOwnerIndex), explod.anim)
            : findExactClip(state, explod.anim));
    if (!clip) {
        return;
    }

    float baseX = fighter.x;
    float baseY = fighter.y;
    float offsetFacing = static_cast<float>(fighter.facing);
    if (explod.postype == "p2" && opponent) {
        baseX = opponent->x;
        baseY = opponent->y;
        offsetFacing = static_cast<float>(opponent->facing);
    } else if (explod.postype == "front") {
        const float halfWidth = logicalWidthF(state) * 0.5f;
        baseX = fighter.facing >= 0 ? state.cameraX + halfWidth : state.cameraX - halfWidth;
        baseY = 0.0f;
        offsetFacing = 1.0f;
    } else if (explod.postype == "back") {
        const float halfWidth = logicalWidthF(state) * 0.5f;
        baseX = fighter.facing >= 0 ? state.cameraX - halfWidth : state.cameraX + halfWidth;
        baseY = 0.0f;
        offsetFacing = 1.0f;
    }

    RuntimeEffect effect;
    effect.id = explod.explodId;
    effect.ownerIndex = fighterIndexInState(state, fighter);
    effect.clipOwnerIndex = clipOwnerIndex;
    effect.action = explod.anim;
    effect.x = std::clamp(baseX + explod.x * offsetFacing, stage.leftbound, stage.rightbound);
    effect.y = baseY + explod.y;
    effect.depthZ = fighter.depthZ;
    effect.bindOffsetX = explod.x;
    effect.bindOffsetY = explod.y;
    effect.bindTicks = explod.bindTime;
    effect.removeTime = explod.removeTime;
    effect.fromFightFx = explod.fromFightFx;
    effect.sprPriority = explod.sprPriority;
    effect.pauseMoveTime = explod.pauseMoveTime;
    effect.superMoveTime = explod.superMoveTime;
    effect.scaleX = explod.fromFightFx ? explod.scaleX : fighter.scaleX * explod.scaleX;
    effect.scaleY = explod.fromFightFx ? explod.scaleY : fighter.scaleY * explod.scaleY;
    state.runtimeEffects.push_back(effect);
}

void spawnGameMakeAnim(
    AppState& state,
    const FighterState& fighter,
    const StageSlot& stage,
    const StateGameMakeAnimController& gameMakeAnim) {
    const auto value = evalMugenExpression(state, fighter, gameMakeAnim.valueExpression, nullptr, &stage);
    if (!value) {
        return;
    }
    const int action = std::max(0, static_cast<int>(std::lround(*value)));
    if (!findFightFxClip(state, action)) {
        return;
    }

    float randomX = 0.0f;
    float randomY = 0.0f;
    if (gameMakeAnim.random > 0) {
        const float spread = static_cast<float>(gameMakeAnim.random) * 0.5f;
        randomX = (static_cast<float>(pseudoMugenRandom(state, fighter, gameMakeAnim.id)) / 999.0f - 0.5f) * static_cast<float>(gameMakeAnim.random);
        randomY = (static_cast<float>(pseudoMugenRandom(state, fighter, gameMakeAnim.id + 7919)) / 999.0f - 0.5f) * static_cast<float>(gameMakeAnim.random);
        randomX = std::clamp(randomX, -spread, spread);
        randomY = std::clamp(randomY, -spread, spread);
    }

    RuntimeEffect effect;
    effect.ownerIndex = rootOwnerIndexInState(state, fighter);
    effect.clipOwnerIndex = effect.ownerIndex;
    effect.action = action;
    effect.fromFightFx = true;
    effect.x = std::clamp(
        fighter.x + (gameMakeAnim.x + randomX) * static_cast<float>(fighter.facing),
        stage.leftbound,
        stage.rightbound);
    effect.y = fighter.y + gameMakeAnim.y + randomY;
    effect.depthZ = fighter.depthZ;
    effect.removeTime = -2;
    effect.sprPriority = gameMakeAnim.under ? -3 : 3;
    state.runtimeEffects.push_back(effect);
}

void spawnDustCloud(AppState& state, const FighterState& fighter, const StageSlot& stage, float x, float y) {
    constexpr int kSmallFloorDustAction = 120;
    if (!findFightFxClip(state, kSmallFloorDustAction)) {
        return;
    }

    RuntimeEffect effect;
    effect.action = kSmallFloorDustAction;
    effect.fromFightFx = true;
    effect.ownerIndex = rootOwnerIndexInState(state, fighter);
    effect.clipOwnerIndex = effect.ownerIndex;
    effect.x = std::clamp(fighter.x + x * static_cast<float>(fighter.facing), stage.leftbound, stage.rightbound);
    effect.y = fighter.y + y;
    effect.depthZ = fighter.depthZ;
    effect.removeTime = -2;
    effect.sprPriority = 3;
    state.runtimeEffects.push_back(effect);
}

void updateStateMakeDustControllers(
    AppState& state,
    FighterState& fighter,
    const FighterState* opponent,
    const StageSlot& stage) {
    forEachRuntimeControllerStateDefinition(state, fighter, [&](const StateDefinition& stateDef) {
        for (const auto& makeDust : stateDef.makeDusts) {
            if (!shouldRunStateRuntimeController(state, fighter, makeDust.id, makeDust.trigger, opponent, &stage)) {
                continue;
            }
            if (fighter.stateTime % std::max(1, makeDust.spacing) != 0) {
                continue;
            }
            spawnDustCloud(state, fighter, stage, makeDust.x, makeDust.y);
            if (makeDust.hasPos2) {
                spawnDustCloud(state, fighter, stage, makeDust.x2, makeDust.y2);
            }
        }
        return true;
    });
}

bool runtimeEffectMatchesOwnerAndId(const RuntimeEffect& effect, int ownerIndex, int explodId) {
    if (effect.ownerIndex != ownerIndex) {
        return false;
    }
    return explodId < 0 || effect.id == explodId;
}

void modifyRuntimeExplods(AppState& state, const FighterState& fighter, const StateModifyExplodController& modifyExplod) {
    const int ownerIndex = fighterIndexInState(state, fighter);
    for (auto& effect : state.runtimeEffects) {
        if (!runtimeEffectMatchesOwnerAndId(effect, ownerIndex, modifyExplod.explodId)) {
            continue;
        }
        if (modifyExplod.hasSprPriority) {
            effect.sprPriority = modifyExplod.sprPriority;
        }
        if (modifyExplod.hasScale) {
            effect.scaleX = effect.fromFightFx ? modifyExplod.scaleX : fighter.scaleX * modifyExplod.scaleX;
            effect.scaleY = effect.fromFightFx ? modifyExplod.scaleY : fighter.scaleY * modifyExplod.scaleY;
        }
    }
}

void removeRuntimeExplods(AppState& state, const FighterState& fighter, const StateRemoveExplodController& removeExplod) {
    const int ownerIndex = fighterIndexInState(state, fighter);
    state.runtimeEffects.erase(
        std::remove_if(state.runtimeEffects.begin(), state.runtimeEffects.end(), [ownerIndex, &removeExplod](const RuntimeEffect& effect) {
            return runtimeEffectMatchesOwnerAndId(effect, ownerIndex, removeExplod.explodId);
        }),
        state.runtimeEffects.end());
}

void updateStateExplodControllers(
    AppState& state,
    FighterState& fighter,
    const FighterState* opponent,
    const StageSlot& stage) {
    forEachRuntimeControllerStateDefinition(state, fighter, [&](const StateDefinition& stateDef) {
        for (const auto& explod : stateDef.explods) {
            if (!shouldRunStateOneShotController(state, fighter, explod.id, explod.trigger, opponent, &stage)) {
                continue;
            }
            spawnStateExplod(state, fighter, opponent, stage, explod);
        }

    for (const auto& gameMakeAnim : stateDef.gameMakeAnims) {
        if (!shouldRunStateOneShotController(state, fighter, gameMakeAnim.id, gameMakeAnim.trigger, opponent, &stage)) {
            continue;
        }
        spawnGameMakeAnim(state, fighter, stage, gameMakeAnim);
    }

    for (const auto& modifyExplod : stateDef.modifyExplods) {
        if (!shouldRunStateRuntimeController(state, fighter, modifyExplod.id, modifyExplod.trigger, opponent, &stage)) {
            continue;
        }
        modifyRuntimeExplods(state, fighter, modifyExplod);
    }

    for (const auto& removeExplod : stateDef.removeExplods) {
        if (!shouldRunStateRuntimeController(state, fighter, removeExplod.id, removeExplod.trigger, opponent, &stage)) {
            continue;
        }
        removeRuntimeExplods(state, fighter, removeExplod);
    }
        return true;
    });
}

bool updateStateChangeStateControllers(
    AppState& state,
    FighterState& fighter,
    const FighterState* opponent = nullptr,
    const StageSlot* stage = nullptr) {
    bool changed = false;
    forEachRuntimeControllerStateDefinition(state, fighter, [&](const StateDefinition& stateDef) {
        for (const auto& changeState : stateDef.changeStates) {
            if (!shouldRunStateRuntimeController(state, fighter, changeState.id, changeState.trigger, opponent, stage)) {
                continue;
            }
            std::optional<bool> ctrl;
            if (changeState.hasCtrl) {
                ctrl = changeState.ctrl;
            }
            const auto targetState = evalMugenExpression(
                state,
                fighter,
                changeState.targetStateExpression.empty()
                    ? std::to_string(changeState.targetState)
                    : changeState.targetStateExpression,
                opponent,
                stage);
            if (!targetState) {
                continue;
            }
            const bool changedDuringHitPause = fighter.hitPauseTicks > 0;
            applyParsedChangeState(state, fighter, static_cast<int>(std::lround(*targetState)), ctrl, changeState.selfState);
            if (changedDuringHitPause) {
                fighter.hitPauseChangeStateControllerId = changeState.id;
            }
            changed = true;
            return false;
        }
        return true;
    });
    return changed;
}

