#pragma once

// Internal App.cpp extraction: render-only F1/debug overlay helpers.
// This file is included from inside App.cpp's anonymous namespace after the
// AppState/FighterState render helpers are defined.

void drawFighterOrigin(SDL_Renderer* renderer, const AppState& state, const StageSlot& stage, size_t fighterIndex) {
    if (fighterIndex >= state.fighters.size()) {
        return;
    }

    const auto& fighter = state.fighters[fighterIndex];
    const float x = screenCenterX(state) + fighter.x - state.cameraX;
    const float y = stage.zoffset + fighter.y - state.cameraY;
    if (fighterIndex == 0) {
        setColor(renderer, 100, 210, 255);
    } else {
        setColor(renderer, 255, 180, 90);
    }
    fillRect(renderer, x - 5.0f, y, 10.0f, 1.0f);
    fillRect(renderer, x, y - 5.0f, 1.0f, 10.0f);
}

SDL_FRect collisionBoxToScreen(
    const CollisionBox& box,
    const FighterState& fighter,
    const AnimationFrame& frame,
    const AppState& state,
    const StageSlot& stage) {
    const bool facingLeft = fighter.facing < 0;
    const bool mirrorX = frame.flipX != facingLeft;
    float x1 = box.x1;
    float x2 = box.x2;
    float y1 = box.y1;
    float y2 = box.y2;

    if (mirrorX) {
        x1 = -box.x2;
        x2 = -box.x1;
    }
    if (frame.flipY) {
        y1 = -box.y2;
        y2 = -box.y1;
    }

    const float left = std::min(x1, x2);
    const float right = std::max(x1, x2);
    const float top = std::min(y1, y2);
    const float bottom = std::max(y1, y2);
    const float originX = screenCenterX(state) + fighter.x - state.cameraX;
    const float originY = stage.zoffset + fighter.y - state.cameraY;
    return SDL_FRect{
        originX + left,
        originY + top,
        std::max(1.0f, right - left),
        std::max(1.0f, bottom - top),
    };
}

void drawCollisionBoxList(
    SDL_Renderer* renderer,
    const std::vector<CollisionBox>& boxes,
    const FighterState& fighter,
    const AnimationFrame& frame,
    const AppState& state,
    const StageSlot& stage,
    bool attackBoxes) {
    if (attackBoxes) {
        setColor(renderer, 238, 70, 70);
    } else {
        setColor(renderer, 90, 150, 255);
    }

    for (const auto& box : boxes) {
        const SDL_FRect rect = collisionBoxToScreen(box, fighter, frame, state, stage);
        SDL_RenderRect(renderer, &rect);
        if (rect.w > 3.0f && rect.h > 3.0f) {
            SDL_FRect inner{ rect.x + 1.0f, rect.y + 1.0f, rect.w - 2.0f, rect.h - 2.0f };
            SDL_RenderRect(renderer, &inner);
        }
    }
}

void drawFighterCollisionBoxes(SDL_Renderer* renderer, const AppState& state, const StageSlot& stage, size_t fighterIndex) {
    if (fighterIndex >= state.fighters.size()) {
        return;
    }

    const FighterState& fighter = state.fighters[fighterIndex];
    const AnimationClip* clip = findClip(state, fighter.action);
    const AnimationFrame* frame = clip ? frameForClip(*clip, fighter.animTick) : nullptr;
    if (!frame) {
        return;
    }

    drawCollisionBoxList(renderer, frame->clsn2, fighter, *frame, state, stage, false);
    drawCollisionBoxList(renderer, frame->clsn1, fighter, *frame, state, stage, true);
}

