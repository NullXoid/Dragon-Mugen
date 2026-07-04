#pragma once

// Internal App.cpp implementation shard.
// Movement, push, camera, environment effect, and spark runtime helpers.

void resetFighterOneTickBounds(AppState& state) {
    auto resetActor = [](FighterState& fighter) {
        fighter.transEffect = {};
        fighter.angleDrawActive = false;
        fighter.displayOffsetX = 0.0f;
        fighter.displayOffsetY = 0.0f;
        if (fighter.projectileHitTicks > 0) {
            --fighter.projectileHitTicks;
            if (fighter.projectileHitTicks <= 0) {
                fighter.projectileHitId = -1;
            }
        }
        if (fighter.projectileContactTicks > 0) {
            --fighter.projectileContactTicks;
            if (fighter.projectileContactTicks <= 0) {
                fighter.projectileContactId = -1;
            }
        }
        if (fighter.projectileGuardedTicks > 0) {
            --fighter.projectileGuardedTicks;
            if (fighter.projectileGuardedTicks <= 0) {
                fighter.projectileGuardedId = -1;
            }
        }
        fighter.screenBound = true;
        fighter.screenBoundMoveCameraX = false;
        fighter.screenBoundMoveCameraY = false;
        fighter.playerPush = true;
        fighter.posFreezeX = false;
        fighter.posFreezeY = false;
        fighter.triggerY = fighter.y;
        fighter.edgeWidthFront = -1.0f;
        fighter.edgeWidthBack = -1.0f;
        fighter.playerWidthFront = -1.0f;
        fighter.playerWidthBack = -1.0f;
        if (fighter.notHitByTicks > 0) {
            --fighter.notHitByTicks;
            if (fighter.notHitByTicks <= 0) {
                fighter.notHitByValue.clear();
            }
        }
        if (fighter.hitByTicks > 0) {
            --fighter.hitByTicks;
            if (fighter.hitByTicks <= 0) {
                fighter.hitByValue.clear();
            }
        }
        if (fighter.targetTicks > 0) {
            --fighter.targetTicks;
            if (fighter.targetTicks <= 0) {
                fighter.targetIndex = -1;
                fighter.targetHitId = -1;
            }
        }
        if (fighter.bindTicks > 0) {
            --fighter.bindTicks;
            if (fighter.bindTicks <= 0) {
                fighter.boundByIndex = -1;
                fighter.targetBindPositionActive = false;
                fighter.targetBindOffsetX = 0.0f;
                fighter.targetBindOffsetY = 0.0f;
                fighter.targetBindFacing = 0;
            }
        }
    };

    for (auto& fighter : state.fighters) {
        resetActor(fighter);
    }
    for (auto& helper : state.helpers) {
        resetActor(helper);
    }
}

void tickFightRuntimeControllerTracking(AppState& state) {
    for (auto& fighter : state.fighters) {
        tickStateRuntimeControllerTracking(fighter);
    }
    for (auto& helper : state.helpers) {
        tickStateRuntimeControllerTracking(helper);
    }
}

float fighterBaseFrontWidth(const AppState& state, const FighterState& fighter) {
    const CharacterConstants& constants = characterConstantsForActor(state, fighter);
    return fighter.onGround ? constants.size.groundFront : constants.size.airFront;
}

float fighterBaseBackWidth(const AppState& state, const FighterState& fighter) {
    const CharacterConstants& constants = characterConstantsForActor(state, fighter);
    return fighter.onGround ? constants.size.groundBack : constants.size.airBack;
}

float fighterPlayerFrontWidth(const AppState& state, const FighterState& fighter) {
    return std::max(0.0f, fighter.playerWidthFront >= 0.0f ? fighter.playerWidthFront : fighterBaseFrontWidth(state, fighter));
}

float fighterPlayerBackWidth(const AppState& state, const FighterState& fighter) {
    return std::max(0.0f, fighter.playerWidthBack >= 0.0f ? fighter.playerWidthBack : fighterBaseBackWidth(state, fighter));
}

