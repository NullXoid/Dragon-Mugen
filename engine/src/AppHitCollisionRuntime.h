#pragma once

// Internal App.cpp implementation shard.
// Hit routing, collision, damage, combo, and Arena combat include wiring.

int currentAnimElemForFighter(const AppState& state, const FighterState& fighter) {
    const AnimationClip* clip = findClipForActor(state, fighter, fighter.action);
    return clip ? frameIndexForClip(*clip, fighter.animTick) + 1 : 0;
}

int animElemTimeForFighter(const AppState& state, const FighterState& fighter, int elem) {
    const AnimationClip* clip = findClipForActor(state, fighter, fighter.action);
    return clip ? animElemTimeForClip(*clip, fighter.animTick, elem) : 0;
}

int hitDefApplicationKey(int hitDefId, int animElem) {
    return hitDefId * 100000 + std::max(0, animElem);
}

int hitDefTargetApplicationKey(int hitDefId, size_t defenderIndex) {
    return hitDefId * 100000 + 90000 + static_cast<int>(defenderIndex);
}

bool hitDefAlreadyApplied(const FighterState& fighter, int hitDefId, int animElem, std::optional<size_t> defenderIndex = std::nullopt) {
    const int key = hitDefApplicationKey(hitDefId, animElem);
    if (defenderIndex) {
        const int targetKey = hitDefTargetApplicationKey(hitDefId, *defenderIndex);
        return std::find(fighter.appliedHitDefIds.begin(), fighter.appliedHitDefIds.end(), targetKey)
            != fighter.appliedHitDefIds.end();
    }
    return std::find(fighter.appliedHitDefIds.begin(), fighter.appliedHitDefIds.end(), hitDefId)
        != fighter.appliedHitDefIds.end()
        || std::find(fighter.appliedHitDefIds.begin(), fighter.appliedHitDefIds.end(), key)
        != fighter.appliedHitDefIds.end();
}

void markHitDefApplied(FighterState& fighter, int hitDefId, int animElem, std::optional<size_t> defenderIndex = std::nullopt) {
    const int key = hitDefApplicationKey(hitDefId, animElem);
    if (defenderIndex) {
        const int targetKey = hitDefTargetApplicationKey(hitDefId, *defenderIndex);
        if (std::find(fighter.appliedHitDefIds.begin(), fighter.appliedHitDefIds.end(), targetKey)
            == fighter.appliedHitDefIds.end()) {
            fighter.appliedHitDefIds.push_back(targetKey);
        }
        return;
    }
    if (std::find(fighter.appliedHitDefIds.begin(), fighter.appliedHitDefIds.end(), key)
        == fighter.appliedHitDefIds.end()) {
        fighter.appliedHitDefIds.push_back(key);
    }
}

bool actorHasPlayerBodyWidth(const AppState& state, const FighterState& fighter) {
    if (fighter.helper) {
        return fighter.ownerIndex >= 0 && fighter.ownerIndex < static_cast<int>(state.fighters.size());
    }
    if (fighter.customStateOwnerIndex >= 0
        && fighter.customStateOwnerIndex < static_cast<int>(state.fighters.size())) {
        return true;
    }
    return fighterIndexInState(state, fighter) >= 0;
}

float actorBodyFrontWidth(const AppState& state, const FighterState& fighter) {
    return actorHasPlayerBodyWidth(state, fighter) ? fighterPlayerFrontWidth(state, fighter) : 0.0f;
}

float actorBodyBackWidth(const AppState& state, const FighterState& fighter) {
    return actorHasPlayerBodyWidth(state, fighter) ? fighterPlayerBackWidth(state, fighter) : 0.0f;
}

float p2BodyDistXValue(const AppState& state, const FighterState& attacker, const FighterState& defender) {
    const float axisDistance = (defender.x - attacker.x) * static_cast<float>(attacker.facing);
    if (!actorHasPlayerBodyWidth(state, attacker) || !actorHasPlayerBodyWidth(state, defender)) {
        return axisDistance;
    }

    const float worldDistance = defender.x - attacker.x;
    const float attackerFrontWorld = actorBodyFrontWidth(state, attacker) * static_cast<float>(attacker.facing);
    const bool defenderIsInFront = worldDistance * static_cast<float>(attacker.facing) >= 0.0f;
    const bool defenderFacingOpposite = attacker.facing != defender.facing;
    const float defenderReferenceLocal = defenderIsInFront == defenderFacingOpposite
        ? actorBodyFrontWidth(state, defender)
        : -actorBodyBackWidth(state, defender);
    const float defenderReferenceWorld = defenderReferenceLocal * static_cast<float>(defender.facing);
    return (worldDistance - attackerFrontWorld + defenderReferenceWorld) * static_cast<float>(attacker.facing);
}

char hitDefStateAttr(const HitDefinition& hitDef) {
    const auto parts = splitCsv(hitDef.attr);
    if (parts.empty()) {
        return 'S';
    }
    const std::string statePart = uppercaseCopy(trim(parts.front()));
    for (const char ch : statePart) {
        if (ch == 'S' || ch == 'C' || ch == 'A') {
            return ch;
        }
    }
    return 'S';
}

std::vector<std::string> hitDefAttackAttrs(const HitDefinition& hitDef) {
    const auto parts = splitCsv(hitDef.attr);
    std::vector<std::string> attrs;
    for (size_t i = 1; i < parts.size(); ++i) {
        const std::string attr = uppercaseCopy(trim(parts[i]));
        if (!attr.empty()) {
            attrs.push_back(attr);
        }
    }
    return attrs;
}

