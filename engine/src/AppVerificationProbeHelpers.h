#pragma once

FighterInputState verificationProbeInput(const verification::SymbolicInput& input) {
    return FighterInputState{
        input.left,
        input.right,
        input.up,
        input.down,
        input.s,
        input.x,
        input.y,
        input.z,
        input.a,
        input.b,
        input.c,
        input.depthModifier,
    };
}

SDL_Keycode verificationProbeKeyCode(std::string_view key) {
    if (key == "enter") return SDLK_RETURN;
    if (key == "space") return SDLK_SPACE;
    if (key == "escape") return SDLK_ESCAPE;
    if (key == "up") return SDLK_UP;
    if (key == "down") return SDLK_DOWN;
    if (key == "left") return SDLK_LEFT;
    if (key == "right") return SDLK_RIGHT;
    if (key == "f2") return SDLK_F2;
    if (key == "r") return SDLK_R;
    return 0;
}

verification::FighterSnapshot verificationProbeFighterSnapshot(const FighterState& fighter, int maxLife) {
    return verification::FighterSnapshot{
        fighter.x,
        fighter.y,
        fighter.depthZ,
        fighter.vx,
        fighter.vy,
        fighter.depthVz,
        fighter.stateNo,
        fighter.action,
        fighter.stateTime,
        fighter.animTick,
        fighter.life,
        maxLife,
        fighter.power,
        fighter.attackMultiplier,
        fighter.defenceMultiplier,
        fighter.targetIndex,
        fighter.targetTicks,
        fighter.targetHitId,
        fighter.hitCount,
        fighter.paletteNo,
        fighter.hitPauseTicks,
        fighter.hitStunTicks,
        fighter.hitDownVelocityX,
        fighter.hitDownVelocityY,
        fighter.displayOffsetX,
        fighter.displayOffsetY,
        fighter.scaleX,
        fighter.scaleY,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        fighter.facing,
        fighter.stateType,
        fighter.moveType,
        fighter.physics,
        fighter.ctrl,
        fighter.onGround,
        fighter.moveContact,
        fighter.moveHit,
        fighter.moveGuarded,
        fighter.afterImage.active,
        static_cast<int>(fighter.afterImage.trail.size()),
    };
}
