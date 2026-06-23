#pragma once

// Internal App.cpp implementation shard.
// Projectile and helper update runtime helpers.

FighterState projectileAsActor(const RuntimeProjectile& projectile) {
    FighterState actor;
    actor.x = projectile.x;
    actor.y = projectile.y;
    actor.depthZ = projectile.depthZ;
    actor.vx = projectile.vx;
    actor.vy = projectile.vy;
    actor.facing = projectile.facing;
    actor.action = projectile.action;
    actor.animTick = projectile.animTick;
    actor.stateNo = projectile.hitDef.stateNo;
    actor.moveType = 'A';
    actor.stateType = 'A';
    actor.physics = 'N';
    actor.ctrl = false;
    actor.onGround = false;
    return actor;
}

const AnimationFrame* currentFrameForProjectile(const AppState& state, const RuntimeProjectile& projectile) {
    if (projectile.ownerIndex < 0 || projectile.ownerIndex >= static_cast<int>(state.fighters.size())) {
        return nullptr;
    }
    const AnimationClip* clip = findClipForFighter(state, static_cast<size_t>(projectile.ownerIndex), projectile.action);
    return clip ? frameForClip(*clip, projectile.animTick) : nullptr;
}

bool projectileAnimationEnded(const AppState& state, const RuntimeProjectile& projectile) {
    if (projectile.ownerIndex < 0 || projectile.ownerIndex >= static_cast<int>(state.fighters.size())) {
        return true;
    }
    const AnimationClip* clip = findExactClipForFighter(state, static_cast<size_t>(projectile.ownerIndex), projectile.action);
    return !clip || (!clip->hasInfiniteDuration && !clip->hasLoopStart && projectile.animTick >= std::max(1, clip->loopTicks));
}

enum class ProjectileRemovalReason {
    Timeout,
    Hit,
    Cancel,
};

void beginProjectileRemoval(
    const AppState& state,
    RuntimeProjectile& projectile,
    int fallbackTicks = 12,
    ProjectileRemovalReason reason = ProjectileRemovalReason::Timeout) {
    if (projectile.removing) {
        return;
    }
    projectile.removing = true;
    projectile.vx = projectile.removeVx;
    projectile.vy = projectile.removeVy;
    projectile.animTick = 0;
    if (reason == ProjectileRemovalReason::Hit && projectile.hitAction >= 0) {
        projectile.action = projectile.hitAction;
    } else if (reason == ProjectileRemovalReason::Cancel && projectile.cancelAction >= 0) {
        projectile.action = projectile.cancelAction;
    } else if (projectile.removeAction >= 0) {
        projectile.action = projectile.removeAction;
    } else if (projectile.hitAction >= 0) {
        projectile.action = projectile.hitAction;
    }
    const AnimationClip* removeClip = projectile.ownerIndex >= 0 && projectile.ownerIndex < static_cast<int>(state.fighters.size())
        ? findExactClipForFighter(state, static_cast<size_t>(projectile.ownerIndex), projectile.action)
        : nullptr;
    projectile.removeTime = removeClip && !removeClip->hasLoopStart
        ? -1
        : fallbackTicks;
}

bool projectileCanUpdateDuringGlobalPause(const AppState& state, const RuntimeProjectile& projectile) {
    if (!globalPauseActive(state)) {
        return true;
    }
    const int moveTime = state.globalPauseIsSuper ? projectile.superMoveTime : projectile.pauseMoveTime;
    return moveTime < 0 || moveTime > 0;
}

void consumeProjectileGlobalPauseMoveTick(const AppState& state, RuntimeProjectile& projectile) {
    if (!globalPauseActive(state)) {
        return;
    }
    int& moveTime = state.globalPauseIsSuper ? projectile.superMoveTime : projectile.pauseMoveTime;
    if (moveTime > 0) {
        --moveTime;
    }
}

bool helperCanUpdateDuringGlobalPause(const AppState& state, const FighterState& helper) {
    if (!globalPauseActive(state)) {
        return true;
    }
    if (!helper.helper || helper.ownerIndex != state.globalPauseOwnerIndex) {
        return false;
    }
    const int moveTime = state.globalPauseIsSuper ? helper.superMoveTime : helper.pauseMoveTime;
    return moveTime < 0 || moveTime > 0;
}