bool hitProtectionMatches(const std::string& protection, const HitDefinition& hitDef) {
    const auto parts = splitCsv(protection);
    if (parts.empty()) {
        return false;
    }

    const std::string states = uppercaseCopy(trim(parts.front()));
    const char hitState = hitDefStateAttr(hitDef);
    if (!states.empty() && states.find(hitState) == std::string::npos) {
        return false;
    }

    std::vector<std::string> allowedAttrs;
    for (size_t i = 1; i < parts.size(); ++i) {
        const std::string attr = uppercaseCopy(trim(parts[i]));
        if (!attr.empty()) {
            allowedAttrs.push_back(attr);
        }
    }
    if (allowedAttrs.empty()) {
        return true;
    }

    const auto hitAttrs = hitDefAttackAttrs(hitDef);
    if (hitAttrs.empty()) {
        return false;
    }
    for (const auto& hitAttr : hitAttrs) {
        for (const auto& allowed : allowedAttrs) {
            if (hitAttr == allowed) {
                return true;
            }
        }
    }
    return false;
}

bool defenderCanBeHitBy(const FighterState& defender, const HitDefinition& hitDef) {
    if (defender.notHitByTicks > 0 && hitProtectionMatches(defender.notHitByValue, hitDef)) {
        return false;
    }
    if (defender.hitByTicks > 0 && !hitProtectionMatches(defender.hitByValue, hitDef)) {
        return false;
    }
    return true;
}

const StateHitOverrideController* activeHitOverrideForDefender(
    AppState& state,
    FighterState& defender,
    const FighterState* attacker,
    const HitDefinition& hitDef) {
    const StateDefinition* stateDef = findStateDefinitionForActor(state, defender, defender.stateNo);
    if (!stateDef) {
        return nullptr;
    }

    for (const auto& hitOverride : stateDef->hitOverrides) {
        if (!findStateDefinitionForActor(state, defender, hitOverride.stateNo)
            || !hitProtectionMatches(hitOverride.attr, hitDef)
            || !shouldRunStateRuntimeController(state, defender, hitOverride.id, hitOverride.trigger, attacker, nullptr)) {
            continue;
        }
        return &hitOverride;
    }
    return nullptr;
}

bool hitDefTriggerIsActive(const AppState& state, const HitDefinition& hitDef, const FighterState& attacker, const FighterState& defender, int animElem) {
    bool hasTrigger = false;
    bool active = false;
    if (hitDef.triggerTime >= 0) {
        hasTrigger = true;
        active = active || attacker.stateTime >= hitDef.triggerTime;
    }
    if (hitDef.triggerAnimElem > 0) {
        hasTrigger = true;
        active = active || animElem >= hitDef.triggerAnimElem;
    }
    if (hasTrigger && !active) {
        return false;
    }
    if (hitDef.hasP2DistX
        && !compareFloatValue(p2AxisDistXValue(attacker, &defender), hitDef.p2DistXOp, hitDef.p2DistX)) {
        return false;
    }
    if (hitDef.hasP2BodyDistX
        && !compareFloatValue(p2BodyDistXValue(state, attacker, defender), hitDef.p2BodyDistXOp, hitDef.p2BodyDistX)) {
        return false;
    }
    return true;
}

int hitDefTriggerScore(const HitDefinition& hitDef) {
    if (hitDef.triggerAnimElem > 0) {
        return 10000 + hitDef.triggerAnimElem;
    }
    if (hitDef.triggerTime >= 0) {
        return hitDef.triggerTime;
    }
    return 0;
}

const HitDefinition* findActiveHitDefinition(const AppState& state, const FighterState& attacker, const FighterState& defender, size_t defenderIndex) {
    const int animElem = currentAnimElemForFighter(state, attacker);
    const HitDefinition* best = nullptr;
    int bestScore = -1;
    for (const auto& hitDef : hitDefinitionsForActor(state, attacker)) {
        if (hitDef.stateNo != attacker.stateNo || hitDefAlreadyApplied(attacker, hitDef.id, animElem, defenderIndex)) {
            continue;
        }
        if (!defenderCanBeHitBy(defender, hitDef)) {
            continue;
        }
        if (!hitFlagAllowsDefender(hitDef, defender)) {
            continue;
        }
        if (!hitDefTriggerIsActive(state, hitDef, attacker, defender, animElem)) {
            continue;
        }
        const int score = hitDefTriggerScore(hitDef);
        if (!best || score >= bestScore) {
            best = &hitDef;
            bestScore = score;
        }
    }
    return best;
}