void drawDebugOverlay(SDL_Renderer* renderer, const AppState& state, const StageSlot& stage) {
    const auto& display = state.trainingOptions;
    if (display.showFloorLine) {
        const float floorY = stage.zoffset - state.cameraY;
        setColor(renderer, 100, 230, 130);
        fillRect(renderer, 0, floorY, logicalWidthF(state), 1.0f);
    }
    if (display.showHitboxes) {
        drawFighterCollisionBoxes(renderer, state, stage, 0);
        drawFighterCollisionBoxes(renderer, state, stage, 1);
    }
    if (display.showOrigins) {
        drawFighterOrigin(renderer, state, stage, 0);
        drawFighterOrigin(renderer, state, stage, 1);
    }
    if (!display.showDebugReadout) {
        return;
    }

    setColor(renderer, 8, 10, 12);
    fillRect(renderer, 4, 42, 250, 130);
    setColor(renderer, 78, 90, 112);
    drawRect(renderer, 4, 42, 250, 130);

    const auto& p1 = state.fighters[0];
    const auto& p2 = state.fighters[1];
    const AnimationClip* p1Clip = findClip(state, p1.action);
    const AnimationClip* p2Clip = findClip(state, p2.action);
    const AnimationFrame* p1AnimFrame = p1Clip ? frameForClip(*p1Clip, p1.animTick) : nullptr;
    const AnimationFrame* p2AnimFrame = p2Clip ? frameForClip(*p2Clip, p2.animTick) : nullptr;
    const int p1Frame = p1Clip ? frameIndexForClip(*p1Clip, p1.animTick) : -1;
    const int p2Frame = p2Clip ? frameIndexForClip(*p2Clip, p2.animTick) : -1;

    setColor(renderer, 210, 224, 238);
    debugText(renderer, 8, 46, "DEBUG F2 menu  A/S/D Z/X/C  R reset");
    setColor(renderer, 155, 164, 174);
    debugText(renderer, 8, 58, "cam " + fixed1(state.cameraX) + "," + fixed1(state.cameraY)
        + "  z " + fixed1(stage.zoffset));
    debugText(renderer, 8, 70, "p1 " + fixed1(p1.x) + "," + fixed1(p1.y)
        + " v " + fixed1(p1.vx) + "," + fixed1(p1.vy));
    debugText(renderer, 8, 82, "p1 s" + std::to_string(p1.stateNo)
        + " t" + std::to_string(p1.stateTime)
        + " a" + std::to_string(p1.action)
        + " f" + std::to_string(p1Frame)
        + " c" + std::to_string(p1.ctrl ? 1 : 0));
    debugText(renderer, 8, 94, "p2 " + fixed1(p2.x) + "," + fixed1(p2.y)
        + " s" + std::to_string(p2.stateNo)
        + " a" + std::to_string(p2.action)
        + " f" + std::to_string(p2Frame));
    debugText(renderer, 8, 106, "life p1 " + std::to_string(p1.life)
        + " p2 " + std::to_string(p2.life));
    debugText(renderer, 8, 118, "hit p1 pause " + std::to_string(p1.hitPauseTicks)
        + " p2 p/s/l " + std::to_string(p2.hitPauseTicks)
        + "/" + std::to_string(p2.hitStunTicks)
        + "/" + std::to_string(p2.hitSlideTicks));
    const size_t p1C1 = p1AnimFrame ? p1AnimFrame->clsn1.size() : 0;
    const size_t p1C2 = p1AnimFrame ? p1AnimFrame->clsn2.size() : 0;
    const size_t p2C1 = p2AnimFrame ? p2AnimFrame->clsn1.size() : 0;
    const size_t p2C2 = p2AnimFrame ? p2AnimFrame->clsn2.size() : 0;
    debugText(renderer, 8, 130, "boxes p1 "
        + std::to_string(p1C1) + "/" + std::to_string(p1C2)
        + " p2 " + std::to_string(p2C1) + "/" + std::to_string(p2C2));
    auto compactDebugLine = [](std::string value, size_t maxChars) {
        const size_t newline = value.find('\n');
        if (newline != std::string::npos) {
            value = value.substr(0, newline);
        }
        if (value.size() > maxChars) {
            value = value.substr(0, maxChars > 3 ? maxChars - 3 : maxChars) + "...";
        }
        return value;
    };
    if (!p1.debugClipboard.empty()) {
        debugText(renderer, 8, 142, "clip1 " + compactDebugLine(p1.debugClipboard, 24));
    }
    if (!p2.debugClipboard.empty()) {
        debugText(renderer, 8, 154, "clip2 " + compactDebugLine(p2.debugClipboard, 24));
    }
}