void consumeHelperGlobalPauseMoveTick(const AppState& state, FighterState& helper) {
    if (!globalPauseActive(state) || !helper.helper || helper.ownerIndex != state.globalPauseOwnerIndex) {
        return;
    }
    int& moveTime = state.globalPauseIsSuper ? helper.superMoveTime : helper.pauseMoveTime;
    if (moveTime > 0) {
        --moveTime;
    }
}

bool projectileBoxesOverlap(const AppState& state, const RuntimeProjectile& lhs, const RuntimeProjectile& rhs) {
    FighterState lhsActor = projectileAsActor(lhs);
    FighterState rhsActor = projectileAsActor(rhs);
    const AnimationFrame* lhsFrame = currentFrameForProjectile(state, lhs);
    const AnimationFrame* rhsFrame = currentFrameForProjectile(state, rhs);
    if (!lhsFrame || !rhsFrame) {
        return false;
    }
    const auto& lhsBoxes = lhsFrame->clsn1.empty() ? lhsFrame->clsn2 : lhsFrame->clsn1;
    const auto& rhsBoxes = rhsFrame->clsn1.empty() ? rhsFrame->clsn2 : rhsFrame->clsn1;
    if (lhsBoxes.empty() || rhsBoxes.empty()) {
        return false;
    }
    for (const auto& lhsBox : lhsBoxes) {
        const CollisionBox lhsWorld = collisionBoxToWorldScaled(lhsBox, lhsActor, *lhsFrame, lhs.scaleX, lhs.scaleY);
        for (const auto& rhsBox : rhsBoxes) {
            const CollisionBox rhsWorld = collisionBoxToWorldScaled(rhsBox, rhsActor, *rhsFrame, rhs.scaleX, rhs.scaleY);
            if (boxesOverlap(lhsWorld, rhsWorld)) {
                return true;
            }
        }
    }
    return false;
}

void resolveProjectileCollisions(AppState& state) {
    for (size_t i = 0; i < state.projectiles.size(); ++i) {
        auto& lhs = state.projectiles[i];
        if (lhs.removing) {
            continue;
        }
        for (size_t j = i + 1; j < state.projectiles.size(); ++j) {
            auto& rhs = state.projectiles[j];
            if (rhs.removing || lhs.ownerIndex == rhs.ownerIndex || !projectileBoxesOverlap(state, lhs, rhs)) {
                continue;
            }
            if (lhs.priority == rhs.priority) {
                beginProjectileRemoval(state, lhs, 12, ProjectileRemovalReason::Cancel);
                beginProjectileRemoval(state, rhs, 12, ProjectileRemovalReason::Cancel);
            } else if (lhs.priority > rhs.priority) {
                beginProjectileRemoval(state, rhs, 12, ProjectileRemovalReason::Cancel);
                lhs.priority = std::max(0, lhs.priority - 1);
            } else {
                beginProjectileRemoval(state, lhs, 12, ProjectileRemovalReason::Cancel);
                rhs.priority = std::max(0, rhs.priority - 1);
            }
            if (lhs.removing) {
                break;
            }
        }
    }
}