HitDefinition resolveHitDefinitionExpressions(
    const AppState& state,
    const HitDefinition& source,
    const FighterState& attacker,
    const FighterState& defender,
    const StageSlot* stage) {
    HitDefinition resolved = source;

    const auto evalFloat = [&](const std::string& expression, float fallback) {
        if (trim(expression).empty()) {
            return fallback;
        }
        if (const auto value = evalMugenExpression(state, attacker, expression, &defender, stage)) {
            return *value;
        }
        return fallback;
    };
    const auto evalInt = [&](const std::string& expression, int fallback) {
        return static_cast<int>(evalFloat(expression, static_cast<float>(fallback)));
    };
    const auto evalBool = [&](const std::string& expression, bool fallback) {
        return evalFloat(expression, fallback ? 1.0f : 0.0f) != 0.0f;
    };
    const auto resolveEnvShake = [&](const EnvShakeSpec& sourceShake) {
        EnvShakeSpec shake = sourceShake;
        shake.time = std::max(0, evalInt(sourceShake.timeExpression, sourceShake.time));
        shake.frequency = std::max(1, evalInt(sourceShake.frequencyExpression, sourceShake.frequency));
        shake.amplitude = evalFloat(sourceShake.amplitudeExpression, sourceShake.amplitude);
        shake.phase = evalInt(sourceShake.phaseExpression, sourceShake.phase);
        shake.enabled = shake.time > 0 && std::abs(shake.amplitude) > 0.001f;
        return shake;
    };
    const auto resolvePalette = [&](const PaletteEffectSpec& sourceEffect) {
        PaletteEffectSpec effect = sourceEffect;
        effect.time = evalInt(sourceEffect.timeExpression, sourceEffect.time);
        effect.addR = evalInt(sourceEffect.addExpressions[0], sourceEffect.addR);
        effect.addG = evalInt(sourceEffect.addExpressions[1], sourceEffect.addG);
        effect.addB = evalInt(sourceEffect.addExpressions[2], sourceEffect.addB);
        effect.mulR = evalInt(sourceEffect.mulExpressions[0], sourceEffect.mulR);
        effect.mulG = evalInt(sourceEffect.mulExpressions[1], sourceEffect.mulG);
        effect.mulB = evalInt(sourceEffect.mulExpressions[2], sourceEffect.mulB);
        effect.sinAddR = evalInt(sourceEffect.sinAddExpressions[0], sourceEffect.sinAddR);
        effect.sinAddG = evalInt(sourceEffect.sinAddExpressions[1], sourceEffect.sinAddG);
        effect.sinAddB = evalInt(sourceEffect.sinAddExpressions[2], sourceEffect.sinAddB);
        effect.sinPeriod = evalInt(sourceEffect.sinAddExpressions[3], sourceEffect.sinPeriod);
        effect.color = evalInt(sourceEffect.colorExpression, sourceEffect.color);
        effect.invertAll = evalBool(sourceEffect.invertAllExpression, sourceEffect.invertAll);
        effect.enabled = effect.time != 0
            && (effect.addR != 0 || effect.addG != 0 || effect.addB != 0
                || effect.mulR != 256 || effect.mulG != 256 || effect.mulB != 256
                || effect.sinAddR != 0 || effect.sinAddG != 0 || effect.sinAddB != 0
                || effect.color != 256 || effect.invertAll);
        return effect;
    };

    resolved.damage = evalInt(source.damageExpression, source.damage);
    resolved.guardDamage = evalInt(source.guardDamageExpression, source.guardDamage);
    resolved.pausetimeP1 = evalInt(source.pausetimeP1Expression, source.pausetimeP1);
    resolved.pausetimeP2 = evalInt(source.pausetimeP2Expression, source.pausetimeP2);
    resolved.sparkNo = evalInt(source.sparkNoExpression, source.sparkNo);
    resolved.guardSparkNo = evalInt(source.guardSparkNoExpression, source.guardSparkNo);
    resolved.sparkX = evalFloat(source.sparkXExpression, source.sparkX);
    resolved.sparkY = evalFloat(source.sparkYExpression, source.sparkY);
    resolved.hitSoundGroup = evalInt(source.hitSoundGroupExpression, source.hitSoundGroup);
    resolved.hitSoundIndex = evalInt(source.hitSoundIndexExpression, source.hitSoundIndex);
    resolved.guardSoundGroup = evalInt(source.guardSoundGroupExpression, source.guardSoundGroup);
    resolved.guardSoundIndex = evalInt(source.guardSoundIndexExpression, source.guardSoundIndex);
    resolved.envShake = resolveEnvShake(source.envShake);
    resolved.fallEnvShake = resolveEnvShake(source.fallEnvShake);
    resolved.palFx = resolvePalette(source.palFx);
    resolved.groundSlideTime = evalInt(source.groundSlideTimeExpression, source.groundSlideTime);
    resolved.groundHitTime = evalInt(source.groundHitTimeExpression, source.groundHitTime);
    resolved.groundVelocityX = evalFloat(source.groundVelocityXExpression, source.groundVelocityX);
    resolved.groundVelocityY = evalFloat(source.groundVelocityYExpression, source.groundVelocityY);
    resolved.hasAirVelocity = source.hasAirVelocity;
    if (source.hasAirVelocity) {
        resolved.airVelocityX = evalFloat(source.airVelocityXExpression, source.airVelocityX);
        resolved.airVelocityY = evalFloat(source.airVelocityYExpression, source.airVelocityY);
    }
    resolved.airHitTime = evalInt(source.airHitTimeExpression, source.airHitTime);
    resolved.hasSnap = source.hasSnap;
    if (source.hasSnap) {
        resolved.snapX = evalFloat(source.snapXExpression, source.snapX);
        resolved.snapY = evalFloat(source.snapYExpression, source.snapY);
    }
    resolved.fall = evalBool(source.fallExpression, source.fall);
    resolved.airFall = evalBool(source.airFallExpression, source.airFall);
    resolved.fallRecover = evalBool(source.fallRecoverExpression, source.fallRecover);
    resolved.fallRecoverTime = evalInt(source.fallRecoverTimeExpression, source.fallRecoverTime);
    resolved.fallDamage = evalInt(source.fallDamageExpression, source.fallDamage);
    resolved.downRecover = evalBool(source.downRecoverExpression, source.downRecover);
    resolved.downRecoverTime = evalInt(source.downRecoverTimeExpression, source.downRecoverTime);
    resolved.downHitTime = evalInt(source.downHitTimeExpression, source.downHitTime);
    resolved.downBounce = evalBool(source.downBounceExpression, source.downBounce);
    resolved.hasDownVelocity = source.hasDownVelocity;
    if (source.hasDownVelocity) {
        resolved.downVelocityX = evalFloat(source.downVelocityXExpression, source.downVelocityX);
        resolved.downVelocityY = evalFloat(source.downVelocityYExpression, source.downVelocityY);
    } else {
        resolved.downVelocityX = resolved.airVelocityX;
        resolved.downVelocityY = resolved.airVelocityY;
    }
    resolved.hasFallXVelocity = source.hasFallXVelocity;
    if (source.hasFallXVelocity) {
        resolved.fallXVelocity = evalFloat(source.fallXVelocityExpression, source.fallXVelocity);
    }
    resolved.hasFallYVelocity = source.hasFallYVelocity;
    if (source.hasFallYVelocity) {
        resolved.fallYVelocity = evalFloat(source.fallYVelocityExpression, source.fallYVelocity);
    }
    resolved.hasYAccel = source.hasYAccel;
    if (source.hasYAccel) {
        resolved.yAccel = evalFloat(source.yAccelExpression, source.yAccel);
    }
    if (source.hasGuardVelocity) {
        resolved.guardVelocityX = evalFloat(source.guardVelocityXExpression, source.guardVelocityX);
        resolved.guardVelocityY = evalFloat(source.guardVelocityYExpression, source.guardVelocityY);
    } else {
        resolved.guardVelocityX = resolved.groundVelocityX;
        resolved.guardVelocityY = resolved.groundVelocityY;
    }
    resolved.p1StateNo = evalInt(source.p1StateNoExpression, source.p1StateNo);
    resolved.hasP1Facing = source.hasP1Facing;
    if (source.hasP1Facing) {
        resolved.p1Facing = evalInt(source.p1FacingExpression, source.p1Facing);
    }
    resolved.p2StateNo = evalInt(source.p2StateNoExpression, source.p2StateNo);
    resolved.p2GetP1State = evalBool(source.p2GetP1StateExpression, source.p2GetP1State);
    resolved.hasP2Facing = source.hasP2Facing;
    if (source.hasP2Facing) {
        resolved.p2Facing = evalInt(source.p2FacingExpression, source.p2Facing);
    }

    return resolved;
}