float fighterEdgeFrontWidth(const AppState& state, const FighterState& fighter) {
    return std::max(0.0f, fighter.edgeWidthFront >= 0.0f ? fighter.edgeWidthFront : fighterBaseFrontWidth(state, fighter));
}

float fighterEdgeBackWidth(const AppState& state, const FighterState& fighter) {
    return std::max(0.0f, fighter.edgeWidthBack >= 0.0f ? fighter.edgeWidthBack : fighterBaseBackWidth(state, fighter));
}

float fighterWidthTowardDirection(float frontWidth, float backWidth, const FighterState& fighter, float direction) {
    const bool directionIsFront = direction * static_cast<float>(fighter.facing) > 0.0f;
    return directionIsFront ? frontWidth : backWidth;
}

float fighterPlayerWidthToward(const AppState& state, const FighterState& fighter, float direction) {
    return fighterWidthTowardDirection(
        fighterPlayerFrontWidth(state, fighter),
        fighterPlayerBackWidth(state, fighter),
        fighter,
        direction);
}

float fighterEdgeWidthToward(const AppState& state, const FighterState& fighter, float direction) {
    return fighterWidthTowardDirection(
        fighterEdgeFrontWidth(state, fighter),
        fighterEdgeBackWidth(state, fighter),
        fighter,
        direction);
}

float clampFighterOriginToStage(float x, const StageSlot& stage) {
    return std::clamp(x, stage.leftbound, stage.rightbound);
}

struct FighterVisualScreenBounds {
    float left = 0.0f;
    float right = 0.0f;
    bool valid = false;
};

FighterVisualScreenBounds fighterVisualScreenBounds(const AppState& state, const StageSlot& stage, const FighterState& fighter) {
    const AnimationFrame* frame = currentFrameForFighter(state, fighter);
    if (!frame || !frame->sprite.texture || frame->sprite.width <= 0) {
        return {};
    }

    const ArenaProjectedPoint projected = projectArenaWorldPoint(state, stage, fighter.x, fighter.y, fighter.depthZ);
    const float displayOriginX = projected.screenX + fighter.displayOffsetX * static_cast<float>(fighter.facing);
    const bool facingLeft = fighter.facing < 0;
    const float drawLeft = facingLeft
        ? displayOriginX
            - static_cast<float>(frame->offsetX) * fighter.scaleX
            - static_cast<float>(frame->sprite.width - frame->sprite.axisX) * fighter.scaleX
        : displayOriginX
            + static_cast<float>(frame->offsetX) * fighter.scaleX
            - static_cast<float>(frame->sprite.axisX) * fighter.scaleX;
    const float drawRight = drawLeft + static_cast<float>(frame->sprite.width) * fighter.scaleX;
    return FighterVisualScreenBounds{
        std::min(drawLeft, drawRight),
        std::max(drawLeft, drawRight),
        true,
    };
}

FighterVisualScreenBounds fighterVisualOriginExtents(const AppState& state, const FighterState& fighter) {
    const AnimationFrame* frame = currentFrameForFighter(state, fighter);
    if (!frame || !frame->sprite.texture || frame->sprite.width <= 0) {
        return {};
    }

    const float displayOffsetX = fighter.displayOffsetX * static_cast<float>(fighter.facing);
    const bool facingLeft = fighter.facing < 0;
    const float left = facingLeft
        ? displayOffsetX
            - static_cast<float>(frame->offsetX) * fighter.scaleX
            - static_cast<float>(frame->sprite.width - frame->sprite.axisX) * fighter.scaleX
        : displayOffsetX
            + static_cast<float>(frame->offsetX) * fighter.scaleX
            - static_cast<float>(frame->sprite.axisX) * fighter.scaleX;
    const float right = left + static_cast<float>(frame->sprite.width) * fighter.scaleX;
    return FighterVisualScreenBounds{
        std::min(left, right),
        std::max(left, right),
        true,
    };
}