void applyProjectileHit(AppState& state, RuntimeProjectile& projectile, size_t defenderIndex) {
    if (projectile.ownerIndex < 0
        || projectile.ownerIndex >= static_cast<int>(state.fighters.size())
        || defenderIndex >= state.fighters.size()
        || projectile.ownerIndex == static_cast<int>(defenderIndex)
        || projectile.hitCooldownTicks > 0) {
        return;
    }

    auto& owner = state.fighters[static_cast<size_t>(projectile.ownerIndex)];
    auto& defender = state.fighters[defenderIndex];
    if (!arenaProjectileDepthOverlapsDefender(state, projectile, defender)) {
        return;
    }
    const HitDefinition resolvedHitDef = resolveHitDefinitionExpressions(
        state,
        projectile.hitDef,
        owner,
        defender,
        selectedStageSlot(state.selection));
    const HitDefinition& hitDef = resolvedHitDef;
    if (!defenderCanBeHitBy(defender, hitDef)) {
        return;
    }
    if (!hitFlagAllowsDefender(hitDef, defender)) {
        return;
    }

    FighterState projectileActor = projectileAsActor(projectile);
    const AnimationFrame* attackFrame = currentFrameForProjectile(state, projectile);
    const AnimationClip* defendClip = findClipForFighter(state, defenderIndex, defender.action);
    const AnimationFrame* defendFrame = defendClip ? frameForClip(*defendClip, defender.animTick) : nullptr;
    if (!attackFrame || !defendFrame || attackFrame->clsn1.empty() || defendFrame->clsn2.empty()) {
        return;
    }
    bool hitBoxesOverlap = false;
    for (const auto& attackBox : attackFrame->clsn1) {
        const CollisionBox attackWorld = collisionBoxToWorldScaled(attackBox, projectileActor, *attackFrame, projectile.scaleX, projectile.scaleY);
        for (const auto& hurtBox : defendFrame->clsn2) {
            const CollisionBox hurtWorld = collisionBoxToWorldScaled(
                hurtBox,
                defender,
                *defendFrame,
                defender.scaleX,
                defender.scaleY);
            if (boxesOverlap(attackWorld, hurtWorld)) {
                hitBoxesOverlap = true;
                break;
            }
        }
        if (hitBoxesOverlap) {
            break;
        }
    }
    if (!hitBoxesOverlap) {
        return;
    }

    const bool trainingDummy = useTrainingDummyOptions(state, defenderIndex);
    GuardStance guardStance = trainingDummy
        ? chooseDummyGuardStance(state.training.options, hitDef, defender)
        : choosePlayerGuardStance(hitDef, defender);
    if (guardStance != GuardStance::None
        && p2BodyDistXValue(state, projectileActor, defender) > static_cast<float>(effectiveGuardDistance(state, owner, hitDef))) {
        guardStance = GuardStance::None;
    }
    const StateHitOverrideController* hitOverride = guardStance == GuardStance::None
        ? activeHitOverrideForDefender(state, defender, &owner, hitDef)
        : nullptr;

    owner.moveContact = true;
    ++owner.hitCount;
    owner.projectileContactId = projectile.id;
    owner.projectileContactTicks = std::max(owner.projectileContactTicks, 32);
    startEnvShake(state, hitDef.envShake);
    startPaletteEffect(defender.paletteEffect, hitDef.palFx);

    std::ostringstream hitText;
    if (guardStance != GuardStance::None) {
        owner.moveGuarded = true;
        endActiveComboForDefender(state, defenderIndex);
        owner.projectileGuardedId = projectile.id;
        owner.projectileGuardedTicks = std::max(owner.projectileGuardedTicks, 32);
        owner.hitPauseTicks = fightHitPauseTicks(state, hitDef.pausetimeP1, 0);
        const int guardDamageDone = state.training.options.guardDamage
            ? scaleAttackThenDefenceDamage(state, hitDef.guardDamage, owner, defender)
            : 0;
        if (!state.training.options.dummyInvincible && state.training.options.guardDamage) {
            defender.life = std::max(0, defender.life - guardDamageDone);
        }
        if (state.training.options.dummyFrozen) {
            clearFighterHitRuntime(defender);
            enterState(state, defender, 0);
        } else {
            enterGroundGuardHitState(state, defender, hitDef, projectile.facing, guardStance);
        }
        spawnGuardSpark(state, hitDef, projectileActor, defender);
        if (shouldPlayFightSounds(state)) {
            playSound(state, hitDef.guardSoundGroup, hitDef.guardSoundIndex, hitDef.guardSoundForceCommon, -1, false, 1.0f, false, projectile.ownerIndex);
        }
        hitText << fighterLabel(static_cast<size_t>(projectile.ownerIndex)) << " proj guard " << hitDef.stateNo << "#" << projectile.id
                << " gdmg " << guardDamageDone
                << " spark " << hitDef.guardSparkNo
                << " snd " << soundPairText(hitDef.guardSoundGroup, hitDef.guardSoundIndex);
    } else {
        owner.moveHit = true;
        owner.projectileHitId = projectile.id;
        owner.projectileHitTicks = std::max(owner.projectileHitTicks, 32);
        owner.hitPauseTicks = fightHitPauseTicks(state, hitDef.pausetimeP1, 0);
        const int damageDone = (!trainingDummy || !state.training.options.dummyInvincible)
            ? scaleAttackThenDefenceDamage(state, hitDef.damage, owner, defender)
            : 0;
        if (damageDone > 0) {
            defender.life = std::max(0, defender.life - damageDone);
        }
        if (!hitOverride) {
            owner.targetIndex = static_cast<int>(defenderIndex);
            owner.targetHitId = hitDef.targetId;
            owner.targetTicks = std::max(owner.targetTicks, kStoredTargetLinkTicks);
        }
        if (trainingDummy && state.training.options.dummyFrozen) {
            clearFighterHitRuntime(defender);
            enterState(state, defender, 0);
        } else if (hitOverride) {
            const bool wasAirborne = !defender.onGround || defender.stateType == 'A';
            const bool wasLyingDown = fighterIsLyingDownForHit(defender);
            const float downVelocityX = (hitDef.hasDownVelocity ? hitDef.downVelocityX : hitDef.airVelocityX)
                * -static_cast<float>(projectile.facing);
            const float downVelocityY = hitDef.hasDownVelocity ? hitDef.downVelocityY : hitDef.airVelocityY;
            enterState(state, defender, hitOverride->stateNo);
            defender.customHitState = true;
            defender.moveType = 'H';
            defender.ctrl = false;
            defender.hitPauseTicks = fightHitPauseTicks(state, hitDef.pausetimeP2, 0);
            defender.hitStunTicks = std::max(hitDef.groundHitTime, defender.hitPauseTicks);
            setGetHitVarsFromHitDef(
                defender,
                hitDef,
                wasAirborne,
                wasLyingDown,
                hitDefCausesFall(hitDef, wasAirborne),
                -hitDef.groundVelocityX * static_cast<float>(projectile.facing),
                hitDef.groundVelocityY,
                downVelocityX,
                downVelocityY);
            applyHitDefP2Facing(defender, hitDef, projectile.facing);
        } else if (enterCustomHitState(state, defender, hitDef, projectile.facing, projectile.ownerIndex)) {
            // Custom projectile p2stateno states are driven by the target CNS after entry.
        } else {
            enterGroundGetHitState(state, defender, hitDef, projectile.x, projectile.y, projectile.facing);
        }
        spawnHitSpark(state, hitDef, projectileActor, defender);
        if (shouldPlayFightSounds(state)) {
            playSound(state, hitDef.hitSoundGroup, hitDef.hitSoundIndex, hitDef.hitSoundForceCommon, -1, false, 1.0f, false, projectile.ownerIndex);
        }
        registerComboHit(state, static_cast<size_t>(projectile.ownerIndex));
        hitText << fighterLabel(static_cast<size_t>(projectile.ownerIndex)) << " proj hit " << hitDef.stateNo << "#" << projectile.id
                << " dmg " << damageDone
                << " spark " << hitDef.sparkNo
                << " snd " << soundPairText(hitDef.hitSoundGroup, hitDef.hitSoundIndex);
    }

    if (projectile.removeWhenHit != 0) {
        --projectile.hitsRemaining;
        if (projectile.hitsRemaining <= 0) {
            beginProjectileRemoval(state, projectile, 12, ProjectileRemovalReason::Hit);
        }
    }
    projectile.hitCooldownTicks = std::max(projectile.hitCooldownTicks, projectile.missTime);
    state.messages.lastHitText = hitText.str();
    state.messages.lastHitTextTicks = 150;
    logFightHitEvent(state.messages.lastHitText);
}