constexpr int kStoredTargetLinkTicks = 600;

void refreshStoredTargetLink(FighterState& fighter) {
    if (fighter.targetIndex >= 0) {
        fighter.targetTicks = std::max(fighter.targetTicks, kStoredTargetLinkTicks);
    }
}

bool targetControllerMatchesStoredTarget(const FighterState& fighter, int controllerTargetId) {
    return controllerTargetId < 0 || controllerTargetId == fighter.targetHitId;
}

bool triggerGroupRequiresCommand(const StateTriggerGroup& group, std::string_view command) {
    return std::any_of(group.requiredCommands.begin(), group.requiredCommands.end(), [command](const std::string& required) {
        return equalsNoCase(required, command);
    });
}

bool triggerRequiresCommand(const StateControllerTrigger& trigger, std::string_view command) {
    for (const auto& allGroupOptions : trigger.triggerAllExpressions) {
        for (const auto& group : allGroupOptions) {
            if (triggerGroupRequiresCommand(group, command)) {
                return true;
            }
        }
    }
    for (const auto& group : trigger.triggerGroups) {
        if (triggerGroupRequiresCommand(group, command)) {
            return true;
        }
    }
    return false;
}

bool targetStateRecoveryCommandSatisfied(
    const AppState& state,
    const FighterState& target,
    const StateTargetStateController& targetState) {
    if (!triggerRequiresCommand(targetState.trigger, "recovery")) {
        return true;
    }
    const auto commands = collectCurrentFighterCommands(state, target);
    return commandActive(commands, "recovery");
}

void clearFighterTargetLink(FighterState& fighter) {
    fighter.targetIndex = -1;
    fighter.targetHitId = -1;
    fighter.targetTicks = 0;
}