void applyScreenBounds(AppState& state, const StageSlot& stage) {
    const float halfWidth = logicalWidthF(state) * 0.5f;
    const float visibleLeft = state.cameraX - halfWidth;
    const float visibleRight = state.cameraX + halfWidth;

    for (auto& fighter : state.fighters) {
        if (!fighter.screenBound) {
            continue;
        }

        const float widthLeft = fighterEdgeWidthToward(state, fighter, -1.0f);
        const float widthRight = fighterEdgeWidthToward(state, fighter, 1.0f);
        float minX = std::max(stage.leftbound, visibleLeft + widthLeft - stage.screenleft);
        float maxX = std::min(stage.rightbound, visibleRight - widthRight + stage.screenright);

        const FighterVisualScreenBounds visual = fighterVisualOriginExtents(state, fighter);
        if (visual.valid) {
            const float visualMinX = visibleLeft - visual.left;
            const float visualMaxX = visibleRight - visual.right;
            if (visualMinX <= visualMaxX) {
                minX = std::max(minX, visualMinX);
                maxX = std::min(maxX, visualMaxX);
            }
        }
        if (minX > maxX) {
            continue;
        }
        fighter.x = std::clamp(fighter.x, minX, maxX);
    }
}

int airMovementBaseAction(const FighterState& fighter) {
    if (fighter.jumpBaseAction >= 41 && fighter.jumpBaseAction <= 43) {
        return fighter.jumpBaseAction;
    }

    const float directionalVelocity = fighter.vx * static_cast<float>(fighter.facing);
    if (directionalVelocity > 0.05f) {
        return 42;
    }
    if (directionalVelocity < -0.05f) {
        return 43;
    }
    return 41;
}

int chooseMovementAction(const AppState& state, const FighterState& fighter) {
    if (!fighter.onGround) {
        const int baseAction = airMovementBaseAction(fighter);
        const int peakAction = baseAction + 3;
        if (fighter.jumpPeakActionApplied && findExactClipForActor(state, fighter, peakAction)) {
            return peakAction;
        }
        return baseAction;
    }

    if (std::fabs(fighter.vx) > 0.05f) {
        return fighter.vx * static_cast<float>(fighter.facing) > 0.0f ? 20 : 21;
    }
    if (arenaDepthActive(state) && std::fabs(fighter.depthVz) > 0.05f) {
        return 20;
    }
    return 0;
}

void updateFighterFacing(AppState& state) {
    state.fighters[0].facing = state.fighters[0].x <= state.fighters[1].x ? 1 : -1;
    state.fighters[1].facing = -state.fighters[0].facing;
}

void updateArenaFighterFacing(AppState& state) {
    for (size_t i = 0; i < state.fighters.size(); ++i) {
        if (state.fighters[i].life <= 0) {
            continue;
        }
        const int target = nearestLivingEnemyIndex(state, static_cast<int>(i));
        if (target >= 0) {
            state.fighters[i].facing = state.fighters[i].x <= state.fighters[static_cast<size_t>(target)].x ? 1 : -1;
        }
    }
}

bool shouldDeferCommonLandingToAuthoredAirChangeState(const AppState& state, const FighterState& fighter) {
    if (fighter.helper
        || fighter.stateType != 'A'
        || fighter.physics != 'A'
        || fighter.moveType == 'H') {
        return false;
    }
    const StateDefinition* stateDef = findStateDefinitionForActor(state, fighter, fighter.stateNo);
    return stateDef && !stateDef->changeStates.empty();
}

