#pragma once

// Internal App.cpp implementation shard.
// Jump buffering, held-repeat, and common air-steering helpers.

#include <algorithm>

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

bool shouldQueueHeldJumpRepeat(const FighterState& fighter, const FighterInputState& input,
    bool attackButtonHeld, bool holdingDown, bool movementLocked) {
    const bool canAct = fighter.onGround && fighter.ctrl && fighter.moveType == 'I'
        && !fighter.guarding && fighter.hitPauseTicks <= 0;
    return input.up && fighter.jumpInputConsumedWhileHeld && fighter.jumpInputBufferTicks <= 0
        && canAct && !attackButtonHeld && !holdingDown && !movementLocked;
}

float approachAirVelocity(float current, float target, float step) {
    if (current < target) {
        return std::min(current + step, target);
    }
    if (current > target) {
        return std::max(current - step, target);
    }
    return current;
}

void applyControlledAirSteer(
    AppState& state,
    FighterState& fighter,
    const FighterInputState& input,
    bool attackButtonHeld,
    bool movementLocked) {
    if (fighter.onGround
        || fighter.physics != 'A'
        || fighter.moveType != 'I'
        || fighter.jumpBaseAction < 41
        || fighter.jumpBaseAction > 43
        || fighter.guarding
        || fighter.hitPauseTicks > 0
        || attackButtonHeld
        || movementLocked) {
        return;
    }

    const bool holdingForward = (fighter.facing >= 0 && input.right) || (fighter.facing < 0 && input.left);
    const bool holdingBack = (fighter.facing >= 0 && input.left) || (fighter.facing < 0 && input.right);
    if (holdingForward == holdingBack) {
        return;
    }

    const CharacterConstants& constants = characterConstantsForActor(state, fighter);
    const float targetLocalVelocity = holdingForward ? constants.velocityJumpFwdX : constants.velocityJumpBackX;
    const float targetVelocity = targetLocalVelocity * static_cast<float>(fighter.facing);
    constexpr float kAirSteerStep = 0.18f;
    fighter.vx = approachAirVelocity(fighter.vx, targetVelocity, kAirSteerStep);
    fighter.jumpBaseAction = holdingForward ? 42 : 43;
    if (findExactClipForActor(state, fighter, fighter.jumpBaseAction)) {
        setFighterAction(fighter, fighter.jumpBaseAction);
    }
}