void updateRuntimeProjectiles(AppState& state, const StageSlot& stage) {
    FramePerfScope scope(state.framePerf, FramePerfSection::ProjectileUpdate);
    for (auto& projectile : state.projectiles) {
        if (!projectileCanUpdateDuringGlobalPause(state, projectile)) {
            continue;
        }
        if (projectile.hitCooldownTicks > 0) {
            --projectile.hitCooldownTicks;
        }
        if (!projectile.removing) {
            projectile.vx = (projectile.vx + projectile.ax) * projectile.velMulX;
            projectile.vy = (projectile.vy + projectile.ay) * projectile.velMulY;
        }
        projectile.x += projectile.vx * static_cast<float>(projectile.facing);
        projectile.y += projectile.vy;
        ++projectile.animTick;
        if (projectile.removeTime > 0) {
            --projectile.removeTime;
        }

        if (!projectile.removing) {
            const float visibleLeft = state.cameraX - logicalWidthF(state) / 2.0f;
            const float visibleRight = state.cameraX + logicalWidthF(state) / 2.0f;
            const float deadLeft = std::max(stage.leftbound - projectile.projStageBound, visibleLeft - projectile.projEdgeBound);
            const float deadRight = std::min(stage.rightbound + projectile.projStageBound, visibleRight + projectile.projEdgeBound);
            if (projectile.removeTime == 0
                || projectile.x < deadLeft
                || projectile.x > deadRight
                || projectile.y < projectile.projHeightBoundLow
                || projectile.y > projectile.projHeightBoundHigh) {
                beginProjectileRemoval(state, projectile);
                continue;
            }
            size_t defenderIndex = projectile.ownerIndex == 0 ? 1 : 0;
            if (state.frontend.pendingMode == PendingMode::Story) {
                const int storyDefender = storyProjectileDefenderIndex(state, projectile.ownerIndex);
                if (storyDefender < 0) {
                    continue;
                }
                defenderIndex = static_cast<size_t>(storyDefender);
            } else if (state.frontend.pendingMode == PendingMode::Arena) {
                const int arenaDefender = nearestLivingEnemyIndex(state, projectile.ownerIndex);
                if (arenaDefender < 0) {
                    continue;
                }
                defenderIndex = static_cast<size_t>(arenaDefender);
            }
            applyProjectileHit(state, projectile, defenderIndex);
        }
        consumeProjectileGlobalPauseMoveTick(state, projectile);
    }
    {
        FramePerfScope collisionScope(state.framePerf, FramePerfSection::CollisionHitRouting);
        resolveProjectileCollisions(state);
    }

    state.projectiles.erase(
        std::remove_if(state.projectiles.begin(), state.projectiles.end(), [&state](const RuntimeProjectile& projectile) {
            if (!projectile.removing) {
                return false;
            }
            if (projectile.removeTime == 0) {
                return true;
            }
            return projectileAnimationEnded(state, projectile);
        }),
        state.projectiles.end());
}