void updateFighterPhysics(const AppState& state, FighterState& fighter, const StageSlot& stage) {
    if (!fighter.posFreezeX) {
        fighter.x = std::clamp(fighter.x + fighter.vx, stage.leftbound, stage.rightbound);
    }
    if (!fighter.posFreezeY) {
        fighter.y += fighter.vy;
    }
    fighter.triggerY = fighter.y;
    if (!fighter.posFreezeY && !fighter.onGround && fighter.physics == 'A') {
        const CharacterConstants& constants = characterConstantsForActor(state, fighter);
        fighter.vy += fighter.hitFall && fighter.hitFallYAccel > 0.0f
            ? fighter.hitFallYAccel
            : constants.movementYAccel;
    }
    if (arenaDepthActive(state)) {
        fighter.depthZ = std::clamp(
            fighter.depthZ + fighter.depthVz,
            state.arenaConfig.depthMin,
            state.arenaConfig.depthMax);
    } else if (isArenaMode(state)) {
        fighter.depthZ = 0.0f;
        fighter.depthVz = 0.0f;
    }
    const bool authoredAirStateHandlesFloor =
        fighter.stateType == 'A'
        && fighter.physics != 'A'
        && fighter.moveType != 'H';
    const bool deferCommonLanding = shouldDeferCommonLandingToAuthoredAirChangeState(state, fighter);
    if (fighter.physics != 'N' && fighter.y >= 0.0f && !authoredAirStateHandlesFloor && !deferCommonLanding) {
        const bool shouldUseCommonLanding =
            !fighter.helper
            && fighter.stateType == 'A'
            && fighter.physics == 'A'
            && fighter.moveType != 'H';
        fighter.y = 0.0f;
        fighter.vy = 0.0f;
        fighter.onGround = true;
        fighter.jumpBaseAction = 0;
        fighter.jumpPeakActionApplied = false;
        if (shouldUseCommonLanding) {
            enterCommonLandingState(state, fighter);
            return;
        }
    }
}

void applyPlayerPush(AppState& state, const StageSlot& stage) {
    auto& p1 = state.fighters[0];
    auto& p2 = state.fighters[1];
    if (!p1.onGround
        || !p2.onGround
        || !p1.playerPush
        || !p2.playerPush
        || p1.life <= 0
        || p2.life <= 0) {
        return;
    }

    float delta = p2.x - p1.x;
    const float distance = std::fabs(delta);
    if (distance < 0.001f) {
        delta = p1.facing >= 0 ? 1.0f : -1.0f;
    }
    const float direction = delta >= 0.0f ? 1.0f : -1.0f;
    const float minSeparation =
        fighterPlayerWidthToward(state, p1, direction)
        + fighterPlayerWidthToward(state, p2, -direction);
    if (distance >= minSeparation) {
        return;
    }

    const float overlap = minSeparation - distance;

    if (state.frontend.pendingMode == PendingMode::Training && activeOpponentType(state) == OpponentType::Dummy) {
        p1.x = clampFighterOriginToStage(p1.x - direction * overlap, stage);
        return;
    }

    p1.x = clampFighterOriginToStage(p1.x - direction * (overlap * 0.5f), stage);
    p2.x = clampFighterOriginToStage(p2.x + direction * (overlap * 0.5f), stage);

    const float adjustedDelta = p2.x - p1.x;
    const float adjustedDistance = std::fabs(adjustedDelta);
    if (adjustedDistance >= minSeparation) {
        return;
    }

    const float remaining = minSeparation - adjustedDistance;
    if (p1.x <= stage.leftbound + 0.001f) {
        p2.x = clampFighterOriginToStage(p2.x + direction * remaining, stage);
    } else if (p2.x >= stage.rightbound - 0.001f) {
        p1.x = clampFighterOriginToStage(p1.x - direction * remaining, stage);
    }
}

