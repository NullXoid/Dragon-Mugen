#pragma once

// Internal App.cpp view assembly file.
// This file depends on App.cpp-local AppState/FighterState/stage/animation helpers.
// Include only from App.cpp after the required types/helpers are defined.
// Do not include from other translation units.

TrainingDebugColorView trainingDebugColor(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a = 255) {
    return TrainingDebugColorView{ r, g, b, a };
}

void appendDebugLine(
    std::vector<TrainingDebugLineView>& lines,
    float x1,
    float y1,
    float x2,
    float y2,
    TrainingDebugColorView color) {
    lines.push_back(TrainingDebugLineView{ x1, y1, x2, y2, color });
}

void appendFighterOrigin(std::vector<TrainingDebugLineView>& lines, const AppState& state, const StageSlot& stage, size_t fighterIndex) {
    if (fighterIndex >= state.fighters.size()) {
        return;
    }

    const auto& fighter = state.fighters[fighterIndex];
    const float x = screenCenterX(state) + fighter.x - state.cameraX;
    const float y = stage.zoffset + fighter.y - state.cameraY;
    const TrainingDebugColorView color = fighterIndex == 0
        ? trainingDebugColor(100, 210, 255)
        : trainingDebugColor(255, 180, 90);
    appendDebugLine(lines, x - 5.0f, y, x + 5.0f, y, color);
    appendDebugLine(lines, x, y - 5.0f, x, y + 5.0f, color);
}