void updateStateTargetControllers(
    AppState& state,
    FighterState& fighter,
    const FighterState* opponent = nullptr,
    const StageSlot* stage = nullptr) {
    if (fighter.targetIndex < 0 || fighter.targetIndex >= static_cast<int>(state.fighters.size())) {
        return;
    }

    auto& target = state.fighters[static_cast<size_t>(fighter.targetIndex)];
    const int binderIndex = fighter.helper ? fighter.ownerIndex : fighterIndexInState(state, fighter);
    forEachRuntimeControllerStateDefinition(state, fighter, [&](const StateDefinition& runtimeStateDef) {
        const StateDefinition* stateDef = &runtimeStateDef;

    for (const auto& targetState : stateDef->targetStates) {
        if (!targetControllerMatchesStoredTarget(fighter, targetState.targetId)) {
            continue;
        }
        const int previousCustomOwnerIndex = target.customStateOwnerIndex;
        target.customStateOwnerIndex = binderIndex;
        const bool stateAvailable = findStateDefinitionForActor(state, target, targetState.value) != nullptr;
        const bool recoverySatisfied = targetStateRecoveryCommandSatisfied(state, target, targetState);
        const bool triggerSatisfied = shouldRunStateOneShotController(state, fighter, targetState.id, targetState.trigger, opponent, stage);
        if (!stateAvailable || !recoverySatisfied || !triggerSatisfied) {
            target.customStateOwnerIndex = previousCustomOwnerIndex;
            continue;
        }
        enterState(state, target, targetState.value);
        target.customHitState = true;
        target.customStateOwnerIndex = binderIndex;
        target.moveType = 'H';
        target.ctrl = false;
        target.boundByIndex = binderIndex;
        target.bindTicks = std::max(target.bindTicks, 1);
        refreshStoredTargetLink(fighter);
    }

    for (const auto& targetBind : stateDef->targetBinds) {
        if (!targetControllerMatchesStoredTarget(fighter, targetBind.targetId)) {
            continue;
        }
        if (!shouldRunStateRuntimeController(state, fighter, targetBind.id, targetBind.trigger, opponent, stage)) {
            continue;
        }
        target.boundByIndex = binderIndex;
        target.bindTicks = targetBind.time < 0 ? 9999 : std::max(1, targetBind.time);
        target.targetBindPositionActive = true;
        target.targetBindOffsetX = targetBind.x;
        target.targetBindOffsetY = targetBind.y;
        target.targetBindFacing = fighter.facing;
        target.vx = 0.0f;
        target.vy = 0.0f;
        refreshStoredTargetLink(fighter);
    }

    for (const auto& targetFacing : stateDef->targetFacings) {
        if (!targetControllerMatchesStoredTarget(fighter, targetFacing.targetId)
            || !shouldRunStateRuntimeController(state, fighter, targetFacing.id, targetFacing.trigger, opponent, stage)) {
            continue;
        }
        target.facing = targetFacing.value >= 0 ? fighter.facing : -fighter.facing;
        refreshStoredTargetLink(fighter);
    }

    for (const auto& targetLifeAdd : stateDef->targetLifeAdds) {
        if (!targetControllerMatchesStoredTarget(fighter, targetLifeAdd.targetId)
            || !shouldRunStateOneShotController(state, fighter, targetLifeAdd.id, targetLifeAdd.trigger, opponent, stage)) {
            continue;
        }
        const auto value = evalMugenExpression(state, fighter, targetLifeAdd.valueExpression, opponent, stage);
        if (!value) {
            continue;
        }
        const int delta = static_cast<int>(std::lround(*value));
        const int minimumLife = targetLifeAdd.kill ? 0 : 1;
        target.life = std::clamp(target.life + delta, minimumLife, characterMaxLifeForActor(state, target));
        refreshStoredTargetLink(fighter);
    }

    for (const auto& targetPowerAdd : stateDef->targetPowerAdds) {
        if (!targetControllerMatchesStoredTarget(fighter, targetPowerAdd.targetId)
            || !shouldRunStateOneShotController(state, fighter, targetPowerAdd.id, targetPowerAdd.trigger, opponent, stage)) {
            continue;
        }
        const auto value = evalMugenExpression(state, fighter, targetPowerAdd.valueExpression, opponent, stage);
        if (!value) {
            continue;
        }
        target.power = std::clamp(
            target.power + static_cast<int>(std::lround(*value)),
            0,
            std::max(0, characterConstantsForActor(state, target).maxPower));
        refreshStoredTargetLink(fighter);
    }

    for (const auto& targetVelocity : stateDef->targetVelocities) {
        if (!targetControllerMatchesStoredTarget(fighter, targetVelocity.targetId)
            || !shouldRunStateOneShotController(state, fighter, targetVelocity.id, targetVelocity.trigger, opponent, stage)) {
            continue;
        }
        if (targetVelocity.hasX) {
            const auto value = evalMugenExpression(state, fighter, targetVelocity.xExpression, opponent, stage);
            if (value) {
                const float x = *value * static_cast<float>(target.facing);
                if (targetVelocity.add) {
                    target.vx += x;
                } else {
                    target.vx = x;
                }
            }
        }
        if (targetVelocity.hasY) {
            const auto value = evalMugenExpression(state, fighter, targetVelocity.yExpression, opponent, stage);
            if (value) {
                if (targetVelocity.add) {
                    target.vy += *value;
                } else {
                    target.vy = *value;
                }
            }
        }
        refreshStoredTargetLink(fighter);
    }

    for (const auto& targetDrop : stateDef->targetDrops) {
        if (!shouldRunStateRuntimeController(state, fighter, targetDrop.id, targetDrop.trigger, opponent, stage)) {
            continue;
        }
        if (targetDrop.excludeId < 0 || targetDrop.excludeId != fighter.targetHitId) {
            clearFighterTargetLink(fighter);
            return false;
        }
    }
        return true;
    });
}

void applyTargetBindings(AppState& state) {
    for (auto& target : state.fighters) {
        if (!target.targetBindPositionActive
            || target.bindTicks <= 0
            || target.boundByIndex < 0
            || target.boundByIndex >= static_cast<int>(state.fighters.size())) {
            continue;
        }
        const FighterState& binder = state.fighters[static_cast<size_t>(target.boundByIndex)];
        const int bindFacing = target.targetBindFacing == 0 ? binder.facing : target.targetBindFacing;
        target.x = binder.x + target.targetBindOffsetX * static_cast<float>(bindFacing);
        target.y = binder.y + target.targetBindOffsetY;
        target.vx = static_cast<float>(target.facing * binder.facing) * binder.vx;
        target.vy = binder.vy;
        target.onGround = target.y >= 0.0f;
    }
}