void applyArenaPlayerPush(AppState& state, const StageSlot& stage) {
    for (size_t lhs = 0; lhs < state.fighters.size(); ++lhs) {
        auto& p1 = state.fighters[lhs];
        if (!p1.onGround || !p1.playerPush || p1.life <= 0) {
            continue;
        }
        for (size_t rhs = lhs + 1; rhs < state.fighters.size(); ++rhs) {
            auto& p2 = state.fighters[rhs];
            if (!p2.onGround || !p2.playerPush || p2.life <= 0) {
                continue;
            }

            float delta = p2.x - p1.x;
            const float distance = std::fabs(delta);
            if (distance < 0.001f) {
                delta = p1.facing >= 0 ? 1.0f : -1.0f;
            }
            const float direction = delta >= 0.0f ? 1.0f : -1.0f;
            const float minSeparation =
                fighterPlayerWidthToward(state, p1, direction)
                + fighterPlayerWidthToward(state, p2, -direction);
            if (distance >= minSeparation) {
                continue;
            }

            const float overlapX = minSeparation - distance;
            if (arenaDepthActive(state)) {
                const float depthDelta = p2.depthZ - p1.depthZ;
                const float depthDistance = std::fabs(depthDelta);
                const float minDepthSeparation = std::max(1.0f, state.arenaConfig.fighterDepthHitTolerance);
                if (depthDistance >= minDepthSeparation) {
                    continue;
                }

                const float depthDirection = depthDistance < 0.001f
                    ? (lhs % 2 == 0 ? -1.0f : 1.0f)
                    : (depthDelta >= 0.0f ? 1.0f : -1.0f);
                const float overlapZ = minDepthSeparation - depthDistance;
                const bool separateInDepth = depthDistance > 0.001f && overlapZ < overlapX;
                if (separateInDepth) {
                    p1.depthZ = std::clamp(
                        p1.depthZ - depthDirection * (overlapZ * 0.5f),
                        state.arenaConfig.depthMin,
                        state.arenaConfig.depthMax);
                    p2.depthZ = std::clamp(
                        p2.depthZ + depthDirection * (overlapZ * 0.5f),
                        state.arenaConfig.depthMin,
                        state.arenaConfig.depthMax);
                    continue;
                }
            }

            p1.x = clampFighterOriginToStage(p1.x - direction * (overlapX * 0.5f), stage);
            p2.x = clampFighterOriginToStage(p2.x + direction * (overlapX * 0.5f), stage);
        }
    }
}

float arenaCameraRotationSourceDepth(const AppState& state) {
    if (!state.fighters.empty() && state.fighters[0].life > 0) {
        return state.fighters[0].depthZ;
    }

    float totalDepth = 0.0f;
    int living = 0;
    for (const auto& fighter : state.fighters) {
        if (fighter.life <= 0) {
            continue;
        }
        totalDepth += fighter.depthZ;
        ++living;
    }
    return living > 0 ? totalDepth / static_cast<float>(living) : 0.0f;
}

void updateArenaCameraRotation(AppState& state) {
    if (!arenaCameraRotationActive(state)) {
        state.arenaCameraYawDeg = 0.0f;
        state.arenaCameraTargetYawDeg = 0.0f;
        return;
    }

    const float sourceDepth = arenaCameraRotationSourceDepth(state);
    const float depthExtent = arenaRotationDepthExtent(state);
    state.arenaCameraTargetYawDeg = std::clamp(
        -sourceDepth / depthExtent * state.arenaConfig.cameraRotationMaxYawDeg,
        -state.arenaConfig.cameraRotationMaxYawDeg,
        state.arenaConfig.cameraRotationMaxYawDeg);
    state.arenaCameraYawDeg += (state.arenaCameraTargetYawDeg - state.arenaCameraYawDeg)
        * state.arenaConfig.cameraRotationEase;
    if (std::fabs(state.arenaCameraYawDeg) < 0.001f && std::fabs(state.arenaCameraTargetYawDeg) < 0.001f) {
        state.arenaCameraYawDeg = 0.0f;
        state.arenaCameraTargetYawDeg = 0.0f;
    }
}