void eraseDestroyedHelpers(AppState& state) {
    state.helpers.erase(
        std::remove_if(state.helpers.begin(), state.helpers.end(), [](const FighterState& helper) {
            return helper.destroyRequested;
        }),
        state.helpers.end());
}

void updateHelperActors(AppState& state, const StageSlot& stage) {
    FramePerfScope scope(state.framePerf, FramePerfSection::HelperUpdate);
    const size_t helperCount = state.helpers.size();
    for (size_t i = 0; i < helperCount && i < state.helpers.size(); ++i) {
        auto& helper = state.helpers[i];
        if (helper.destroyRequested) {
            continue;
        }
        if (!helperCanUpdateDuringGlobalPause(state, helper)) {
            helper.vx = 0.0f;
            helper.vy = 0.0f;
            continue;
        }
        FighterState* opponent = opponentForActor(state, helper);
        updateStateMovementControllers(state, helper, opponent, &stage);
        updateStateHelperControllers(state, helper, opponent, &stage);
        updateStateProjectileControllers(state, helper, opponent, &stage);
        updateStateMakeDustControllers(state, helper, opponent, stage);
        updateStateExplodControllers(state, helper, opponent, stage);
        const bool changedBeforePhysics = updateStateChangeStateControllers(state, helper, opponent, &stage);
        applyRootBinding(state, helper);
        updateFighterPhysics(state, helper, stage);
        applyRootBinding(state, helper);
        if (!changedBeforePhysics && updateStateChangeStateControllers(state, helper, opponent, &stage) && helper.y >= 0.0f && helper.stateType != 'A') {
            helper.y = 0.0f;
            helper.vy = 0.0f;
            helper.onGround = true;
        }
        updateStateTargetControllers(state, helper, opponent, &stage);
        updateStateChangeAnimControllers(state, helper, opponent, &stage);
        updateStatePosAddControllers(state, helper, opponent, &stage);
        applyTargetBindings(state);
        updateStateCtrlControllers(state, helper);
        updateStateAudioControllers(state, helper, opponent, &stage);
        if (helper.hitPauseTicks > 0) {
            --helper.hitPauseTicks;
        } else {
            ++helper.animTick;
            ++helper.stateTime;
        }
        updateStateHelperControllers(state, helper, opponent, &stage);
        if (helper.destroyRequested) {
            continue;
        }
        consumeHelperGlobalPauseMoveTick(state, helper);
        updateAfterImageEffect(helper);
        finishStateIfAnimationEnded(state, helper);
    }
    eraseDestroyedHelpers(state);
}

#include "CommandRecognition.h"