CollisionBox collisionBoxToWorldScaled(
    const CollisionBox& box,
    const FighterState& fighter,
    const AnimationFrame& frame,
    float scaleX,
    float scaleY) {
    const bool facingLeft = fighter.facing < 0;
    const bool mirrorX = frame.flipX != facingLeft;
    float x1 = box.x1 * scaleX;
    float x2 = box.x2 * scaleX;
    float y1 = box.y1 * scaleY;
    float y2 = box.y2 * scaleY;

    if (mirrorX) {
        x1 = -box.x2 * scaleX;
        x2 = -box.x1 * scaleX;
    }
    if (frame.flipY) {
        y1 = -box.y2 * scaleY;
        y2 = -box.y1 * scaleY;
    }

    return CollisionBox{
        fighter.x + std::min(x1, x2),
        fighter.y + std::min(y1, y2),
        fighter.x + std::max(x1, x2),
        fighter.y + std::max(y1, y2),
    };
}

bool boxesOverlap(const CollisionBox& a, const CollisionBox& b) {
    return a.x1 < b.x2
        && a.x2 > b.x1
        && a.y1 < b.y2
        && a.y2 > b.y1;
}

bool fighterBoxesOverlap(
    const FighterState& attacker,
    const AnimationFrame& attackFrame,
    const FighterState& defender,
    const AnimationFrame& defendFrame) {
    for (const auto& attackBox : attackFrame.clsn1) {
        const CollisionBox attackWorld = collisionBoxToWorldScaled(
            attackBox,
            attacker,
            attackFrame,
            attacker.scaleX,
            attacker.scaleY);
        for (const auto& hurtBox : defendFrame.clsn2) {
            const CollisionBox hurtWorld = collisionBoxToWorldScaled(
                hurtBox,
                defender,
                defendFrame,
                defender.scaleX,
                defender.scaleY);
            if (boxesOverlap(attackWorld, hurtWorld)) {
                return true;
            }
        }
    }
    return false;
}

bool arenaActorDepthsOverlap(const AppState& state, const FighterState& attacker, const FighterState& defender, float tolerance) {
    if (!arenaDepthActive(state)) {
        return true;
    }
    return std::fabs(attacker.depthZ - defender.depthZ) <= tolerance;
}

bool arenaProjectileDepthOverlapsDefender(const AppState& state, const RuntimeProjectile& projectile, const FighterState& defender) {
    if (!arenaDepthActive(state)) {
        return true;
    }
    return std::fabs(projectile.depthZ - defender.depthZ) <= state.arenaConfig.projectileDepthHitTolerance;
}

std::string_view fighterLabel(size_t fighterIndex) {
    return fighterIndex == 0 ? "P1" : "P2";
}

bool useTrainingDummyOptions(const AppState& state, size_t defenderIndex) {
    return defenderIndex == 1 && activeOpponentType(state) == OpponentType::Dummy;
}

bool shouldPlayFightSounds(const AppState& state) {
    return state.frontend.pendingMode != PendingMode::Training || state.training.options.playHitSounds;
}

#include "AppReversalDefRuntime.h"

int effectiveGuardDistance(const AppState& state, const FighterState& attacker, const HitDefinition& hitDef) {
    if (attacker.attackDistanceOverride >= 0) {
        return attacker.attackDistanceOverride;
    }
    if (hitDef.guardDistance >= 0) {
        return hitDef.guardDistance;
    }
    return std::max(0, characterConstantsForActor(state, attacker).attackDistance);
}

int scaleDamageForDefence(const AppState& state, int damage, const FighterState& defender) {
    if (damage <= 0) {
        return 0;
    }
    const float baseDefence = std::max(1, characterConstantsForActor(state, defender).defence) / 100.0f;
    const float multiplier = std::max(0.001f, baseDefence * defender.defenceMultiplier * defender.modeDefenceMultiplier);
    return std::max(0, static_cast<int>(std::lround(static_cast<float>(damage) / multiplier)));
}

int scaleDamageForAttack(const AppState& state, int damage, const FighterState& attacker) {
    if (damage <= 0) {
        return 0;
    }
    const float baseAttack = std::max(1, characterConstantsForActor(state, attacker).attack) / 100.0f;
    return std::max(0, static_cast<int>(std::lround(
        static_cast<float>(damage) * baseAttack * attacker.attackMultiplier * attacker.modeAttackMultiplier)));
}

int scaleAttackThenDefenceDamage(const AppState& state, int damage, const FighterState& attacker, const FighterState& defender) {
    return scaleDamageForDefence(state, scaleDamageForAttack(state, damage, attacker), defender);
}

void clearComboCounters(AppState& state) {
    state.display.comboCounters = {};
}

void endActiveComboForDefender(AppState& state, size_t defenderIndex) {
    if (defenderIndex >= state.display.comboCounters.size()) {
        return;
    }
    const size_t attackerIndex = defenderIndex == 0 ? 1 : 0;
    auto& combo = state.display.comboCounters[attackerIndex];
    combo.activeHits = 0;
}