void updateCamera(AppState& state, const StageSlot& stage) {
    const float minFighterX = std::min(state.fighters[0].x, state.fighters[1].x);
    const float maxFighterX = std::max(state.fighters[0].x, state.fighters[1].x);
    const float halfWidth = logicalWidthF(state) * 0.5f;
    const float leftEdge = state.cameraX - halfWidth + stage.cameraTension;
    const float rightEdge = state.cameraX + halfWidth - stage.cameraTension;

    float targetX = state.cameraX;
    if (minFighterX < leftEdge) {
        targetX += minFighterX - leftEdge;
    }
    if (maxFighterX > rightEdge) {
        targetX += maxFighterX - rightEdge;
    }
    state.cameraX = std::clamp(targetX, stage.cameraBoundleft, stage.cameraBoundright);

    const float highestY = std::min(state.fighters[0].y, state.fighters[1].y);
    float targetY = stage.cameraStarty;
    if (highestY < -stage.cameraFloortension) {
        targetY = (highestY + stage.cameraFloortension) * stage.cameraVerticalfollow;
    }
    state.cameraY = std::clamp(targetY, stage.cameraBoundhigh, stage.cameraBoundlow);
}

bool arenaOpenBorScrollerActive(const AppState& state, const StageSlot& stage) {
    return isArenaMode(state) && stage.openborScrolling;
}

bool arenaFighterCanUseForcedWalkAction(const FighterState& fighter) {
    return fighter.life > 0
        && fighter.stateNo == 0
        && fighter.moveType == 'I'
        && fighter.onGround
        && !fighter.guarding
        && !fighter.customHitState
        && fighter.hitPauseTicks <= 0;
}

void setArenaForcedWalkActionForDelta(const AppState& state, FighterState& fighter, float deltaX) {
    if (!arenaFighterCanUseForcedWalkAction(fighter) || std::fabs(deltaX) <= 0.05f) {
        return;
    }

    const int action = deltaX * static_cast<float>(fighter.facing) >= 0.0f ? 20 : 21;
    if (findExactClipForActor(state, fighter, action)) {
        setFighterAction(fighter, action);
    }
}

void updateArenaOpenBorScrollingCamera(AppState& state, const StageSlot& stage) {
    const float minCamera = std::max(stage.cameraBoundleft, std::min(stage.openborScrollStartx, stage.openborScrollEndx));
    const float maxCamera = std::min(stage.cameraBoundright, std::max(stage.openborScrollStartx, stage.openborScrollEndx));
    if (minCamera > maxCamera) {
        updateCamera(state, stage);
        return;
    }

    state.cameraX = std::clamp(state.cameraX, minCamera, maxCamera);

    bool any = false;
    float maxFighterX = 0.0f;
    for (const auto& fighter : state.fighters) {
        if (fighter.life <= 0) {
            continue;
        }
        maxFighterX = any ? std::max(maxFighterX, fighter.x) : fighter.x;
        any = true;
    }

    if (any) {
        const float halfWidth = logicalWidthF(state) * 0.5f;
        const float leadEdge = state.cameraX + halfWidth - stage.openborScrollLead;
        float targetX = state.cameraX;
        if (maxFighterX > leadEdge) {
            targetX += maxFighterX - leadEdge;
        }
        targetX = std::clamp(targetX, minCamera, maxCamera);
        if (targetX > state.cameraX) {
            state.cameraX = std::min(targetX, state.cameraX + stage.openborScrollSpeed);
        }
    }

    state.cameraY = std::clamp(stage.cameraStarty, stage.cameraBoundhigh, stage.cameraBoundlow);
}

void applyArenaScreenBounds(AppState& state, const StageSlot& stage) {
    if (!arenaOpenBorScrollerActive(state, stage)) {
        applyScreenBounds(state, stage);
        return;
    }

    std::vector<float> beforeX;
    beforeX.reserve(state.fighters.size());
    for (const auto& fighter : state.fighters) {
        beforeX.push_back(fighter.x);
    }

    applyScreenBounds(state, stage);

    for (size_t i = 0; i < state.fighters.size() && i < beforeX.size(); ++i) {
        setArenaForcedWalkActionForDelta(state, state.fighters[i], state.fighters[i].x - beforeX[i]);
    }
}