SDL_FRect collisionBoxToScreen(
    const CollisionBox& box,
    const FighterState& fighter,
    const AnimationFrame& frame,
    const AppState& state,
    const StageSlot& stage) {
    const bool facingLeft = fighter.facing < 0;
    const bool mirrorX = frame.flipX != facingLeft;
    float x1 = box.x1 * fighter.scaleX;
    float x2 = box.x2 * fighter.scaleX;
    float y1 = box.y1 * fighter.scaleY;
    float y2 = box.y2 * fighter.scaleY;

    if (mirrorX) {
        x1 = -box.x2 * fighter.scaleX;
        x2 = -box.x1 * fighter.scaleX;
    }
    if (frame.flipY) {
        y1 = -box.y2 * fighter.scaleY;
        y2 = -box.y1 * fighter.scaleY;
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

void appendCollisionBoxList(
    std::vector<TrainingDebugRectView>& boxes,
    const std::vector<CollisionBox>& sourceBoxes,
    const FighterState& fighter,
    const AnimationFrame& frame,
    const AppState& state,
    const StageSlot& stage,
    bool attackBoxes) {
    const TrainingDebugColorView color = attackBoxes
        ? trainingDebugColor(238, 70, 70)
        : trainingDebugColor(90, 150, 255);

    for (const auto& box : sourceBoxes) {
        const SDL_FRect rect = collisionBoxToScreen(box, fighter, frame, state, stage);
        boxes.push_back(TrainingDebugRectView{
            rect.x,
            rect.y,
            rect.w,
            rect.h,
            color,
            rect.w > 3.0f && rect.h > 3.0f,
        });
    }
}

void appendFighterCollisionBoxes(std::vector<TrainingDebugRectView>& boxes, const AppState& state, const StageSlot& stage, size_t fighterIndex) {
    if (fighterIndex >= state.fighters.size()) {
        return;
    }

    const FighterState& fighter = state.fighters[fighterIndex];
    const AnimationFrame* frame = currentFrameForFighter(state, fighter);
    if (!frame) {
        return;
    }

    appendCollisionBoxList(boxes, frame->clsn2, fighter, *frame, state, stage, false);
    appendCollisionBoxList(boxes, frame->clsn1, fighter, *frame, state, stage, true);
}

std::string compactDebugLine(std::string value, size_t maxChars) {
    const size_t newline = value.find('\n');
    if (newline != std::string::npos) {
        value = value.substr(0, newline);
    }
    if (value.size() > maxChars) {
        value = value.substr(0, maxChars > 3 ? maxChars - 3 : maxChars) + "...";
    }
    return value;
}

void appendTrainingDebugReadout(std::vector<std::string>& readoutLines, const AppState& state, const StageSlot& stage) {
    if (state.fighters.size() < 2) {
        return;
    }

    const auto& p1 = state.fighters[0];
    const auto& p2 = state.fighters[1];
    const AnimationClip* p1Clip = findClipForActor(state, p1, p1.action);
    const AnimationClip* p2Clip = findClipForActor(state, p2, p2.action);
    const AnimationFrame* p1AnimFrame = p1Clip ? frameForClip(*p1Clip, p1.animTick) : nullptr;
    const AnimationFrame* p2AnimFrame = p2Clip ? frameForClip(*p2Clip, p2.animTick) : nullptr;
    const int p1Frame = p1Clip ? frameIndexForClip(*p1Clip, p1.animTick) : -1;
    const int p2Frame = p2Clip ? frameIndexForClip(*p2Clip, p2.animTick) : -1;

    readoutLines.push_back("DEBUG F2 menu  A/S/D Z/X/C  R reset");
    readoutLines.push_back("cam " + fixed1(state.cameraX) + "," + fixed1(state.cameraY)
        + "  z " + fixed1(stage.zoffset));
    readoutLines.push_back("p1 " + fixed1(p1.x) + "," + fixed1(p1.y)
        + " v " + fixed1(p1.vx) + "," + fixed1(p1.vy));
    readoutLines.push_back("p1 s" + std::to_string(p1.stateNo)
        + " t" + std::to_string(p1.stateTime)
        + " a" + std::to_string(p1.action)
        + " f" + std::to_string(p1Frame)
        + " c" + std::to_string(p1.ctrl ? 1 : 0));
    readoutLines.push_back("p2 " + fixed1(p2.x) + "," + fixed1(p2.y)
        + " s" + std::to_string(p2.stateNo)
        + " a" + std::to_string(p2.action)
        + " f" + std::to_string(p2Frame));
    readoutLines.push_back("life p1 " + std::to_string(p1.life)
        + " p2 " + std::to_string(p2.life));
    readoutLines.push_back("hit p1 pause " + std::to_string(p1.hitPauseTicks)
        + " p2 p/s/l " + std::to_string(p2.hitPauseTicks)
        + "/" + std::to_string(p2.hitStunTicks)
        + "/" + std::to_string(p2.hitSlideTicks));
    const size_t p1C1 = p1AnimFrame ? p1AnimFrame->clsn1.size() : 0;
    const size_t p1C2 = p1AnimFrame ? p1AnimFrame->clsn2.size() : 0;
    const size_t p2C1 = p2AnimFrame ? p2AnimFrame->clsn1.size() : 0;
    const size_t p2C2 = p2AnimFrame ? p2AnimFrame->clsn2.size() : 0;
    readoutLines.push_back("boxes p1 "
        + std::to_string(p1C1) + "/" + std::to_string(p1C2)
        + " p2 " + std::to_string(p2C1) + "/" + std::to_string(p2C2));
    if (!p1.debugClipboard.empty()) {
        readoutLines.push_back("clip1 " + compactDebugLine(p1.debugClipboard, 24));
    }
    if (!p2.debugClipboard.empty()) {
        readoutLines.push_back("clip2 " + compactDebugLine(p2.debugClipboard, 24));
    }
}

TrainingDebugView trainingDebugView(
    const AppState& state,
    const StageSlot& stage,
    std::vector<TrainingDebugLineView>& lines,
    std::vector<TrainingDebugRectView>& boxes,
    std::vector<std::string>& readoutLines) {
    const auto& display = state.training.options;
    if (display.showFloorLine) {
        const float floorY = stage.zoffset - state.cameraY;
        appendDebugLine(lines, 0.0f, floorY, logicalWidthF(state), floorY, trainingDebugColor(100, 230, 130));
    }
    if (display.showHitboxes) {
        appendFighterCollisionBoxes(boxes, state, stage, 0);
        appendFighterCollisionBoxes(boxes, state, stage, 1);
    }
    if (display.showOrigins) {
        appendFighterOrigin(lines, state, stage, 0);
        appendFighterOrigin(lines, state, stage, 1);
    }
    if (display.showDebugReadout) {
        appendTrainingDebugReadout(readoutLines, state, stage);
    }

    TrainingDebugView view;
    view.lines = std::span<const TrainingDebugLineView>(lines.data(), lines.size());
    view.boxes = std::span<const TrainingDebugRectView>(boxes.data(), boxes.size());
    view.readout.visible = display.showDebugReadout;
    view.readout.lines = std::span<const std::string>(readoutLines.data(), readoutLines.size());
    return view;
}

void drawDebugOverlay(SDL_Renderer* renderer, const AppState& state, const StageSlot& stage) {
    std::vector<TrainingDebugLineView> lines;
    std::vector<TrainingDebugRectView> boxes;
    std::vector<std::string> readoutLines;
    lines.reserve(5);
    boxes.reserve(16);
    readoutLines.reserve(10);

    const TrainingDebugView view = trainingDebugView(state, stage, lines, boxes, readoutLines);
    drawTrainingDebugOverlay(uiRenderContext(renderer, state), view);
}