void registerComboHit(AppState& state, size_t attackerIndex) {
    if (attackerIndex >= state.display.comboCounters.size()) {
        return;
    }

    auto& combo = state.display.comboCounters[attackerIndex];
    ++combo.activeHits;
    combo.displayHits = combo.activeHits;
    combo.displayTicks = std::max(1, state.fightRoundSettings.combo.displayTime);
}

void updateComboCounterBreaks(AppState& state) {
    for (size_t i = 0; i < state.fighters.size(); ++i) {
        const auto& defender = state.fighters[i];
        if (defender.moveType != 'H' && !defender.guarding) {
            endActiveComboForDefender(state, i);
        }
    }
}

void updateComboDisplayTimers(AppState& state) {
    for (auto& combo : state.display.comboCounters) {
        if (combo.displayTicks <= 0) {
            combo.activeHits = 0;
            combo.displayHits = 0;
            continue;
        }
        --combo.displayTicks;
        if (combo.displayTicks <= 0) {
            combo.activeHits = 0;
            combo.displayHits = 0;
        }
    }
}

void applyHitBetween(AppState& state, size_t attackerIndex, size_t defenderIndex) {
    if (defenderIndex >= state.fighters.size()) {
        return;
    }

    FighterState* attackerPtr = nullptr;
    if (attackerIndex < state.fighters.size()) {
        if (attackerIndex == defenderIndex) {
            return;
        }
        attackerPtr = &state.fighters[attackerIndex];
    } else {
        const size_t helperIndex = attackerIndex - state.fighters.size();
        if (helperIndex >= state.helpers.size() || state.helpers[helperIndex].destroyRequested) {
            return;
        }
        attackerPtr = &state.helpers[helperIndex];
        if (attackerPtr->ownerIndex == static_cast<int>(defenderIndex)) {
            return;
        }
    }

    auto& attacker = *attackerPtr;
    auto& defender = state.fighters[defenderIndex];
    const size_t comboAttackerIndex = attacker.helper && attacker.ownerIndex >= 0
        ? static_cast<size_t>(attacker.ownerIndex)
        : attackerIndex;
    const int attackerStateOwnerIndex = actorClipOwnerIndex(state, attacker);
    const float arenaDepthTolerance = attacker.helper
        ? state.arenaConfig.projectileDepthHitTolerance
        : state.arenaConfig.fighterDepthHitTolerance;
    if (!arenaActorDepthsOverlap(state, attacker, defender, arenaDepthTolerance)) {
        return;
    }

    const AnimationFrame* attackFrame = currentFrameForFighter(state, attacker);
    const AnimationFrame* defendFrame = currentFrameForFighter(state, defender);
    if (!attackFrame || !defendFrame || attackFrame->clsn1.empty() || defendFrame->clsn2.empty()) {
        return;
    }

    if (!fighterBoxesOverlap(attacker, *attackFrame, defender, *defendFrame)) {
        return;
    }

    const HitDefinition* hitDef = findActiveHitDefinition(state, attacker, defender, defenderIndex);
    if (!hitDef) {
        return;
    }
    const HitDefinition resolvedHitDef = resolveHitDefinitionExpressions(
        state,
        *hitDef,
        attacker,
        defender,
        selectedStageSlot(state.selection));
    hitDef = &resolvedHitDef;

    if (const auto* reversal = activeReversalDefForDefender(state, defender, attacker, *hitDef, selectedStageSlot(state.selection))) {
        applyReversalDef(state, attacker, defender, *hitDef, *reversal, defenderIndex, comboAttackerIndex);
        return;
    }

    const bool trainingDummy = useTrainingDummyOptions(state, defenderIndex);
    GuardStance guardStance = trainingDummy
        ? chooseDummyGuardStance(state.training.options, *hitDef, defender)
        : choosePlayerGuardStance(*hitDef, defender);
    if (guardStance != GuardStance::None
        && p2BodyDistXValue(state, attacker, defender) > static_cast<float>(effectiveGuardDistance(state, attacker, *hitDef))) {
        guardStance = GuardStance::None;
    }
    const StateHitOverrideController* hitOverride = guardStance == GuardStance::None
        ? activeHitOverrideForDefender(state, defender, &attacker, *hitDef)
        : nullptr;

    markHitDefApplied(attacker, hitDef->id, currentAnimElemForFighter(state, attacker), defenderIndex);
    attacker.moveContact = true;
    ++attacker.hitCount;
    startEnvShake(state, hitDef->envShake);
    startPaletteEffect(defender.paletteEffect, hitDef->palFx);
    if (!hitOverride) {
        attacker.targetIndex = static_cast<int>(defenderIndex);
        attacker.targetHitId = hitDef->targetId;
        attacker.targetTicks = std::max(attacker.targetTicks, kStoredTargetLinkTicks);
    }
    std::ostringstream hitText;

    if (guardStance != GuardStance::None) {
        attacker.moveGuarded = true;
        endActiveComboForDefender(state, defenderIndex);
        attacker.hitPauseTicks = fightHitPauseTicks(state, hitDef->pausetimeP1, 1);
        const int guardDamageDone = state.training.options.guardDamage
            ? scaleAttackThenDefenceDamage(state, hitDef->guardDamage, attacker, defender)
            : 0;
        if (!state.training.options.dummyInvincible && state.training.options.guardDamage) {
            defender.life = std::max(0, defender.life - guardDamageDone);
        }
        if (state.training.options.dummyFrozen) {
            clearFighterHitRuntime(defender);
            enterState(state, defender, 0);
        } else {
            enterGroundGuardHitState(state, defender, *hitDef, attacker.facing, guardStance);
        }
        spawnGuardSpark(state, *hitDef, attacker, defender);
        if (shouldPlayFightSounds(state)) {
            playSound(state, hitDef->guardSoundGroup, hitDef->guardSoundIndex, hitDef->guardSoundForceCommon, -1, false, 1.0f, false, static_cast<int>(comboAttackerIndex));
        }

        hitText << fighterLabel(comboAttackerIndex) << " guard " << hitDef->stateNo << "#" << hitDef->id
                << " gdmg " << guardDamageDone
                << " flag " << hitDef->guardflag
                << " mode " << (guardStance == GuardStance::Crouch ? "C" : "S")
                << " spark " << hitDef->guardSparkNo
                << " snd " << soundPairText(hitDef->guardSoundGroup, hitDef->guardSoundIndex);
    } else {
        attacker.moveHit = true;
        attacker.hitPauseTicks = fightHitPauseTicks(state, hitDef->pausetimeP1, 1);
        const int damageDone = (!trainingDummy || !state.training.options.dummyInvincible)
            ? scaleAttackThenDefenceDamage(state, hitDef->damage, attacker, defender)
            : 0;
        if (damageDone > 0) {
            defender.life = std::max(0, defender.life - damageDone);
        }
        applyHitDefP1Facing(attacker, *hitDef);
        if (trainingDummy && state.training.options.dummyFrozen) {
            clearFighterHitRuntime(defender);
            enterState(state, defender, 0);
        } else if (hitOverride) {
            const bool wasAirborne = !defender.onGround || defender.stateType == 'A';
            const bool wasLyingDown = fighterIsLyingDownForHit(defender);
            const float downVelocityX = (hitDef->hasDownVelocity ? hitDef->downVelocityX : hitDef->airVelocityX)
                * -static_cast<float>(attacker.facing);
            const float downVelocityY = hitDef->hasDownVelocity ? hitDef->downVelocityY : hitDef->airVelocityY;
            enterState(state, defender, hitOverride->stateNo);
            defender.customHitState = true;
            defender.moveType = 'H';
            defender.ctrl = false;
            defender.hitPauseTicks = fightHitPauseTicks(state, hitDef->pausetimeP2, 0);
            defender.hitStunTicks = std::max(hitDef->groundHitTime, defender.hitPauseTicks);
            setGetHitVarsFromHitDef(
                defender,
                *hitDef,
                wasAirborne,
                wasLyingDown,
                hitDefCausesFall(*hitDef, wasAirborne),
                -hitDef->groundVelocityX * static_cast<float>(attacker.facing),
                hitDef->groundVelocityY,
                downVelocityX,
                downVelocityY);
            applyHitDefP2Facing(defender, *hitDef, attacker.facing);
        } else if (enterCustomHitState(state, defender, *hitDef, attacker.facing, attackerStateOwnerIndex)) {
            // Custom p2stateno states are driven by the character CNS after entry.
        } else {
            enterGroundGetHitState(state, defender, *hitDef, attacker.x, attacker.y, attacker.facing);
        }
        if (hitDef->p1StateNo >= 0 && enterState(state, attacker, hitDef->p1StateNo)) {
            attacker.ctrl = false;
        }
        spawnHitSpark(state, *hitDef, attacker, defender);
        if (shouldPlayFightSounds(state)) {
            playSound(state, hitDef->hitSoundGroup, hitDef->hitSoundIndex, hitDef->hitSoundForceCommon, -1, false, 1.0f, false, static_cast<int>(comboAttackerIndex));
        }
        registerComboHit(state, comboAttackerIndex);

        hitText << fighterLabel(comboAttackerIndex) << " hit " << hitDef->stateNo << "#" << hitDef->id
                << " dmg " << damageDone
                << " attr " << hitDef->attr
                << " hit " << hitDef->groundHitTime
                << " spark " << hitDef->sparkNo
                << " snd " << soundPairText(hitDef->hitSoundGroup, hitDef->hitSoundIndex);
    }
    state.messages.lastHitText = hitText.str();
    state.messages.lastHitTextTicks = 150;
    logFightHitEvent(state.messages.lastHitText);
}

bool trainingCommandDemoActive(const AppState& state);

void applyHitIfNeeded(AppState& state) {
    FramePerfScope scope(state.framePerf, FramePerfSection::CollisionHitRouting);
    applyHitBetween(state, 0, 1);
    const bool trainingDummyCanAttackPlayer =
        state.frontend.pendingMode == PendingMode::Training
        && activeOpponentType(state) == OpponentType::Dummy
        && (trainingCommandDemoActive(state) || state.training.options.p2Controlled);
    if (activeOpponentType(state) != OpponentType::Dummy || trainingDummyCanAttackPlayer) {
        applyHitBetween(state, 1, 0);
    }
    const size_t helperBase = state.fighters.size();
    for (size_t i = 0; i < state.helpers.size(); ++i) {
        const auto& helper = state.helpers[i];
        if (helper.destroyRequested || helper.ownerIndex < 0 || helper.ownerIndex >= static_cast<int>(state.fighters.size())) {
            continue;
        }
        const size_t defenderIndex = helper.ownerIndex == 0 ? 1 : 0;
        applyHitBetween(state, helperBase + i, defenderIndex);
    }
}

#include "ArenaModeCombat.h"