void updateArenaCamera(AppState& state, const StageSlot& stage) {
    if (arenaOpenBorScrollerActive(state, stage)) {
        updateArenaOpenBorScrollingCamera(state, stage);
        updateArenaCameraRotation(state);
        return;
    }

    bool any = false;
    float minFighterX = 0.0f;
    float maxFighterX = 0.0f;
    float highestY = 0.0f;
    for (const auto& fighter : state.fighters) {
        if (fighter.life <= 0) {
            continue;
        }
        if (!any) {
            minFighterX = fighter.x;
            maxFighterX = fighter.x;
            highestY = fighter.y;
            any = true;
            continue;
        }
        minFighterX = std::min(minFighterX, fighter.x);
        maxFighterX = std::max(maxFighterX, fighter.x);
        highestY = std::min(highestY, fighter.y);
    }
    if (!any) {
        updateCamera(state, stage);
        updateArenaCameraRotation(state);
        return;
    }

    const float halfWidth = logicalWidthF(state) * 0.5f;
    const float leftEdge = state.cameraX - halfWidth + stage.cameraTension;
    const float rightEdge = state.cameraX + halfWidth - stage.cameraTension;

    float targetX = state.cameraX;
    if (minFighterX < leftEdge) {
        targetX += minFighterX - leftEdge;
    }
    if (maxFighterX > rightEdge) {
        targetX += maxFighterX - rightEdge;
    }
    state.cameraX = std::clamp(targetX, stage.cameraBoundleft, stage.cameraBoundright);

    state.cameraY = std::clamp(stage.cameraStarty, stage.cameraBoundhigh, stage.cameraBoundlow);
    updateArenaCameraRotation(state);
}

void startEnvShake(AppState& state, const EnvShakeSpec& shake) {
    if (!shake.enabled || shake.time <= 0 || std::abs(shake.amplitude) <= 0.001f) {
        return;
    }
    if (shake.time < state.display.envShakeTicks && std::abs(shake.amplitude) <= std::abs(state.display.envShakeAmplitude)) {
        return;
    }
    state.display.envShakeTicks = shake.time;
    state.display.envShakeTotalTicks = shake.time;
    state.display.envShakeFrequency = std::max(1, shake.frequency);
    state.display.envShakeAmplitude = shake.amplitude;
    state.display.envShakePhase = shake.phase;
}

void startPaletteEffect(ActivePaletteEffect& active, const PaletteEffectSpec& effect) {
    if (!effect.enabled || effect.time == 0) {
        return;
    }
    active.spec = effect;
    active.elapsedTicks = 0;
    active.ticksLeft = effect.time < 0 ? 999999 : std::max(1, effect.time);
}

void updatePaletteEffect(ActivePaletteEffect& active) {
    if (active.ticksLeft <= 0) {
        active = {};
        return;
    }
    ++active.elapsedTicks;
    if (active.spec.time >= 0) {
        --active.ticksLeft;
        if (active.ticksLeft <= 0) {
            active = {};
        }
    }
}

void startEnvColor(AppState& state, const StateEnvColorController& envColor) {
    state.envColor.ticksLeft = std::max(1, envColor.time);
    state.envColor.r = std::clamp(envColor.r, 0, 255);
    state.envColor.g = std::clamp(envColor.g, 0, 255);
    state.envColor.b = std::clamp(envColor.b, 0, 255);
}

void updateEnvColor(AppState& state) {
    if (state.envColor.ticksLeft <= 0) {
        state.envColor = {};
        return;
    }
    --state.envColor.ticksLeft;
    if (state.envColor.ticksLeft <= 0) {
        state.envColor = {};
    }
}

void updateEnvShake(AppState& state) {
    if (state.display.envShakeTicks <= 0) {
        state.display.envShakeTicks = 0;
        state.display.envShakeOffsetY = 0.0f;
        return;
    }

    const int elapsed = std::max(0, state.display.envShakeTotalTicks - state.display.envShakeTicks);
    const float progress = state.display.envShakeTotalTicks > 0
        ? static_cast<float>(state.display.envShakeTicks) / static_cast<float>(state.display.envShakeTotalTicks)
        : 0.0f;
    constexpr float tau = 6.28318530718f;
    const float phase = (static_cast<float>(elapsed + state.display.envShakePhase) * static_cast<float>(state.display.envShakeFrequency) / 60.0f) * tau;
    state.display.envShakeOffsetY = std::sin(phase) * state.display.envShakeAmplitude * progress;
    --state.display.envShakeTicks;
}

std::string soundPairText(int group, int index) {
    if (group < 0 || index < 0) {
        return "-";
    }
    return std::to_string(group) + "," + std::to_string(index);
}

void spawnContactSpark(AppState& state, int action, const HitDefinition& hitDef, const FighterState& attacker, const FighterState& target) {
    if (action < 0 || !findFightFxClip(state, action)) {
        return;
    }

    RuntimeEffect effect;
    effect.action = action;
    effect.x = target.x + (hitDef.sparkX * static_cast<float>(attacker.facing));
    effect.y = attacker.y + hitDef.sparkY;
    effect.depthZ = target.depthZ;
    state.runtimeEffects.push_back(effect);
}

void spawnHitSpark(AppState& state, const HitDefinition& hitDef, const FighterState& attacker, const FighterState& target) {
    spawnContactSpark(state, hitDef.sparkNo, hitDef, attacker, target);
}

void spawnGuardSpark(AppState& state, const HitDefinition& hitDef, const FighterState& attacker, const FighterState& target) {
    spawnContactSpark(state, hitDef.guardSparkNo, hitDef, attacker, target);
}

bool runtimeEffectCanUpdateDuringGlobalPause(const AppState& state, const RuntimeEffect& effect) {
    if (!globalPauseActive(state)) {
        return true;
    }
    const int moveTime = state.globalPauseIsSuper ? effect.superMoveTime : effect.pauseMoveTime;
    return moveTime < 0 || effect.ageTicks < moveTime;
}

void updateRuntimeEffects(AppState& state) {
    updateEnvShake(state);
    updateEnvColor(state);
    updatePaletteEffect(state.backgroundPaletteEffect);
    for (auto& fighter : state.fighters) {
        updatePaletteEffect(fighter.paletteEffect);
    }
    for (auto& effect : state.runtimeEffects) {
        if (!runtimeEffectCanUpdateDuringGlobalPause(state, effect)) {
            continue;
        }
        ++effect.animTick;
        ++effect.ageTicks;
        if (effect.bindTicks != 0 && effect.ownerIndex >= 0 && effect.ownerIndex < static_cast<int>(state.fighters.size())) {
            const FighterState& owner = state.fighters[static_cast<size_t>(effect.ownerIndex)];
            effect.x = owner.x + effect.bindOffsetX * static_cast<float>(owner.facing);
            effect.y = owner.y + effect.bindOffsetY;
            effect.depthZ = owner.depthZ;
            if (effect.bindTicks > 0) {
                --effect.bindTicks;
            }
        }
    }
    state.runtimeEffects.erase(
        std::remove_if(state.runtimeEffects.begin(), state.runtimeEffects.end(), [&state](const RuntimeEffect& effect) {
            const AnimationClip* clip = findExactClipForRuntimeEffect(state, effect);
            if (!clip) {
                return true;
            }
            if (effect.removeTime >= 0) {
                return effect.ageTicks >= effect.removeTime;
            }
            if (effect.removeTime == -1) {
                return false;
            }
            return !clip->hasInfiniteDuration && effect.animTick >= clip->loopTicks;
        }),
        state.runtimeEffects.end());
}

const AnimationFrame* currentFrameForFighter(const AppState& state, const FighterState& fighter) {
    const AnimationClip* clip = findClipForActor(state, fighter, fighter.action);
    return clip ? frameForClip(*clip, fighter.animTick) : nullptr;
}

